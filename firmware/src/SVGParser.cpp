#include <Arduino.h>
#include <vector>
#include <cstdlib>
#include <cctype>

#include "SVGParser.h"

static bool readAttribute(
    const String& tag,
    const char* name,
    float& value
)
{
    String key = String(name) + "=\"";

    int start = tag.indexOf(key);

    char quote = '"';

    if (start < 0) {
        key = String(name) + "='";
        start = tag.indexOf(key);
        quote = '\'';
    }

    if (start < 0) {
        return false;
    }

    start += key.length();

    int end = tag.indexOf(quote, start);

    if (end < 0) {
        return false;
    }

    value = tag.substring(start, end).toFloat();

    return true;
}
bool SVGParser::parseCircle(
    const String& tag,
    std::vector<VectorPoint>& points
)
{
    float cx;
    float cy;
    float r;

    if (!readAttribute(tag, "cx", cx)) {
        cx = 0;
    }

    if (!readAttribute(tag, "cy", cy)) {
        cy = 0;
    }

    if (!readAttribute(tag, "r", r)) {
        Serial.println("Circle missing radius.");
        return false;
    }

    // More segments = smoother circle.
    constexpr int segments = 120;

    for (int i = 0; i <= segments; i++) {

        float angle =
            ((float)i / segments)
            * 2.0f
            * PI;

        float x =
            cx +
            cosf(angle) * r;

        float y =
            cy +
            sinf(angle) * r;

        points.push_back({
            x,
            y,
            i == 0
        });
    }

    return true;
}
static void skipSeparators(const char*& p)
{
    while (*p) {
        if (
            *p == ' '  ||
            *p == '\t' ||
            *p == '\r' ||
            *p == '\n' ||
            *p == ','
        ) {
            p++;
        } else {
            break;
        }
    }
}

static bool readNumber(const char*& p, float& value)
{
    skipSeparators(p);

    if (!*p) {
        return false;
    }

    char* end = nullptr;

    value = strtof(p, &end);

    if (end == p) {
        return false;
    }

    p = end;

    return true;
}

bool SVGParser::parse(
    const String& svg,
    std::vector<VectorPoint>& points
)
{
    points.clear();
    // Avoid lots of repeated heap reallocations
    points.reserve(4000);
    int position = 0;

    // ========================================================
    // Parse <path>
    // ========================================================

    while (true) {

        int pathStart = svg.indexOf("<path", position);

        if (pathStart < 0) {
            break;
        }

        int pathEnd = svg.indexOf('>', pathStart);

        if (pathEnd < 0) {
            break;
        }

        String pathTag = svg.substring(
            pathStart,
            pathEnd + 1
        );

        int dStart = pathTag.indexOf("d=\"");
        char quote = '"';

        if (dStart < 0) {
            dStart = pathTag.indexOf("d='");
            quote = '\'';
        }

        if (dStart >= 0) {

            dStart += 3;

            int dEnd = pathTag.indexOf(
                quote,
                dStart
            );

            if (dEnd > dStart) {

                String pathData = pathTag.substring(
                    dStart,
                    dEnd
                );
                Serial.println("PATH:");
                Serial.println(pathData);
                if (!parsePath(pathData, points)) {
                    Serial.println("Failed to parse <path>");
                    return false;
                }
            }
        }

        position = pathEnd + 1;
    }


    // ========================================================
    // Parse <circle>
    // ========================================================

    position = 0;

    while (true) {

        int circleStart = svg.indexOf(
            "<circle",
            position
        );

        if (circleStart < 0) {
            break;
        }

        int circleEnd = svg.indexOf(
            '>',
            circleStart
        );

        if (circleEnd < 0) {
            break;
        }

        String circleTag = svg.substring(
            circleStart,
            circleEnd + 1
        );

        if (!parseCircle(
            circleTag,
            points
        )) {
            Serial.println("Failed to parse <circle>");
        }

        position = circleEnd + 1;
    }


    // ========================================================
    // Result
    // ========================================================

    Serial.print("Parsed points: ");
    Serial.println(points.size());

    if (points.empty()) {
        Serial.println("No supported SVG elements found.");
        return false;
    }

    return true;
}

