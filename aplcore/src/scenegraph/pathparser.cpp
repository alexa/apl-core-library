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

#include <array>
#include <apl/scenegraph/path.h>

#include "apl/primitives/transform2d.h"
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

        // TODO: This isn't exactly right because it accepts hexadecimal values and other invalid SVG path data
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

/**
 * A MutablePath is a path to which we add pathData strings
 * A valid MutablePath is one that has at least one drawn segment.
 */
class MutablePath {
public:
    MutablePath()
        : mGeneralPath(std::make_shared<GeneralPath>()),
          mPoints(mGeneralPath->mPoints),
          mCommands(mGeneralPath->mValue)
    {}

    void add(const std::string& path);
    std::shared_ptr<GeneralPath> getGeneralPath();

private:
    void addArc(float radiusX, float radiusY, float degrees, bool largeArcFlag,
                bool sweepFlag, float endX, float endY);

private:
    std::shared_ptr<GeneralPath> mGeneralPath;

    std::vector<float>& mPoints;
    std::string& mCommands;

    // Last point
    float mLastX = 0;
    float mLastY = 0;

    // Last control point
    float mControlX = 0;
    float mControlY = 0;

    char mLastCharacter = 0;
    bool mIsDrawn = false;   // Set to true once we find something that draws
};

std::shared_ptr<GeneralPath>
MutablePath::getGeneralPath() {
    // Return an empty path if nothing was drawn
    if (!mIsDrawn) {
        mPoints.clear();
        mCommands.erase();
    }

    // Drop any move commands from the end of the command list
    while (!mCommands.empty() && mCommands.back() == 'M') {
        mCommands.resize(mCommands.size() - 1);
        mPoints.resize(mPoints.size() - 2);
    }

    return mGeneralPath;
}

