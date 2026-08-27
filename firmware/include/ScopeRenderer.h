#pragma once

#include <vector>

#include "VectorPoint.h"

class ScopeRenderer {
public:
    static void begin();

    static void setImage(
        const std::vector<VectorPoint>& points
    );

    static void draw();

private:
    static void drawLine(
        float x0,
        float y0,
        float x1,
        float y1
    );

    static void outputPoint(
        float x,
        float y
    );

    static float mapX(float x);
    static float mapY(float y);

    static std::vector<VectorPoint> image;

    static float minX;
    static float maxX;
    static float minY;
    static float maxY;
};
