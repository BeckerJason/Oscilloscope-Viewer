#include <Arduino.h>
#include "PictureData.h"
#define X_OUT 25
#define Y_OUT 26

void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, int steps)
{
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;

        uint8_t x = x0 + (x1 - x0) * t;
        uint8_t y = y0 + (y1 - y0) * t;

        dacWrite(X_OUT, x);
        dacWrite(Y_OUT, 255 - y);

        delayMicroseconds(5);
    }
}

void setup() {
}

void loop() {
    for (size_t i = 0; i < pointCount; i++) {

        if (image[i].move) {
            // Jump to beginning of new shape
            dacWrite(X_OUT, image[i].x);
            dacWrite(Y_OUT, 255 - image[i].y);
            continue;
        }

        // Draw from previous point to current point
        drawLine(
            image[i - 1].x,
            image[i - 1].y,
            image[i].x,
            image[i].y,
            20
        );
    }
}