void
MutablePath::addArc(float radiusX, float radiusY, float degrees, bool largeArcFlag,
                    bool sweepFlag, float endX, float endY)
{
    if (mLastX == endX && mLastY == endY)
        return;

    auto start = Point(mLastX, mLastY);
    auto end = Point(endX, endY);

    // Straight line?
    auto rx = std::abs(radiusX);
    auto ry = std::abs(radiusY);
    if (rx < 1e-12 || ry < 1e-12) {
        mPoints.emplace_back(endX);
        mPoints.emplace_back(endY);
        mCommands.push_back('L');
        mIsDrawn = true;
        return;
    }

    auto pointTransform = Transform2D::rotate(-degrees);

    // TODO: Is this negative?
    auto midPointDistance = Point(0.5f*(mLastX - endX), 0.5f*(mLastY - endY));
    auto transformedMidPoint = pointTransform * midPointDistance;

    float squareRx = rx * rx;
    float squareRy = ry * ry;
    float squareX = transformedMidPoint.getX() * transformedMidPoint.getX();
    float squareY = transformedMidPoint.getY() * transformedMidPoint.getY();

    // Check if the radii are big enough to draw the arc, scale radii if not.
    // http://www.w3.org/TR/SVG/implnote.html#ArcCorrectionOutOfRangeRadii
    float radiiScale = squareX / squareRx + squareY / squareRy;
    if (radiiScale > 1) {
        radiiScale = sqrt(radiiScale);
        rx *= radiiScale;
        ry *= radiiScale;
    }

    // Set up a transformation for a unit circle (scale, then rotate)
    pointTransform = Transform2D::scale(1/rx, 1/ry) * Transform2D::rotate(-degrees);

    auto point1 = pointTransform * start;
    auto point2 = pointTransform * end;

    float dx = point2.getX() - point1.getX();
    float dy = point2.getY() - point1.getY();

    float d = dx * dx + dy * dy;
    float scaleFactorSquared = std::max(1 / d - 0.25f, 0.f);
    float scaleFactor = sqrt(scaleFactorSquared);
    if (largeArcFlag == sweepFlag)
        scaleFactor = -scaleFactor;

    dx *= scaleFactor;
    dy *= scaleFactor;

    auto centerPoint = Point(0.5f * (point1.getX() + point2.getX()) - dy,
                             0.5f * (point1.getY() + point2.getY()) + dx);

    // Calculate the starting and ending angle
    float theta1 = atan2f(point1.getY() - centerPoint.getY(), point1.getX() - centerPoint.getX());
    float theta2 = atan2f(point2.getY() - centerPoint.getY(), point2.getX() - centerPoint.getX());

    float thetaArc = theta2 - theta1;
    if (thetaArc < 0 && sweepFlag) {  // arcSweep flipped from the original implementation
        thetaArc += M_PI * 2;
    } else if (thetaArc > 0 && !sweepFlag) {  // arcSweep flipped from the original implementation
        thetaArc -= M_PI * 2;
    }

    // Very tiny angles cause our subsequent math to go wonky (skbug.com/9272)
    // so we do a quick check here. The precise tolerance amount is just made up.
    // PI/million happens to fix the bug in 9272, but a larger value is probably
    // ok too.
    if (std::abs(thetaArc) < (M_PI / (1000 * 1000))) {
        mPoints.emplace_back(endX);
        mPoints.emplace_back(endY);
        mCommands.push_back('L');
        mIsDrawn = true;
        return;
    }

    // Configure the transform to go from the unit circle to the rotated ellipse
    pointTransform = Transform2D::rotate(degrees) * Transform2D::scale(rx, ry);

    // the arc may be slightly bigger than 1/4 circle, so allow up to 1/3rd
    int segments = ceil(std::abs(thetaArc / (2 * M_PI / 3)));   // Also has been done as (M_PI_2 + 0.001)
    float thetaWidth = thetaArc / segments;
    float startThetaSin = sinf(theta1);
    float startThetaCos = cosf(theta1);
    float t = (4.f/3.f) * tanf(0.25f * thetaWidth);  // Used for the Cubic's control points
    if (!std::isfinite(t))
        return;

    for (int i = 0 ; i < segments ; i++) {
        float endTheta = theta1 + (i + 1) * thetaWidth;

        float endThetaSin = sinf(endTheta);
        float endThetaCos = cosf(endTheta);

        auto cp1 = pointTransform * Point( startThetaCos - t * startThetaSin + centerPoint.getX(),
                                          startThetaSin + t * startThetaCos + centerPoint.getY());
        auto cp2 = pointTransform * Point( endThetaCos + t * endThetaSin + centerPoint.getX(),
                                           endThetaSin - t * endThetaCos + centerPoint.getY());
        auto xy = pointTransform * Point( endThetaCos + centerPoint.getX(),
                                         endThetaSin + centerPoint.getY());

        mPoints.insert(mPoints.end(), { cp1.getX(), cp1.getY(), cp2.getX(), cp2.getY(), xy.getX(), xy.getY()});
        mCommands.push_back('C');
        mControlX = 2 * xy.getX() - cp2.getX();  // Set this because we could be followed by an 'S' or 's'
        mControlY = 2 * xy.getY() - cp2.getY();
        mIsDrawn = true;

        startThetaSin = endThetaSin;
        startThetaCos = endThetaCos;
    }
}

