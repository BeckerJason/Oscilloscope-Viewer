#pragma once

#include <Arduino.h>

class SVGFetcher {
public:
    static bool fetch(const char* url, String& output);
};