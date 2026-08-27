#pragma once

#include <Arduino.h>
#include <vector>

#include "VectorPoint.h"

class VectorText {
public:
    // Converts text into VectorPoints.
    //
    // x, y       = starting position
    // scale      = character size
    // spacing    = spacing between characters
    //
    // Returns a vector that can be passed directly to
    // ScopeRenderer::setImage().
    static std::vector<VectorPoint> create(
        const String& text,
        float x = 0.0f,
        float y = 0.0f,
        float scale = 10.0f,
        float spacing = 2.0f
    );

private:
    static void addCharacter(
        char c,
        float x,
        float y,
        float scale,
        std::vector<VectorPoint>& output
    );

    static void line(
        std::vector<VectorPoint>& output,
        float x1,
        float y1,
        float x2,
        float y2
    );
};