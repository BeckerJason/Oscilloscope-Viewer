#pragma once

#include <Arduino.h>
#include <vector>

#include "VectorPoint.h"

class SVGParser {
public:
    static bool parse(
        const String& svg,
        std::vector<VectorPoint>& points
    );


private:
    static bool parsePath(
        const String& pathData,
        std::vector<VectorPoint>& points
    );
        static bool parseCircle(
    const String& tag,
    std::vector<VectorPoint>& points
    );
};