// ============================================================
// SVG elliptical arc helper
// Converts A/a commands into line segments
// ============================================================

static void addArc(
    std::vector<VectorPoint>& points,
    float x1,
    float y1,
    float rx,
    float ry,
    float xAxisRotation,
    bool largeArcFlag,
    bool sweepFlag,
    float x2,
    float y2
)
{
    // SVG spec: negative radii are treated as positive.
    rx = fabsf(rx);
    ry = fabsf(ry);

    // Degenerate arc becomes a line.
    if (rx == 0.0f || ry == 0.0f) {
        points.push_back({
            x2,
            y2,
            false
        });
        return;
    }

    // Same start/end means there is nothing to draw.
    if (fabsf(x1 - x2) < 0.00001f &&
        fabsf(y1 - y2) < 0.00001f)
    {
        return;
    }

    const float PI_F = 3.14159265358979323846f;

    float phi =
        xAxisRotation *
        PI_F /
        180.0f;

    float cosPhi = cosf(phi);
    float sinPhi = sinf(phi);

    // --------------------------------------------------------
    // Transform to arc coordinate system
    // --------------------------------------------------------

    float dx =
        (x1 - x2) * 0.5f;

    float dy =
        (y1 - y2) * 0.5f;

    float x1Prime =
        cosPhi * dx +
        sinPhi * dy;

    float y1Prime =
        -sinPhi * dx +
        cosPhi * dy;

    // --------------------------------------------------------
    // Ensure radii are large enough
    // --------------------------------------------------------

    float rx2 = rx * rx;
    float ry2 = ry * ry;

    float x1p2 =
        x1Prime * x1Prime;

    float y1p2 =
        y1Prime * y1Prime;

    float lambda =
        x1p2 / rx2 +
        y1p2 / ry2;

    if (lambda > 1.0f) {

        float scale =
            sqrtf(lambda);

        rx *= scale;
        ry *= scale;

        rx2 = rx * rx;
        ry2 = ry * ry;
    }

    // --------------------------------------------------------
    // Find center
    // --------------------------------------------------------

    float numerator =
        rx2 * ry2 -
        rx2 * y1p2 -
        ry2 * x1p2;

    float denominator =
        rx2 * y1p2 +
        ry2 * x1p2;

    float factor = 0.0f;

    if (denominator > 0.0f) {

        float ratio =
            numerator /
            denominator;

        if (ratio < 0.0f)
            ratio = 0.0f;

        factor = sqrtf(ratio);
    }

    if (largeArcFlag == sweepFlag) {
        factor = -factor;
    }

    float cxPrime =
        factor *
        ((rx * y1Prime) / ry);

    float cyPrime =
        factor *
        (-(ry * x1Prime) / rx);

    // Transform center back.
    float cx =
        cosPhi * cxPrime -
        sinPhi * cyPrime +
        (x1 + x2) * 0.5f;

    float cy =
        sinPhi * cxPrime +
        cosPhi * cyPrime +
        (y1 + y2) * 0.5f;

    // --------------------------------------------------------
    // Angle helper
    // --------------------------------------------------------

    auto vectorAngle = [](
        float ux,
        float uy,
        float vx,
        float vy
    ) -> float
    {
        float dot =
            ux * vx +
            uy * vy;

        float len =
            sqrtf(
                (ux * ux + uy * uy) *
                (vx * vx + vy * vy)
            );

        if (len <= 0.0f)
            return 0.0f;

        float value =
            dot / len;

        if (value < -1.0f)
            value = -1.0f;

        if (value > 1.0f)
            value = 1.0f;

        float angle =
            acosf(value);

        float cross =
            ux * vy -
            uy * vx;

        if (cross < 0.0f)
            angle = -angle;

        return angle;
    };

    // --------------------------------------------------------
    // Start and sweep angles
    // --------------------------------------------------------

    float ux =
        (x1Prime - cxPrime) / rx;

    float uy =
        (y1Prime - cyPrime) / ry;

    float vx =
        (-x1Prime - cxPrime) / rx;

    float vy =
        (-y1Prime - cyPrime) / ry;

    float theta1 =
        vectorAngle(
            1.0f,
            0.0f,
            ux,
            uy
        );

    float deltaTheta =
        vectorAngle(
            ux,
            uy,
            vx,
            vy
        );

    if (!sweepFlag &&
        deltaTheta > 0.0f)
    {
        deltaTheta -=
            2.0f * PI_F;
    }

    if (sweepFlag &&
        deltaTheta < 0.0f)
    {
        deltaTheta +=
            2.0f * PI_F;
    }

    // --------------------------------------------------------
    // Flatten arc
    // --------------------------------------------------------

    int segments =
        (int)(
            fabsf(deltaTheta) /
            (PI_F / 24.0f)
        );

    if (segments < 8)
        segments = 8;

    if (segments > 128)
        segments = 128;

    for (int i = 1; i <= segments; i++) {

        float t =
            (float)i /
            (float)segments;

        float angle =
            theta1 +
            deltaTheta * t;

        float cosA =
            cosf(angle);

        float sinA =
            sinf(angle);

        float x =
            cx +
            cosPhi * rx * cosA -
            sinPhi * ry * sinA;

        float y =
            cy +
            sinPhi * rx * cosA +
            cosPhi * ry * sinA;

        points.push_back({
            x,
            y,
            false
        });
    }
}


