#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "SVGFetcher.h"

bool SVGFetcher::fetch(const char* url, String& output)
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Cannot fetch SVG: WiFi not connected.");
        return false;
    }

    HTTPClient http;

    Serial.print("Downloading SVG: ");
    Serial.println(url);

    if (!http.begin(url)) {
        Serial.println("HTTP begin failed.");
        return false;
    }

    int responseCode = http.GET();

    if (responseCode != HTTP_CODE_OK) {
        Serial.print("HTTP error: ");
        Serial.println(responseCode);

        http.end();
        return false;
    }

    output = http.getString();

    Serial.print("Downloaded ");
    Serial.print(output.length());
    Serial.println(" bytes.");

    http.end();

    return true;
}