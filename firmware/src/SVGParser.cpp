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

bool SVGParser::parsePath(
    const String& pathData,
    std::vector<VectorPoint>& points
)
{
    const char* p = pathData.c_str();

    char command = 0;

    float currentX = 0.0f;
    float currentY = 0.0f;

    float startX = 0.0f;
    float startY = 0.0f;

    while (*p) {

        skipSeparators(p);

        if (!*p)
            break;

        // New command
        if (isalpha((unsigned char)*p)) {
            command = *p;
            p++;
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
                    return false;

                currentX = x;
                currentY = y;

                startX = x;
                startY = y;

                points.push_back({
                    currentX,
                    currentY,
                    true
                });

                // More pairs after M are lines
                command = 'L';

                break;
            }


            // =================================================
            // m - relative move
            // =================================================

            case 'm':
            {
                float x, y;

                if (!readNumber(p, x) ||
                    !readNumber(p, y))
                    return false;

                currentX += x;
                currentY += y;

                startX = currentX;
                startY = currentY;

                points.push_back({
                    currentX,
                    currentY,
                    true
                });

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
                    return false;

                currentX = x;
                currentY = y;

                points.push_back({
                    currentX,
                    currentY,
                    false
                });

                break;
            }


            // =================================================
            // l - relative line
            // =================================================

            case 'l':
            {
                float x, y;

                if (!readNumber(p, x) ||
                    !readNumber(p, y))
                    return false;

                currentX += x;
                currentY += y;

                points.push_back({
                    currentX,
                    currentY,
                    false
                });

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

                break;
            }


            // =================================================
            // h - relative horizontal
            // =================================================

            case 'h':
            {
                float x;

                if (!readNumber(p, x))
                    return false;

                currentX += x;

                points.push_back({
                    currentX,
                    currentY,
                    false
                });

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

                break;
            }


            // =================================================
            // v - relative vertical
            // =================================================

            case 'v':
            {
                float y;

                if (!readNumber(p, y))
                    return false;

                currentY += y;

                points.push_back({
                    currentX,
                    currentY,
                    false
                });

                break;
            }


            // =================================================
            // C - absolute cubic Bezier
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

                float x0 = currentX;
                float y0 = currentY;

                constexpr int segments = 24;

                for (int i = 1; i <= segments; i++) {

                    float t =
                        (float)i / segments;

                    float u =
                        1.0f - t;

                    float x =
                        u*u*u*x0 +
                        3*u*u*t*x1 +
                        3*u*t*t*x2 +
                        t*t*t*x3;

                    float y =
                        u*u*u*y0 +
                        3*u*u*t*y1 +
                        3*u*t*t*y2 +
                        t*t*t*y3;

                    points.push_back({
                        x,
                        y,
                        false
                    });
                }

                currentX = x3;
                currentY = y3;

                break;
            }


            // =================================================
            // c - relative cubic Bezier
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

                float x0 = currentX;
                float y0 = currentY;

                float x1 = currentX + dx1;
                float y1 = currentY + dy1;

                float x2 = currentX + dx2;
                float y2 = currentY + dy2;

                float x3 = currentX + dx3;
                float y3 = currentY + dy3;

                constexpr int segments = 24;

                for (int i = 1; i <= segments; i++) {

                    float t =
                        (float)i / segments;

                    float u =
                        1.0f - t;

                    float x =
                        u*u*u*x0 +
                        3*u*u*t*x1 +
                        3*u*t*t*x2 +
                        t*t*t*x3;

                    float y =
                        u*u*u*y0 +
                        3*u*u*t*y1 +
                        3*u*t*t*y2 +
                        t*t*t*y3;

                    points.push_back({
                        x,
                        y,
                        false
                    });
                }

                currentX = x3;
                currentY = y3;

                break;
            }


            // =================================================
            // Q - absolute quadratic Bezier
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

                float x0 = currentX;
                float y0 = currentY;

                constexpr int segments = 24;

                for (int i = 1; i <= segments; i++) {

                    float t =
                        (float)i / segments;

                    float u =
                        1.0f - t;

                    float x =
                        u*u*x0 +
                        2*u*t*x1 +
                        t*t*x2;

                    float y =
                        u*u*y0 +
                        2*u*t*y1 +
                        t*t*y2;

                    points.push_back({
                        x,
                        y,
                        false
                    });
                }

                currentX = x2;
                currentY = y2;

                break;
            }


            // =================================================
            // q - relative quadratic Bezier
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

                float x0 = currentX;
                float y0 = currentY;

                float x1 = currentX + dx1;
                float y1 = currentY + dy1;

                float x2 = currentX + dx2;
                float y2 = currentY + dy2;

                constexpr int segments = 24;

                for (int i = 1; i <= segments; i++) {

                    float t =
                        (float)i / segments;

                    float u =
                        1.0f - t;

                    float x =
                        u*u*x0 +
                        2*u*t*x1 +
                        t*t*x2;

                    float y =
                        u*u*y0 +
                        2*u*t*y1 +
                        t*t*y2;

                    points.push_back({
                        x,
                        y,
                        false
                    });
                }

                currentX = x2;
                currentY = y2;

                break;
            }


            // =================================================
            // Close path
            // =================================================

            case 'Z':
            case 'z':
            {
                currentX = startX;
                currentY = startY;

                points.push_back({
                    currentX,
                    currentY,
                    false
                });

                command = 0;

                break;
            }


            // =================================================
            // Unsupported
            // =================================================

            default:
            {
                Serial.print(
                    "Unsupported SVG command: "
                );

                Serial.println(command);

                return false;
            }
        }
    }

    return true;
}