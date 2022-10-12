/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <map>
#include <array>
#include <apl/scenegraph/path.h>

#include "apl/primitives/point.h"
#include "apl/scenegraph/pathparser.h"

namespace apl {
namespace sg {


/**
 * Tokenizer class that walks through a string and returns single character AVG path commands
 * (such as "M" for "MoveTo") and floating point numbers.
 */
class Tokenizer
{
public:
    explicit Tokenizer(const char *pathData) : mPtr(pathData)
    {
        advanceToToken();
    }

    enum TT
    {
        CHARACTER, NUMBER, END
    };

    // Return the type of the next token
    TT peek()
    {
        if (isalpha(*mPtr))
            return CHARACTER;

        if (::isdigit(*mPtr) || *mPtr == '-' || *mPtr == '+' || *mPtr == '.')
            return NUMBER;

        return END;
    }

    /**
     * The next item should be a number. If it is, store it in "result", advance to the next token,
     * and return true.  If it is not, return false.
     * @param result Where to store the parsed number
     * @return True if we found a number
     */
    bool expectNumber(float& result)
    {
        if (peek() != NUMBER)
            return false;

        // TODO: This isn't exactly right because it accepts a hexidecimal values and other invalid SVG path data
        char *p;
        result = ::strtof(mPtr, &p);  // Note: can I pass &mPtr as the second argument?
        mPtr = p;
        advanceToToken();
        return true;
    }

    /**
     * The next item should be a single alphabetic character.  If it is, return it and advance to the
     * next token.  If it is not, return 0.
     * @return
     */
    bool expectChar(char& result)
    {
        if (::isalpha(*mPtr)) {
            result = *mPtr++;
            advanceToToken();
            return true;
        }

        return false;
    }

private:
    void advanceToToken()
    {
        mPtr += ::strspn(mPtr, ", \f\t\n\r");
    }

    const char *mPtr;
};

/**
 * Parser for AVG path data.
 */
class PathParser
{
public:
    explicit PathParser(const char *pathData)
        : mTokenizer(pathData) {}

    bool failed() {
        return mError || mTokenizer.peek() != Tokenizer::END;
    }

    bool isCharacter() {
        return !mError && mTokenizer.peek() == Tokenizer::CHARACTER;
    }

    bool isNumber() {
        return !mError && mTokenizer.peek() == Tokenizer::NUMBER;
    }

    char matchCharacter() {
        char result = 0;
        if (!mTokenizer.expectChar(result))
            mError = true;
        return result;
    }

