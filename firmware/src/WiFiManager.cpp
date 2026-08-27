#include <Arduino.h>
#include <WiFi.h>

#include "WiFiManager.h"
#include "Secrets.h"

bool WiFiManager::connect()
{
    Serial.print("Connecting to WiFi");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");

        // Give up after 15 seconds
        if (millis() - start > 15000) {
            Serial.println();
            Serial.println("WiFi connection failed.");
            return false;
        }
    }

    Serial.println();
    Serial.println("WiFi connected.");

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    return true;
}