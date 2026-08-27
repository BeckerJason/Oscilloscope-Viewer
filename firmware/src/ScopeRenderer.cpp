#include <Arduino.h>
#include <vector>
#include <math.h>

#include "ScopeRenderer.h"
#include "Config.h"

std::vector<VectorPoint> ScopeRenderer::image;

float ScopeRenderer::minX = 0;
float ScopeRenderer::maxX = 1;
float ScopeRenderer::minY = 0;
float ScopeRenderer::maxY = 1;

void ScopeRenderer::begin()
{
    pinMode(X_OUT, OUTPUT);
    pinMode(Y_OUT, OUTPUT);

    dacWrite(X_OUT, 128);
    dacWrite(Y_OUT, 128);
}

void ScopeRenderer::setImage(
    const std::vector<VectorPoint>& points
)
{
    image = points;

    if (image.empty())
        return;

    minX = image[0].x;
    maxX = image[0].x;

    minY = image[0].y;
    maxY = image[0].y;

    for (const auto& p : image) {

        if (p.x < minX)
            minX = p.x;

        if (p.x > maxX)
            maxX = p.x;

        if (p.y < minY)
            minY = p.y;

        if (p.y > maxY)
            maxY = p.y;
    }

    Serial.println("Image bounds:");

    Serial.print("X: ");
    Serial.print(minX);
    Serial.print(" -> ");
    Serial.println(maxX);

    Serial.print("Y: ");
    Serial.print(minY);
    Serial.print(" -> ");
    Serial.println(maxY);
}

float ScopeRenderer::mapX(float x)
{
    float width = maxX - minX;

    if (width <= 0)
        return 128;

    return (
        (x - minX) /
        width
    ) * 220.0f + 18.0f;
}

float ScopeRenderer::mapY(float y)
{
    float height = maxY - minY;

    if (height <= 0)
        return 128;

    float mapped =
        ((y - minY) / height)
        * 220.0f
        + 18.0f;

    // Flip SVG Y axis for scope
    return 255.0f - mapped;
}

void ScopeRenderer::outputPoint(
    float x,
    float y
)
{
    int dacX = constrain(
        (int)mapX(x),
        0,
        255
    );

    int dacY = constrain(
        (int)mapY(y),
        0,
        255
    );

    dacWrite(X_OUT, dacX);
    dacWrite(Y_OUT, dacY);
}

void ScopeRenderer::drawLine(
    float x0,
    float y0,
    float x1,
    float y1
)
{
    float mappedX0 = mapX(x0);
    float mappedY0 = mapY(y0);

    float mappedX1 = mapX(x1);
    float mappedY1 = mapY(y1);

    float dx = fabs(mappedX1 - mappedX0);
    float dy = fabs(mappedY1 - mappedY0);

    int steps = (int)max(dx, dy);

    if (steps < 1)
        steps = 1;

    for (int i = 0; i <= steps; i++) {

        float t =
            (float)i /
            (float)steps;

        float x =
            x0 +
            (x1 - x0) * t;

        float y =
            y0 +
            (y1 - y0) * t;

        outputPoint(x, y);

        delayMicroseconds(DRAW_DELAY_US);
    }
}

void ScopeRenderer::draw()
{
    if (image.empty())
        return;

    VectorPoint previous = image[0];

    outputPoint(
        previous.x,
        previous.y
    );

    for (size_t i = 1; i < image.size(); i++) {

        const VectorPoint& current = image[i];

        if (current.move) {

            // Fast jump between disconnected paths
            outputPoint(
                current.x,
                current.y
            );

        } else {

            drawLine(
                previous.x,
                previous.y,
                current.x,
                current.y
            );
        }

        previous = current;
    }
}