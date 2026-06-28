#include <Arduino.h>
#include <ESP8266WiFi.h>

#define SSID_NAME "_ALIVE-old"


void setup()
{


  // ---- start WiFi AP ----
  WiFi.persistent(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID_NAME);

  // optional: reduce power saving weirdness
  WiFi.setSleepMode(WIFI_NONE_SLEEP);


}

void loop()
{
  // never used
}
