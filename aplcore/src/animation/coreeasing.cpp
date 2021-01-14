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


#include <algorithm>

#include "apl/animation/coreeasing.h"
#include "apl/animation/easingapproximation.h"
#include "apl/utils/weakcache.h"

namespace apl {

static WeakCache<EasingApproximation> sEasingAppoxCache;

std::string
dofSig(int dof, const float* array) {
    std::string result = "x" + std::to_string(array[0]);
    for (int i = 1; i < dof; i++)
        result += "," + std::to_string(array[i]);
    return result;
}

std::string
easingApproxSignature(int dof, const float* start, const float* tout, const float* tin,
                      const float* end) {
    return std::to_string(dof) + dofSig(dof, start) + dofSig(dof, tout) + dofSig(dof, tin) +
           dofSig(dof, end);
}

// Cubic
static inline float
f(float a, float b, float t) {
    return t * (3 * (1 - t) * (a * (1 - t) + b * t) + t * t);
}

/**
 * Given a cubic Bézier polynomial, where
 *
 *     x(t) = a1*(1-t)^3 + 3*a2*t*(1-t)^2 + 3*a3*t^2*(1-t) + a4*t^3
 *     y(t) = b1*(1-t)^3 + 3*b2*t*(1-t)^2 + 3*b3*t^2*(1-t) + b4*t^3
 *
 * For a given value of x find the matching value of y.  We restrict ourselves
 * to the case where a1=b1=0 and a4=b4=1.
 *
 * @param a The first x-control point parameter (a2)
 * @param b The first y-control point parameter (b2)
 * @param c The second x-control point parameter (a3)
 * @param d The second y-control point parameter (b3)
 * @param x The target value
 * @return The calculated value y
 */
static float
binarySearchCubic(const float a[], float x) {
    if (a[0] == a[1] && a[2] == a[3]) // Linear; don't need to do anything
        return x;

    float left = 0;
    float right = 1;

    // Binary search for a sufficiently accurate estimate of the interpolated value.
    while (left < right) {
        float mid = (left + right) / 2;
        float x_estimate = f(a[0], a[2], mid);
        if (std::fabs(x - x_estimate) < 0.00001)
            return f(a[1], a[3], mid);

        if (x_estimate < x)
            left = mid;
        else
            right = mid;
    }
    // This should never be reached.
    assert(false);
    return 0;
}

EasingPtr
CoreEasing::bezier(float a, float b, float c, float d) noexcept
{
    return create(
        std::vector<EasingSegment>{EasingSegment(kCurveSegment, 0), EasingSegment(kEndSegment, 6)},
        std::vector<float>{0, 0, a, b, c, d, 1, 1},
        "cubic-bezier(" + std::to_string(a) + "," + std::to_string(b) + "," + std::to_string(c) +
            "," + std::to_string(d) + ")");
}

EasingPtr
CoreEasing::linear() noexcept
{
    return create(
        std::vector<EasingSegment>{EasingSegment(kLinearSegment, 0), EasingSegment(kEndSegment, 2)},
        std::vector<float>{0, 0, 1, 1}, "path()");
}

EasingPtr
CoreEasing::create(std::vector<EasingSegment>&& segments, std::vector<float>&& points,
                   std::string debugString)
{
    assert(segments.size() > 1 && points.size() >= 4);
    assert(points.size() >= segments.rbegin()->offset + 2);

    return std::make_shared<CoreEasing>(std::move(segments), std::move(points),
                                        std::move(debugString));
}

class BaseSegment {
public:
    BaseSegment(const CoreEasing& easing, std::vector<EasingSegment>::const_iterator it)
        : mData(&easing.mPoints.at(it->offset)) {}

    float startTime() const { return mData[0]; }
    float startValue() const { return mData[1]; }

protected:
    const float* mData;
};

class LinearSegment : public BaseSegment {
public:
    LinearSegment(const CoreEasing& easing, std::vector<EasingSegment>::const_iterator it)
        : BaseSegment(easing, it) {}
    float endTime() const { return mData[2]; }
    float endValue() const { return mData[3]; }
};

class CurveSegment : public BaseSegment {
public:
    CurveSegment(const CoreEasing& easing, std::vector<EasingSegment>::const_iterator it)
        : BaseSegment(easing, it) {}
    const float* controlPoints() const { return &mData[2]; }
    float endTime() const { return mData[6]; }
    float endValue() const { return mData[7]; }
};

class PSegment {
public:
    PSegment(const CoreEasing& easing, std::vector<EasingSegment>::const_iterator it)
        : easing(easing), mData(&easing.mPoints.at(it->offset)) {}

    int dof() const { return ::abs(static_cast<int>(easing.mPoints.at(0))); }
    int index() const { return static_cast<int>(easing.mPoints.at(1)); }

    const float* start() const { return &mData[1]; }

    float startTime() const { return mData[0]; }
    float startValue() const { return mData[1 + index()]; }

protected:
    const CoreEasing& easing;
    const float* mData;
};

class PCurveSegment : public PSegment {
public:
    PCurveSegment(const CoreEasing& easing, std::vector<EasingSegment>::const_iterator it)
        : PSegment(easing, it) {}

    float endTime() const { return mData[5 + 3 * dof()]; }
    float endValue() const { return mData[6 + 3 * dof() + index()]; }

