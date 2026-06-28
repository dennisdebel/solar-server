#include <ESP8266WiFi.h>

// Measuring voltages means measuring PARALLEL that means we need to make a voltage divider, the Wemos D1 Mini (clone) already has one built in, so we only need one 100k resistor! Wire like the diagram below:
//
//  Battery +
//     |
//    100k
//     |
//    A0   (Wemos D1 Mini)
//     |
//  (internal divider)
//     |
//  Battery -/gnd
//
// However, we need a calibration factor (e.g. 4.17 instead of 4.20) because resistor tolerances and ADC accuracy vary from board to board. To get the calibration factor, measure the actual battery voltage and divide it by the voltage measured by the Wemos D1 Mini


extern "C" {
  #include "user_interface.h"
}

// =========================
// DEBUG SETTINGS
// =========================

const bool DEBUG_MODE = true; // enable test mode that simulates battery levels, see option below (true/false)
const float DEBUG_BATTERY = 4; // for real battery value use: -1, to simulate states, use for example 3.9
const uint64_t DEBUG_SLEEP_TIME = 10e6;

// =========================
// BATTERY SETTINGS
// =========================

const float ADC_CAL = 0.975; // calibrate analog voltage measuring via A0 (battery voltage/)
const int DISPLAY_TIME_MS = 5000;

// =========================
// RTC MEMORY
// =========================

struct RTCData {
    uint32_t magic;
    float history[10];
    uint8_t index;
    uint8_t count;
};

RTCData rtc;

const uint32_t RTC_MAGIC = 0xDEADBEEF;

// =========================
// TREND DETECTION
// =========================

String batteryTrend(float currentVoltage)
{
    system_rtc_mem_read(65, &rtc, sizeof(rtc));

    // FIRST BOOT / RESET
    if (rtc.magic != RTC_MAGIC)
    {
        rtc.magic = RTC_MAGIC;
        rtc.index = 0;
        rtc.count = 0;

        for (int i = 0; i < 10; i++)
            rtc.history[i] = currentVoltage;

        system_rtc_mem_write(65, &rtc, sizeof(rtc));

        return "WARM"; // Warming up mode (take 10 voltage measure to determain trend)
    }

    // store new reading first
    rtc.history[rtc.index] = currentVoltage;
    rtc.index = (rtc.index + 1) % 10;

    if (rtc.count < 10)
        rtc.count++;

    system_rtc_mem_write(65, &rtc, sizeof(rtc));

    // 🔥 HARD EARLY EXIT (THIS IS THE FIX)
    if (rtc.count < 10)
        return "WARM";

    // compute average
    float avg = 0;
    for (int i = 0; i < 10; i++)
        avg += rtc.history[i];

    avg /= 10.0;

    float delta = currentVoltage - avg;

    if (delta > 0.01) return "CHG";
    if (delta < -0.01) return "DIS";

    return "OK";
}

// =========================
// BATTERY READING
// =========================

float readBatteryVoltage()
{
    long sum = 0;

    for (int i = 0; i < 16; i++)
    {
        sum += analogRead(A0);
        delay(2);
    }

    float raw = sum / 16.0;

    return raw * (4.2 / 1023.0) * ADC_CAL;
}

int batteryPercent(float v) // Mapping for battery percentages (Lipo battery curves are not linear).
{
    if (v >= 4.20) return 100;
    if (v >= 4.10) return 90;
    if (v >= 4.00) return 80;
    if (v >= 3.92) return 70;
    if (v >= 3.85) return 60;
    if (v >= 3.80) return 50;
    if (v >= 3.75) return 40;
    if (v >= 3.70) return 30;
    if (v >= 3.65) return 20;
    if (v >= 3.50) return 10;
    return 0;
}

// =========================
// MAIN
// =========================

void setup()
{
    delay(100);

    float batt;

    if (DEBUG_BATTERY > 0)
        batt = DEBUG_BATTERY;
    else
        batt = readBatteryVoltage();

    int pct = batteryPercent(batt);
    String trend = batteryTrend(batt);

    uint64_t sleepTime;

    if (pct > 80)
        sleepTime = 10e6;
    else if (pct > 50)
        sleepTime = 30e6;
    else if (pct > 25)
        sleepTime = 300e6;
    else
        sleepTime = 600e6;

    if (trend == "CHG")
        sleepTime /= 2;
    else if (trend == "DIS")
        sleepTime *= 2;

    if (DEBUG_MODE)
        sleepTime = DEBUG_SLEEP_TIME;

    char ssid[64];

    snprintf(
        ssid,
        sizeof(ssid),
        "\xF0\x9F\x8C\x9E %d%% %.2fV %s", 
        pct,
        batt,
        trend.c_str()
    );

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);

    delay(DISPLAY_TIME_MS);

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    ESP.deepSleep(sleepTime);
}

void loop()
{
}
