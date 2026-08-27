#include <Arduino.h>

#include "Config.h"
#include "WiFiManager.h"
#include "SVGFetcher.h"
#include "SVGParser.h"
#include "ScopeRenderer.h"
#include "VectorText.h"

#include <vector>

std::vector<VectorPoint> vectorImage;


// ============================================================
// Show status text on the oscilloscope
// ============================================================

void showStatus(const String& text, unsigned long durationMs = 1000)
{
    std::vector<VectorPoint> statusImage =
        VectorText::create(text);

    ScopeRenderer::setImage(statusImage);

    unsigned long start = millis();

    while (millis() - start < durationMs) {
        ScopeRenderer::draw();
    }
}


// ============================================================
// Show an error forever
// ============================================================

void showError(const String& text)
{
    std::vector<VectorPoint> errorImage =
        VectorText::create(text);

    ScopeRenderer::setImage(errorImage);

    while (true) {
        ScopeRenderer::draw();
    }
}


// ============================================================
// Setup
// ============================================================

void setup()
{
    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println("ESP32 Scope Vector Display");

    ScopeRenderer::begin();


    // --------------------------------------------------------
    // Boot
    // --------------------------------------------------------

    showStatus("BOOT", 800);


    // --------------------------------------------------------
    // Connect Wi-Fi
    // --------------------------------------------------------

    showStatus("WIFI", 800);

    if (!WiFiManager::connect()) {

        Serial.println("Stopping: WiFi failed.");

        showError("WIFI ERROR");
    }


    showStatus("WIFI OK", 700);


    // --------------------------------------------------------
    // Download SVG
    // --------------------------------------------------------

    showStatus("DOWNLOAD", 800);

    String svg;

    if (!SVGFetcher::fetch(
        SVG_URL,
        svg
    )) {

        Serial.println(
            "Stopping: SVG download failed."
        );

        showError("HTTP ERROR");
    }


    // --------------------------------------------------------
    // Parse SVG
    // --------------------------------------------------------

    showStatus("PARSING", 800);

    if (!SVGParser::parse(
        svg,
        vectorImage
    )) {

        Serial.println(
            "Stopping: SVG parsing failed."
        );

        showError("SVG ERROR");
    }


    // --------------------------------------------------------
    // Ready
    // --------------------------------------------------------

    showStatus("READY", 800);


    // --------------------------------------------------------
    // Load SVG into renderer
    // --------------------------------------------------------

    ScopeRenderer::setImage(
        vectorImage
    );

    Serial.println("Ready.");
}


// ============================================================
// Main loop
// ============================================================

void loop()
{
    ScopeRenderer::draw();
}