    const float* tout() const { return &mData[1 + dof()]; }
    const float* tin() const { return &mData[1 + 2 * dof()]; }
    const float* controlPoints() const { return &mData[1 + 3 * dof()]; }
    const float* end() const { return &mData[6 + 3 * dof()]; }
};

// This number has been experimentally determined as appearing visually "smooth enough"
// for long path segments.  In the future we should select a number of easing segments
// based on the pixel length of the path.
const int EASING_POINTS = 51;

std::shared_ptr<EasingApproximation>
CoreEasing::ensureApproximation(std::vector<EasingSegment>::iterator it)
{
    if (it->data)
        return std::static_pointer_cast<EasingApproximation>(it->data);

    auto segment = PCurveSegment(*this, it);
    const float* start = segment.start();
    const float* tout = segment.tout();
    const float* tin = segment.tin();
    const float* end = segment.end();

    auto sig = easingApproxSignature(segment.dof(), start, tout, tin, end);
    auto ptr = sEasingAppoxCache.find(sig);
    if (ptr)
        return ptr;

    ptr = EasingApproximation::create(segment.dof(), start, tout, tin, end, EASING_POINTS);
    it->data = ptr;

    sEasingAppoxCache.insert(sig, ptr);
    return ptr;
}

float
CoreEasing::calcInternal(float t)
{
    switch (mSegments.begin()->type) {
        case kEndSegment:
        case kLinearSegment: // FALL_THROUGH
        case kCurveSegment: {
            auto segment = BaseSegment(*this, mSegments.begin());
            if (t <= segment.startTime())
                return segment.startValue();
        }

        case kSEndSegment: // FALL_THROUGH
        case kSCurveSegment: {
            auto segment = PSegment(*this, mSegments.begin());
            if (t <= segment.startTime()) {
                return segment.startValue();
            }
        }
    }

    // Use a binary search to find the largest segment with a start time less than or equal to "t"
    auto it = mSegments.begin();
    auto len = std::distance(mSegments.begin(), mSegments.end());
    while (len > 1) {
        auto step = len / 2;
        auto mid = it;
        std::advance(mid, step);
        auto midTime = mPoints.at(mid->offset);
        if (t < midTime)
            len = step;
        else {
            it = mid;
            len -= step;
        }
    }

    switch (it->type) {
        case kEndSegment: {
            auto segment = BaseSegment(*this, it);
            return segment.startValue();
        }

        case kLinearSegment: {
            auto segment = LinearSegment(*this, it);
            auto t1 = segment.startTime();
            auto v1 = segment.startValue();
            auto t2 = segment.endTime();
            auto v2 = segment.endValue();
            return v1 + (v2 - v1) * (t - t1) / (t2 - t1);
        }

        case kCurveSegment: {
            auto segment = CurveSegment(*this, it);
            auto t1 = segment.startTime();
            auto v1 = segment.startValue();
            auto t2 = segment.endTime();
            auto v2 = segment.endValue();

            auto dt = (t - t1) / (t2 - t1);
            return v1 + (v2 - v1) * binarySearchCubic(segment.controlPoints(), dt);
        }

        case kSEndSegment: {
            auto segment = PSegment(*this, it);
            return segment.startValue();
        }

        case kSCurveSegment: {
            auto segment = PCurveSegment(*this, it);
            auto t1 = segment.startTime();
            auto t2 = segment.endTime();

            auto dt = (t - t1) / (t2 - t1);
            auto percentage = binarySearchCubic(segment.controlPoints(), dt);
            auto approx = ensureApproximation(it);
            return approx->getPosition(percentage, segment.index());
        }
    }
    return 0;
}

float
CoreEasing::calc(float t)
{
    if (t == mLastTime)
        return mLastValue;

    mLastTime = t;
    mLastValue = calcInternal(t);
    return mLastValue;
}

float
CoreEasing::segmentStartTime(std::vector<EasingSegment>::iterator it)
{
    switch (it->type) {
        case kEndSegment:
        case kLinearSegment:
        case kCurveSegment:
            return BaseSegment(*this, it).startTime();

        case kSEndSegment:
        case kSCurveSegment:
            return PSegment(*this, it).startTime();
    }
    return 0;
}


Easing::Bounds
CoreEasing::bounds()
{
    float minTime = mSegments.empty() ? 0 : segmentStartTime(mSegments.begin());
    // The ending time is given by the start of the last segment
    float maxTime = mSegments.empty() ? 0 : segmentStartTime(mSegments.end() - 1);

    // We have to search the segments for the minimum and maximum values
    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::min();

    for (auto it = mSegments.begin(); it != mSegments.end(); it++) {
        switch (it->type) {
            case kEndSegment:
            case kLinearSegment: {
                auto segment = BaseSegment(*this, it);
                minValue = std::min(minValue, segment.startValue());
                maxValue = std::min(maxValue, segment.startValue());

                if (it == mSegments.begin())
                    minTime = segment.startTime();
                if (it == mSegments.end() - 1)
                    maxTime = segment.startTime();
                break;
            }
            case kCurveSegment: {
                // Approximate the bounding box using the control points
                auto segment = CurveSegment(*this, it);
                auto v1 = segment.startValue();
                auto v2 = segment.endValue();
                auto v3 = v1 + (v2 - v1) * segment.controlPoints()[1];
                auto v4 = v1 + (v2 - v1) * segment.controlPoints()[3];

                minValue = std::min({minValue, v1, v2, v3, v4});
                maxValue = std::max({maxValue, v1, v2, v3, v4});
                break;
            }
            case kSEndSegment: {
                auto segment = PSegment(*this, it);
                minValue = std::min(minValue, segment.startValue());
                maxValue = std::max(maxValue, segment.startValue());
                break;
            }
            case kSCurveSegment: {
                auto segment = PCurveSegment(*this, it);
                auto approx = ensureApproximation(it);
                auto index = segment.index();
                for (int i = 0; i < 11; i++) {
                    auto p = i * 0.01;
                    auto v = approx->getPosition(p, index);
                    minValue = std::min(minValue, v);
                    maxValue = std::max(maxValue, v);
                }
            } break;
        }
    }

    return {minTime, maxTime, minValue, maxValue};
}

} // namespace apl