    float matchNumber() {
        float result = 0.0;
        if (!mTokenizer.expectNumber(result))
            mError = true;

        return result;
    }

private:
    Tokenizer mTokenizer;
    bool mError = false;
};

std::shared_ptr<GeneralPath>
parsePathString(const std::string& path)
{
    std::vector<float> points;
    std::string commands;

    PathParser parser(path.c_str());

    // Last point
    float last_x = 0;
    float last_y = 0;

    // Last control point
    float control_x = 0;
    float control_y = 0;

    char lastCh = 0;
    bool isDrawn = false;   // Set to true once we find something that draws

    auto result = std::make_shared<GeneralPath>();

    while (parser.isCharacter()) {
        auto ch = parser.matchCharacter();
        switch (ch) {
            case 'M':   // Absolute MoveTo
                do {
                    last_x = parser.matchNumber();
                    last_y = parser.matchNumber();
                    points.emplace_back(last_x);
                    points.emplace_back(last_y);
                    commands.push_back('M');
                } while (parser.isNumber());
                break;

            case 'm':   // Relative MoveTo  -> convert to absolute move to
                do {
                    last_x += parser.matchNumber();
                    last_y += parser.matchNumber();
                    points.emplace_back(last_x);
                    points.emplace_back(last_y);
                    commands.push_back('M');
                } while (parser.isNumber());
                break;

            case 'L':   // Absolute LineTo
                do {
                    last_x = parser.matchNumber();
                    last_y = parser.matchNumber();
                    points.emplace_back(last_x);
                    points.emplace_back(last_y);
                    commands.push_back('L');
                    isDrawn = true;
                } while (parser.isNumber());
                break;

            case 'l':   // Relative LineTo -> convert to absolute line to
                do {
                    last_x += parser.matchNumber();
                    last_y += parser.matchNumber();
                    points.emplace_back(last_x);
                    points.emplace_back(last_y);
                    commands.push_back('L');
                    isDrawn = true;
                } while (parser.isNumber());
                break;

            case 'H':   // Absolute Horizontal Line
                do {
                    last_x = parser.matchNumber();
                    points.emplace_back(last_x);
                    points.emplace_back(last_y);
                    commands.push_back('L');
                    isDrawn = true;
                } while (parser.isNumber());
                break;

            case 'h':   // Relative Horizontal Line
                do {
                    last_x += parser.matchNumber();
                    points.emplace_back(last_x);
                    points.emplace_back(last_y);
                    commands.push_back('L');
                    isDrawn = true;
                } while (parser.isNumber());
                break;

            case 'V':  // Absolute Vertical Line
                do {
                    last_y = parser.matchNumber();
                    points.emplace_back(last_x);
                    points.emplace_back(last_y);
                    commands.push_back('L');
                    isDrawn = true;
                } while (parser.isNumber());
                break;

            case 'v':  // Relative Vertical Line
                do {
                    last_y += parser.matchNumber();
                    points.emplace_back(last_x);
                    points.emplace_back(last_y);
                    commands.push_back('L');
                    isDrawn = true;
                } while (parser.isNumber());
                break;

            case 'C':  // Absolute Bezier cubic curve
                do {
                    float x1 = parser.matchNumber();
                    float y1 = parser.matchNumber();
                    float x2 = parser.matchNumber();
                    float y2 = parser.matchNumber();
                    last_x = parser.matchNumber();
                    last_y = parser.matchNumber();
                    points.insert(points.end(), {x1, y1, x2, y2, last_x, last_y});
                    commands.push_back('C');
                    control_x = 2 * last_x - x2;
                    control_y = 2 * last_y - y2;
                    isDrawn = true;
                } while (parser.isNumber());

                break;

            case 'c':  // Relative Bezier curve
                do {
                    float x1 = parser.matchNumber() + last_x;
                    float y1 = parser.matchNumber() + last_y;
                    float x2 = parser.matchNumber() + last_x;
                    float y2 = parser.matchNumber() + last_y;
                    last_x += parser.matchNumber();
                    last_y += parser.matchNumber();
                    points.insert(points.end(), {x1, y1, x2, y2, last_x, last_y});
                    commands.push_back('C');
                    control_x = 2 * last_x - x2;
                    control_y = 2 * last_y - y2;
                    isDrawn = true;
                } while (parser.isNumber());

                break;

            case 'S':  // Smooth Bezier curve
                if (!std::strchr("CcSs", lastCh)) {
                    control_x = last_x;
                    control_y = last_y;
                }

                do {
                    float x2 = parser.matchNumber();
                    float y2 = parser.matchNumber();
                    last_x = parser.matchNumber();
                    last_y = parser.matchNumber();
                    points.insert(points.end(), {control_x, control_y, x2, y2, last_x, last_y});
                    commands.push_back('C');
                    control_x = 2 * last_x - x2;
                    control_y = 2 * last_y - y2;
                    isDrawn = true;
                } while (parser.isNumber());

                break;

            case 's':
                if (!std::strchr("CcSs", lastCh)) {
                    control_x = last_x;
                    control_y = last_y;
                }

                do {
                    float x2 = parser.matchNumber() + last_x;
                    float y2 = parser.matchNumber() + last_y;
                    last_x += parser.matchNumber();
                    last_y += parser.matchNumber();
                    points.insert(points.end(), {control_x, control_y, x2, y2, last_x, last_y});
                    commands.push_back('C');
                    control_x = 2 * last_x - x2;
                    control_y = 2 * last_y - y2;
                    isDrawn = true;
                } while (parser.isNumber());

                break;

            case 'Q':  // Quadratic Bezier curve
                do {
                    float x1 = parser.matchNumber();
                    float y1 = parser.matchNumber();
                    last_x = parser.matchNumber();
                    last_y = parser.matchNumber();
                    points.insert(points.end(), {x1, y1, last_x, last_y});
                    commands.push_back('Q');
                    control_x = 2 * last_x - x1;
                    control_y = 2 * last_y - y1;
                    isDrawn = true;
                } while (parser.isNumber());

                break;

            case 'q':
                do {
                    float x1 = parser.matchNumber() + last_x;
                    float y1 = parser.matchNumber() + last_y;
                    last_x += parser.matchNumber();
                    last_y += parser.matchNumber();
                    points.insert(points.end(), {x1, y1, last_x, last_y});
                    commands.push_back('Q');
                    control_x = 2 * last_x - x1;
                    control_y = 2 * last_y - y1;
                    isDrawn = true;
                } while (parser.isNumber());

                break;

            case 'T':  // Smooth Quadratic Bezier curve
                if (!std::strchr("QqTt", lastCh)) {
                    control_x = last_x;
                    control_y = last_y;
                }

                do {
                    last_x = parser.matchNumber();
                    last_y = parser.matchNumber();
                    points.insert(points.end(), {control_x, control_y, last_x, last_y});
                    commands.push_back('Q');
                    control_x = 2 * last_x - control_x;
                    control_y = 2 * last_y - control_y;
                    isDrawn = true;
                } while (parser.isNumber());

                break;

            case 't':
                if (!std::strchr("QqTt", lastCh)) {
                    control_x = last_x;
                    control_y = last_y;
                }

                do {
                    last_x += parser.matchNumber();
                    last_y += parser.matchNumber();
                    points.insert(points.end(), {control_x, control_y, last_x, last_y});
                    commands.push_back('Q');
                    control_x = 2 * last_x - control_x;
                    control_y = 2 * last_y - control_y;
                    isDrawn = true;
                } while (parser.isNumber());

                break;

            case 'A':  // Elliptical arc
                do {
                    float rx = parser.matchNumber();
                    float ry = parser.matchNumber();
                    float x_axis_rotation = parser.matchNumber();
                    float large_arc_flag = parser.matchNumber();
                    float sweep_flag = parser.matchNumber();
                    last_x = parser.matchNumber();
                    last_y = parser.matchNumber();
                    points.insert(points.end(), {rx, ry, x_axis_rotation, large_arc_flag, sweep_flag, last_x, last_y});
                    commands.push_back('A');
                    isDrawn = true;
                } while (parser.isNumber());

                break;

            case 'a':
                do {
                    float rx = parser.matchNumber();
                    float ry = parser.matchNumber();
                    float x_axis_rotation = parser.matchNumber();
                    float large_arc_flag = parser.matchNumber();
                    float sweep_flag = parser.matchNumber();
                    last_x += parser.matchNumber();
                    last_y += parser.matchNumber();
                    points.insert(points.end(), {rx, ry, x_axis_rotation, large_arc_flag, sweep_flag, last_x, last_y});
                    commands.push_back('A');
                    isDrawn = true;
                } while (parser.isNumber());

                break;

            case 'Z':  // Close the current sub-path
            case 'z':
                commands.push_back('Z');
                break;

            default:
                printf("Error - unrecognized character %d\n", ch);
                return result;
        }
        lastCh = ch;
    }

    if (!parser.failed() && isDrawn)
        result->setPaths(std::move(commands), std::move(points));

    return result;
}

} // namespace sg
} // namespace apl