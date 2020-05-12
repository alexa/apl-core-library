/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef _APL_CORE_EASING_H
#define _APL_CORE_EASING_H

#include <vector>
#include <array>
#include <cmath>

#include "apl/animation/easing.h"

namespace apl {

class PathEasing;
class CubicBezierEasing;

class EasingCurve {
public:
    virtual ~EasingCurve() {}

    virtual float calc(float t) const = 0;
    virtual bool equal(EasingCurvePtr rhs) const = 0;
    virtual std::string toDebugString() const = 0;
};
/**
 * Linear easing curve.
 */
class LinearEasing : public EasingCurve {
public:
    float calc(float t) const override {
        if (t < 0) return 0;
        if (t > 1) return 1;
        return t;
    }

    bool equal(EasingCurvePtr rhs) const override {
        auto other = std::dynamic_pointer_cast<LinearEasing>(rhs);
        return other != nullptr;
    }

    std::string toDebugString() const override {
        return "linear";
    }
};

/**
 * Path easing curve.  A linear piecewise function from (0,0) to (1,1).  The time values must be in ascending
 * order and between 0 and 1.  The y values may be arbitrary.  The end points of (0,0) and (1,1) must be included.
 */
class PathEasing : public EasingCurve {
public:
    // We assume that the endpoints are provided and the x-values are correctly ordered and distinct
    PathEasing(std::vector<float>&& points)
        : mPoints(std::move(points))
    {}

    float calc(float t) const override {
        if (t <= 0) return 0;
        if (t >= 1) return 1;

        int left = 0;
        int right = mPoints.size() / 2;

        while (left < right - 1) {
            auto mid = (left + right) / 2;
            if (t < mPoints.at(mid * 2))
                right = mid;
            else
                left = mid;
        }

        left *= 2;
        float t1 = mPoints.at(left);
        float t2 = mPoints.at(left + 2);
        float v1 = mPoints.at(left + 1);
        float v2 = mPoints.at(left + 3);

        return v1 + (v2 - v1) * (t - t1) / (t2 - t1);
    }

    bool equal(EasingCurvePtr rhs) const override {
        auto other = std::dynamic_pointer_cast<PathEasing>(rhs);
        return other != nullptr && mPoints == other->mPoints;
    }

    std::string toDebugString() const override {
        std::string result = "path(";
        for (const auto& p : mPoints)
            result += std::to_string(p)+",";
        return result.substr(0, result.size()-1) + ")";
    }

private:
    std::vector<float> mPoints;
};


class CubicBezierEasing : public EasingCurve {
public:
    CubicBezierEasing(float a, float b, float c, float d)
        : mA(a), mB(b), mC(c), mD(d)
    {}

    static inline float f(float a, float b, float t) {
        //return 3*a*(1-t)*(1-t)*t + 3*b*(1-t)*t*t + t*t*t;
        return t*(3*(1-t)*(a*(1-t)+b*t) + t*t);
    }

    float calc(float t) const override {
        if (t <= 0) return 0;
        if (t >= 1) return 1;

        float left = 0;
        float right = 1;

        while (left < right) {
            float mid = (left + right) / 2;
            float t_estimate = f(mA, mC, mid);
            if (std::fabs(t - t_estimate) < 0.00001)
                return f(mB, mD, mid);

            if (t_estimate < t)
                left = mid;
            else
                right = mid;
        }
        // This should never be reached.
        return f(mB, mD, left);
    }

    bool equal(EasingCurvePtr rhs) const override {
        auto other = std::dynamic_pointer_cast<CubicBezierEasing>(rhs);
        return other != nullptr && mA == other->mA && mB == other->mB && mC == other->mC && mD == other->mD;
    }

    std::string toDebugString() const override {
        return "cubic-bezier("+std::to_string(mA)+","+std::to_string(mB)+","+std::to_string(mC)+","+std::to_string(mD)+")";
    }

private:
    float mA, mB, mC, mD;
};

} // namespace apl

#endif //_APL_CORE_EASING_H