void
MutablePath::add(const std::string& path)
{
    PathParser parser(path.c_str());
    while (parser.isCharacter()) {
        auto ch = parser.matchCharacter();
        switch (ch) {
            case 'M': // Absolute MoveTo
                do {
                    mLastX = parser.matchNumber();
                    mLastY = parser.matchNumber();
                    if (!mCommands.empty() && mCommands.back() == 'M')  // Last was move
                        mPoints.resize(mPoints.size() - 2);  // Drop the points from the last move
                    else
                        mCommands.push_back('M');
                    mPoints.emplace_back(mLastX);
                    mPoints.emplace_back(mLastY);
                } while (parser.isNumber());
                break;

            case 'm': // Relative MoveTo  -> convert to absolute move to
                do {
                    mLastX += parser.matchNumber();
                    mLastY += parser.matchNumber();
                    if (!mCommands.empty() && mCommands.back() == 'M')  // Last was move
                        mPoints.resize(mPoints.size() - 2);  // Drop the points from the last move
                    else
                        mCommands.push_back('M');
                    mPoints.emplace_back(mLastX);
                    mPoints.emplace_back(mLastY);
                } while (parser.isNumber());
                break;

            case 'L': // Absolute LineTo
                do {
                    mLastX = parser.matchNumber();
                    mLastY = parser.matchNumber();
                    mPoints.emplace_back(mLastX);
                    mPoints.emplace_back(mLastY);
                    mCommands.push_back('L');
                    mIsDrawn = true;
                } while (parser.isNumber());
                break;

            case 'l': // Relative LineTo -> convert to absolute line to
                do {
                    mLastX += parser.matchNumber();
                    mLastY += parser.matchNumber();
                    mPoints.emplace_back(mLastX);
                    mPoints.emplace_back(mLastY);
                    mCommands.push_back('L');
                    mIsDrawn = true;
                } while (parser.isNumber());
                break;

            case 'H': // Absolute Horizontal Line
                do {
                    mLastX = parser.matchNumber();
                    mPoints.emplace_back(mLastX);
                    mPoints.emplace_back(mLastY);
                    mCommands.push_back('L');
                    mIsDrawn = true;
                } while (parser.isNumber());
                break;

            case 'h': // Relative Horizontal Line
                do {
                    mLastX += parser.matchNumber();
                    mPoints.emplace_back(mLastX);
                    mPoints.emplace_back(mLastY);
                    mCommands.push_back('L');
                    mIsDrawn = true;
                } while (parser.isNumber());
                break;

            case 'V': // Absolute Vertical Line
                do {
                    mLastY = parser.matchNumber();
                    mPoints.emplace_back(mLastX);
                    mPoints.emplace_back(mLastY);
                    mCommands.push_back('L');
                    mIsDrawn = true;
                } while (parser.isNumber());
                break;

            case 'v': // Relative Vertical Line
                do {
                    mLastY += parser.matchNumber();
                    mPoints.emplace_back(mLastX);
                    mPoints.emplace_back(mLastY);
                    mCommands.push_back('L');
                    mIsDrawn = true;
                } while (parser.isNumber());
                break;

            case 'C': // Absolute Bezier cubic curve
                do {
                    float x1 = parser.matchNumber();
                    float y1 = parser.matchNumber();
                    float x2 = parser.matchNumber();
                    float y2 = parser.matchNumber();
                    mLastX = parser.matchNumber();
                    mLastY = parser.matchNumber();
                    mPoints.insert(mPoints.end(), {x1, y1, x2, y2, mLastX, mLastY});
                    mCommands.push_back('C');
                    mControlX = 2 * mLastX - x2;
                    mControlY = 2 * mLastY - y2;
                    mIsDrawn = true;
                } while (parser.isNumber());

                break;

            case 'c': // Relative Bezier curve
                do {
                    float x1 = parser.matchNumber() + mLastX;
                    float y1 = parser.matchNumber() + mLastY;
                    float x2 = parser.matchNumber() + mLastX;
                    float y2 = parser.matchNumber() + mLastY;
                    mLastX += parser.matchNumber();
                    mLastY += parser.matchNumber();
                    mPoints.insert(mPoints.end(), {x1, y1, x2, y2, mLastX, mLastY});
                    mCommands.push_back('C');
                    mControlX = 2 * mLastX - x2;
                    mControlY = 2 * mLastY - y2;
                    mIsDrawn = true;
                } while (parser.isNumber());

                break;

            case 'S': // Smooth Bezier curve
                if (!std::strchr("CcSs", mLastCharacter)) {
                    mControlX = mLastX;
                    mControlY = mLastY;
                }

                do {
                    float x2 = parser.matchNumber();
                    float y2 = parser.matchNumber();
                    mLastX = parser.matchNumber();
                    mLastY = parser.matchNumber();
                    mPoints.insert(mPoints.end(), {mControlX, mControlY, x2, y2, mLastX, mLastY});
                    mCommands.push_back('C');
                    mControlX = 2 * mLastX - x2;
                    mControlY = 2 * mLastY - y2;
                    mIsDrawn = true;
                } while (parser.isNumber());

                break;

            case 's':
                if (!std::strchr("CcSs", mLastCharacter)) {
                    mControlX = mLastX;
                    mControlY = mLastY;
                }

                do {
                    float x2 = parser.matchNumber() + mLastX;
                    float y2 = parser.matchNumber() + mLastY;
                    mLastX += parser.matchNumber();
                    mLastY += parser.matchNumber();
                    mPoints.insert(mPoints.end(), {mControlX, mControlY, x2, y2, mLastX, mLastY});
                    mCommands.push_back('C');
                    mControlX = 2 * mLastX - x2;
                    mControlY = 2 * mLastY - y2;
                    mIsDrawn = true;
                } while (parser.isNumber());

                break;

            case 'Q': // Quadratic Bezier curve
                do {
                    float x1 = parser.matchNumber();
                    float y1 = parser.matchNumber();
                    mLastX = parser.matchNumber();
                    mLastY = parser.matchNumber();
                    mPoints.insert(mPoints.end(), {x1, y1, mLastX, mLastY});
                    mCommands.push_back('Q');
                    mControlX = 2 * mLastX - x1;
                    mControlY = 2 * mLastY - y1;
                    mIsDrawn = true;
                } while (parser.isNumber());

                break;

            case 'q':
                do {
                    float x1 = parser.matchNumber() + mLastX;
                    float y1 = parser.matchNumber() + mLastY;
                    mLastX += parser.matchNumber();
                    mLastY += parser.matchNumber();
                    mPoints.insert(mPoints.end(), {x1, y1, mLastX, mLastY});
                    mCommands.push_back('Q');
                    mControlX = 2 * mLastX - x1;
                    mControlY = 2 * mLastY - y1;
                    mIsDrawn = true;
                } while (parser.isNumber());

                break;

            case 'T': // Smooth Quadratic Bezier curve
                if (!std::strchr("QqTt", mLastCharacter)) {
                    mControlX = mLastX;
                    mControlY = mLastY;
                }

                do {
                    mLastX = parser.matchNumber();
                    mLastY = parser.matchNumber();
                    mPoints.insert(mPoints.end(), {mControlX, mControlY, mLastX, mLastY});
                    mCommands.push_back('Q');
                    mControlX = 2 * mLastX - mControlX;
                    mControlY = 2 * mLastY - mControlY;
                    mIsDrawn = true;
                } while (parser.isNumber());

                break;

            case 't':
                if (!std::strchr("QqTt", mLastCharacter)) {
                    mControlX = mLastX;
                    mControlY = mLastY;
                }

                do {
                    mLastX += parser.matchNumber();
                    mLastY += parser.matchNumber();
                    mPoints.insert(mPoints.end(), {mControlX, mControlY, mLastX, mLastY});
                    mCommands.push_back('Q');
                    mControlX = 2 * mLastX - mControlX;
                    mControlY = 2 * mLastY - mControlY;
                    mIsDrawn = true;
                } while (parser.isNumber());

                break;

            case 'A': // Elliptical arc
                do {
                    float rx = parser.matchNumber();
                    float ry = parser.matchNumber();
                    float x_axis_rotation = parser.matchNumber();
                    bool large_arc_flag = parser.matchNumber() != 0.0f;
                    bool sweep_flag = parser.matchNumber() != 0.0f;
                    float endX = parser.matchNumber();
                    float endY = parser.matchNumber();
                    addArc(rx, ry, x_axis_rotation, large_arc_flag, sweep_flag, endX, endY);
                    mLastX = endX;
                    mLastY = endY;
                } while (parser.isNumber());

                break;

            case 'a':
                do {
                    float rx = parser.matchNumber();
                    float ry = parser.matchNumber();
                    float x_axis_rotation = parser.matchNumber();
                    bool large_arc_flag = parser.matchNumber() != 0.0f;
                    bool sweep_flag = parser.matchNumber() != 0.0f;
                    float endX = mLastX + parser.matchNumber();
                    float endY = mLastY + parser.matchNumber();
                    addArc(rx, ry, x_axis_rotation, large_arc_flag, sweep_flag, endX, endY);
                    mLastX = endX;
                    mLastY = endY;
                } while (parser.isNumber());

                break;

            case 'Z': // Close the current sub-path
            case 'z':
                if (!mCommands.empty() && mCommands.back() != 'Z')
                    mCommands.push_back('Z');
                break;

            default:
                // Illegal character; drop all drawing changes
                mCommands.erase();
                mPoints.clear();
                return;
        }
        mLastCharacter = ch;
    }
}

std::shared_ptr<GeneralPath>
parsePathString(const std::string& path)
{
    MutablePath mp;
    mp.add(path);
    return mp.getGeneralPath();
}

} // namespace sg
} // namespace apl
