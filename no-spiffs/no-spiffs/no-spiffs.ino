#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

const char* ssid = "Battery.local"; // visti http://battery.local after connecting to view the battery/solar power status in 'real time'!

ESP8266WebServer server(80);
DNSServer dnsServer;


void sendBatteryPage() {

  String html =
      "<!DOCTYPE html>"
      "<html>"
      "<head>"
      "<meta name='viewport' content='width=device-width, initial-scale=1'>"
      "<style>"
      "body{"
      "margin:0;"
      "height:100vh;"
      "display:flex;"
      "justify-content:center;"
      "align-items:center;"
      "font-family:sans-serif;"
      "font-size:4rem;"
      "transition:background-color 1s;"
      "}"
      "</style>"
      "</head>"
      "<body>"
      "<div id='adc'>---</div>"

      "<script>"

      "async function updateADC(){"

      "let r=await fetch('/a0');"
      "let txt=await r.text();"
      "let raw=parseInt(txt);"

      "document.getElementById('adc').innerHTML=raw;"

      "let hue=(raw/1023.0)*120;"
      "document.body.style.backgroundColor="
      "'hsl('+hue+',100%,50%)';"

      "}"

      "updateADC();"
      "setInterval(updateADC,1000);"

      "</script>"
      "</body>"
      "</html>";

  server.send(200, "text/html", html);
}


void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, "12345678"); // use a password to reliably connect .....
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.println("DNS started");

//  MDNS.begin("battery");

  Serial.println();
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Normal mode
  server.on("/", sendBatteryPage); 

  
  // Captive portal catchers
  server.on("/hotspot-detect.html", sendBatteryPage);
  
  server.on("/generate_204", sendBatteryPage);
  
  server.on("/connecttest.txt", sendBatteryPage);
  
  server.on("/library/test/success.html", sendBatteryPage);
  
  server.on("/success.txt", sendBatteryPage);

  // Redirect all traffic to index.html
  server.onNotFound([]() {
//      server.sendHeader("Location", "http://192.168.4.1/", true);
//      server.send(302, "text/plain", "");

  Serial.println("-----");
  Serial.print("HOST: ");
  Serial.println(server.hostHeader());

  Serial.print("URI: ");
  Serial.println(server.uri());

  sendBatteryPage();
  });




  server.on("/a0", []() {
    server.send(200, "text/plain", String(analogRead(A0)));
  });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
