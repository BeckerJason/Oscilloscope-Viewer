#ifndef PICTUREDATA_H
#define PICTUREDATA_H
#include <stdint.h>
struct Point {
    uint8_t x;
    uint8_t y;
    bool move;
};

const Point image[] = {

    // =========================
    // LINE 1: I LOVE
    // Y = 30..85
    // =========================

    // I
    {25, 30, true},
    {55, 30, false},

    {40, 30, true},
    {40, 85, false},

    {25, 85, true},
    {55, 85, false},


    // L
    {70, 30, true},
    {70, 85, false},
    {100, 85, false},


    // O
    {115, 40, true},
    {115, 75, false},
    {125, 85, false},
    {145, 85, false},
    {155, 75, false},
    {155, 40, false},
    {145, 30, false},
    {125, 30, false},
    {115, 40, false},


    // V
    {165, 30, true},
    {185, 85, false},
    {205, 30, false},


    // E
    {245, 30, true},
    {215, 30, false},
    {215, 85, false},
    {245, 85, false},

    {215, 57, true},
    {240, 57, false},


    // =========================
    // LINE 2: YOU
    // Y = 100..155
    // =========================

    // Y
    {65, 100, true},
    {85, 127, false},
    {105, 100, false},

    {85, 127, true},
    {85, 155, false},


    // O
    {120, 110, true},
    {120, 145, false},
    {130, 155, false},
    {150, 155, false},
    {160, 145, false},
    {160, 110, false},
    {150, 100, false},
    {130, 100, false},
    {120, 110, false},


    // U
    {175, 100, true},
    {175, 145, false},
    {185, 155, false},
    {205, 155, false},
    {215, 145, false},
    {215, 100, false},


    // =========================
    // LINE 3: AMANDA
    // Y = 175..230
    // =========================

    // A
    {10, 230, true},
    {25, 175, false},
    {40, 230, false},

    {17, 205, true},
    {33, 205, false},


    // M
    {50, 230, true},
    {50, 175, false},
    {65, 205, false},
    {80, 175, false},
    {80, 230, false},


    // A
    {90, 230, true},
    {105, 175, false},
    {120, 230, false},

    {97, 205, true},
    {113, 205, false},


    // N
    {130, 230, true},
    {130, 175, false},
    {160, 230, false},
    {160, 175, false},


    // D
    {170, 175, true},
    {170, 230, false},
    {190, 230, false},
    {205, 215, false},
    {205, 190, false},
    {190, 175, false},
    {170, 175, false},


    // A
    {215, 230, true},
    {230, 175, false},
    {245, 230, false},

    {222, 205, true},
    {238, 205, false}
};

constexpr size_t pointCount = sizeof(image) / sizeof(image[0]);
#endif