// ============================================================
// Parse SVG <path d="...">
// Supports every standard SVG path command:
//
// M/m
// L/l
// H/h
// V/v
// C/c
// S/s
// Q/q
// T/t
// A/a
// Z/z
// ============================================================

bool SVGParser::parsePath(
    const String& pathData,
    std::vector<VectorPoint>& points
)
{
    const char* p =
        pathData.c_str();

    char command = 0;
    char previousCommand = 0;

    float currentX = 0.0f;
    float currentY = 0.0f;

    float startX = 0.0f;
    float startY = 0.0f;

    // Previous cubic control point
    float cubicControlX = 0.0f;
    float cubicControlY = 0.0f;

    // Previous quadratic control point
    float quadControlX = 0.0f;
    float quadControlY = 0.0f;


    // ========================================================
    // Cubic Bézier generator
    // ========================================================

    auto cubicBezier =
    [&](float x0,
        float y0,
        float x1,
        float y1,
        float x2,
        float y2,
        float x3,
        float y3)
{
    constexpr int segments = 8;

    for (int i = 1; i <= segments; i++) {

        float t =
            (float)i /
            (float)segments;

        float u = 1.0f - t;

        float x =
            u*u*u*x0 +
            3.0f*u*u*t*x1 +
            3.0f*u*t*t*x2 +
            t*t*t*x3;

        float y =
            u*u*u*y0 +
            3.0f*u*u*t*y1 +
            3.0f*u*t*t*y2 +
            t*t*t*y3;

        points.push_back({
            x,
            y,
            false
        });
    }
};


    // ========================================================
    // Quadratic Bézier generator
    // ========================================================

    auto quadraticBezier =
    [&](float x0,
        float y0,
        float x1,
        float y1,
        float x2,
        float y2)
{
    constexpr int segments = 8;

    for (int i = 1; i <= segments; i++) {

        float t =
            (float)i /
            (float)segments;

        float u = 1.0f - t;

        float x =
            u*u*x0 +
            2.0f*u*t*x1 +
            t*t*x2;

        float y =
            u*u*y0 +
            2.0f*u*t*y1 +
            t*t*y2;

        points.push_back({
            x,
            y,
            false
        });
    }
};


    // ========================================================
    // Parse
    // ========================================================

    while (*p) {

        skipSeparators(p);

        if (!*p)
            break;


        // ----------------------------------------------------
        // Read command
        // ----------------------------------------------------

        if (isalpha(
            (unsigned char)*p
        )) {
            command = *p;
            p++;
        }

        if (command == 0) {
            Serial.println(
                "SVG path missing command."
            );
            return false;
        }


        switch (command) {

            // =================================================
            // M - absolute move
            // =================================================

            case 'M':
            {
                float x, y;

                if (!readNumber(p, x) ||
                    !readNumber(p, y))
                {
                    return false;
                }

                currentX = x;
                currentY = y;

                startX = x;
                startY = y;

                points.push_back({
                    x,
                    y,
                    true
                });

                previousCommand = 'M';

                // Remaining pairs are implicit L
                command = 'L';

                break;
            }


            // =================================================
            // m - relative move
            // =================================================

            case 'm':
            {
                float dx, dy;

                if (!readNumber(p, dx) ||
                    !readNumber(p, dy))
                {
                    return false;
                }

                currentX += dx;
                currentY += dy;

                startX = currentX;
                startY = currentY;

                points.push_back({
                    currentX,
                    currentY,
                    true
                });

                previousCommand = 'm';

                // Remaining pairs are implicit l
                command = 'l';

                break;
            }


            // =================================================
            // L - absolute line
            // =================================================

            case 'L':
            {
                float x, y;

                if (!readNumber(p, x) ||
                    !readNumber(p, y))
                {
                    return false;
                }

                currentX = x;
                currentY = y;

                points.push_back({
                    currentX,
                    currentY,
                    false
                });

                previousCommand = 'L';

                break;
            }


            // =================================================
            // l - relative line
            // =================================================

            case 'l':
            {
                float dx, dy;

                if (!readNumber(p, dx) ||
                    !readNumber(p, dy))
                {
                    return false;
                }

                currentX += dx;
                currentY += dy;

                points.push_back({
                    currentX,
                    currentY,
                    false
                });

                previousCommand = 'l';

                break;
            }


            // =================================================
            // H - absolute horizontal
            // =================================================

            case 'H':
            {
                float x;

                if (!readNumber(p, x))
                    return false;

                currentX = x;

                points.push_back({
                    currentX,
                    currentY,
                    false
                });

                previousCommand = 'H';

                break;
            }


            // =================================================
            // h - relative horizontal
            // =================================================

            case 'h':
            {
                float dx;

                if (!readNumber(p, dx))
                    return false;

                currentX += dx;

                points.push_back({
                    currentX,
                    currentY,
                    false
                });

                previousCommand = 'h';

                break;
            }


            // =================================================
            // V - absolute vertical
            // =================================================

            case 'V':
            {
                float y;

                if (!readNumber(p, y))
                    return false;

                currentY = y;

                points.push_back({
                    currentX,
                    currentY,
                    false
                });

                previousCommand = 'V';

                break;
            }


            // =================================================
            // v - relative vertical
            // =================================================

            case 'v':
            {
                float dy;

                if (!readNumber(p, dy))
                    return false;

                currentY += dy;

                points.push_back({
                    currentX,
                    currentY,
                    false
                });

                previousCommand = 'v';

                break;
            }


            // =================================================
            // C - absolute cubic Bézier
            // =================================================

            case 'C':
            {
                float x1, y1;
                float x2, y2;
                float x3, y3;

                if (!readNumber(p, x1) ||
                    !readNumber(p, y1) ||
                    !readNumber(p, x2) ||
                    !readNumber(p, y2) ||
                    !readNumber(p, x3) ||
                    !readNumber(p, y3))
                {
                    return false;
                }

                cubicBezier(
                    currentX,
                    currentY,
                    x1,
                    y1,
                    x2,
                    y2,
                    x3,
                    y3
                );

                cubicControlX = x2;
                cubicControlY = y2;

                currentX = x3;
                currentY = y3;

                previousCommand = 'C';

                break;
            }


            // =================================================
            // c - relative cubic Bézier
            // =================================================

            case 'c':
            {
                float dx1, dy1;
                float dx2, dy2;
                float dx3, dy3;

                if (!readNumber(p, dx1) ||
                    !readNumber(p, dy1) ||
                    !readNumber(p, dx2) ||
                    !readNumber(p, dy2) ||
                    !readNumber(p, dx3) ||
                    !readNumber(p, dy3))
                {
                    return false;
                }

                float x1 =
                    currentX + dx1;

                float y1 =
                    currentY + dy1;

                float x2 =
                    currentX + dx2;

                float y2 =
                    currentY + dy2;

                float x3 =
                    currentX + dx3;

                float y3 =
                    currentY + dy3;

                cubicBezier(
                    currentX,
                    currentY,
                    x1,
                    y1,
                    x2,
                    y2,
                    x3,
                    y3
                );

                cubicControlX = x2;
                cubicControlY = y2;

                currentX = x3;
                currentY = y3;

                previousCommand = 'c';

                break;
            }


            // =================================================
            // S - absolute smooth cubic Bézier
            // =================================================

            case 'S':
            {
                float x2, y2;
                float x3, y3;

                if (!readNumber(p, x2) ||
                    !readNumber(p, y2) ||
                    !readNumber(p, x3) ||
                    !readNumber(p, y3))
                {
                    return false;
                }

                float x1 = currentX;
                float y1 = currentY;

                if (
                    previousCommand == 'C' ||
                    previousCommand == 'c' ||
                    previousCommand == 'S' ||
                    previousCommand == 's'
                ) {
                    x1 =
                        2.0f * currentX -
                        cubicControlX;

                    y1 =
                        2.0f * currentY -
                        cubicControlY;
                }

                cubicBezier(
                    currentX,
                    currentY,
                    x1,
                    y1,
                    x2,
                    y2,
                    x3,
                    y3
                );

                cubicControlX = x2;
                cubicControlY = y2;

                currentX = x3;
                currentY = y3;

                previousCommand = 'S';

                break;
            }


            // =================================================
            // s - relative smooth cubic Bézier
            // =================================================

            case 's':
            {
                float dx2, dy2;
                float dx3, dy3;

                if (!readNumber(p, dx2) ||
                    !readNumber(p, dy2) ||
                    !readNumber(p, dx3) ||
                    !readNumber(p, dy3))
                {
                    return false;
                }

                float x1 = currentX;
                float y1 = currentY;

                if (
                    previousCommand == 'C' ||
                    previousCommand == 'c' ||
                    previousCommand == 'S' ||
                    previousCommand == 's'
                ) {
                    x1 =
                        2.0f * currentX -
                        cubicControlX;

                    y1 =
                        2.0f * currentY -
                        cubicControlY;
                }

                float x2 =
                    currentX + dx2;

                float y2 =
                    currentY + dy2;

                float x3 =
                    currentX + dx3;

                float y3 =
                    currentY + dy3;

                cubicBezier(
                    currentX,
                    currentY,
                    x1,
                    y1,
                    x2,
                    y2,
                    x3,
                    y3
                );

                cubicControlX = x2;
                cubicControlY = y2;

                currentX = x3;
                currentY = y3;

                previousCommand = 's';

                break;
            }


            // =================================================
            // Q - absolute quadratic Bézier
            // =================================================

            case 'Q':
            {
                float x1, y1;
                float x2, y2;

                if (!readNumber(p, x1) ||
                    !readNumber(p, y1) ||
                    !readNumber(p, x2) ||
                    !readNumber(p, y2))
                {
                    return false;
                }

                quadraticBezier(
                    currentX,
                    currentY,
                    x1,
                    y1,
                    x2,
                    y2
                );

                quadControlX = x1;
                quadControlY = y1;

                currentX = x2;
                currentY = y2;

                previousCommand = 'Q';

                break;
            }


            // =================================================
            // q - relative quadratic Bézier
            // =================================================

            case 'q':
            {
                float dx1, dy1;
                float dx2, dy2;

                if (!readNumber(p, dx1) ||
                    !readNumber(p, dy1) ||
                    !readNumber(p, dx2) ||
                    !readNumber(p, dy2))
                {
                    return false;
                }

                float x1 =
                    currentX + dx1;

                float y1 =
                    currentY + dy1;

                float x2 =
                    currentX + dx2;

                float y2 =
                    currentY + dy2;

                quadraticBezier(
                    currentX,
                    currentY,
                    x1,
                    y1,
                    x2,
                    y2
                );

                quadControlX = x1;
                quadControlY = y1;

                currentX = x2;
                currentY = y2;

                previousCommand = 'q';

                break;
            }


            // =================================================
            // T - absolute smooth quadratic
            // =================================================

            case 'T':
            {
                float x, y;

                if (!readNumber(p, x) ||
                    !readNumber(p, y))
                {
                    return false;
                }

                float controlX = currentX;
                float controlY = currentY;

                if (
                    previousCommand == 'Q' ||
                    previousCommand == 'q' ||
                    previousCommand == 'T' ||
                    previousCommand == 't'
                ) {
                    controlX =
                        2.0f * currentX -
                        quadControlX;

                    controlY =
                        2.0f * currentY -
                        quadControlY;
                }

                quadraticBezier(
                    currentX,
                    currentY,
                    controlX,
                    controlY,
                    x,
                    y
                );

                quadControlX = controlX;
                quadControlY = controlY;

                currentX = x;
                currentY = y;

                previousCommand = 'T';

                break;
            }


            // =================================================
            // t - relative smooth quadratic
            // =================================================

            case 't':
            {
                float dx, dy;

                if (!readNumber(p, dx) ||
                    !readNumber(p, dy))
                {
                    return false;
                }

                float controlX = currentX;
                float controlY = currentY;

                if (
                    previousCommand == 'Q' ||
                    previousCommand == 'q' ||
                    previousCommand == 'T' ||
                    previousCommand == 't'
                ) {
                    controlX =
                        2.0f * currentX -
                        quadControlX;

                    controlY =
                        2.0f * currentY -
                        quadControlY;
                }

                float x =
                    currentX + dx;

                float y =
                    currentY + dy;

                quadraticBezier(
                    currentX,
                    currentY,
                    controlX,
                    controlY,
                    x,
                    y
                );

                quadControlX = controlX;
                quadControlY = controlY;

                currentX = x;
                currentY = y;

                previousCommand = 't';

                break;
            }


            // =================================================
            // A - absolute elliptical arc
            // =================================================

            case 'A':
            {
                float rx, ry;
                float rotation;
                float largeArc;
                float sweep;
                float x, y;

                if (!readNumber(p, rx) ||
                    !readNumber(p, ry) ||
                    !readNumber(p, rotation) ||
                    !readNumber(p, largeArc) ||
                    !readNumber(p, sweep) ||
                    !readNumber(p, x) ||
                    !readNumber(p, y))
                {
                    return false;
                }

                addArc(
                    points,
                    currentX,
                    currentY,
                    rx,
                    ry,
                    rotation,
                    largeArc != 0.0f,
                    sweep != 0.0f,
                    x,
                    y
                );

                currentX = x;
                currentY = y;

                previousCommand = 'A';

                break;
            }


            // =================================================
            // a - relative elliptical arc
            // =================================================

            case 'a':
            {
                float rx, ry;
                float rotation;
                float largeArc;
                float sweep;
                float dx, dy;

                if (!readNumber(p, rx) ||
                    !readNumber(p, ry) ||
                    !readNumber(p, rotation) ||
                    !readNumber(p, largeArc) ||
                    !readNumber(p, sweep) ||
                    !readNumber(p, dx) ||
                    !readNumber(p, dy))
                {
                    return false;
                }

                float x =
                    currentX + dx;

                float y =
                    currentY + dy;

                addArc(
                    points,
                    currentX,
                    currentY,
                    rx,
                    ry,
                    rotation,
                    largeArc != 0.0f,
                    sweep != 0.0f,
                    x,
                    y
                );

                currentX = x;
                currentY = y;

                previousCommand = 'a';

                break;
            }


            // =================================================
            // Z/z - close path
            // =================================================

            case 'Z':
            case 'z':
            {
                points.push_back({
                    startX,
                    startY,
                    false
                });

                currentX = startX;
                currentY = startY;

                previousCommand = command;

                // Prevent trying to reuse Z with numbers
                command = 0;

                break;
            }


            // =================================================
            // Unknown
            // =================================================

            default:
            {
                Serial.print(
                    "Unsupported SVG path command: "
                );

                Serial.println(command);

                return false;
            }
        }
    }

    return true;
}