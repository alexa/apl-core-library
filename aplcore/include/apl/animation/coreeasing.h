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

#ifndef _APL_CORE_EASING_H
#define _APL_CORE_EASING_H

#include <string>
#include <vector>

#include "apl/animation/easing.h"

namespace apl {

enum SegmentType {
    kEndSegment,
    kLinearSegment,
    kCurveSegment,
    kSEndSegment,
    kSCurveSegment,
};

struct EasingSegment {
    struct Data {};

    EasingSegment(SegmentType type, size_t offset) : type(type), offset(offset) {}
    bool operator==(const EasingSegment& rhs) const { return type == rhs.type && offset == rhs.offset; }

    const SegmentType type;
    const size_t offset;
    std::shared_ptr<Data> data;  // Optional shared pointer
};

class EasingApproximation;

class CoreEasing : public Easing {
public:
    static EasingPtr bezier(float a, float b, float c, float d) noexcept;
    static EasingPtr linear() noexcept;

    static EasingPtr create(std::vector<EasingSegment>&& segments,
                            std::vector<float>&& points,
                            std::string debugString);

    // Standard constructor.  Do not call this; use CoreEasing::create()
    CoreEasing(std::vector<EasingSegment>&& segments,
               std::vector<float>&& points,
               std::string debugString) noexcept
        : mSegments(std::move(segments)),
          mPoints(std::move(points)),
          mDebugString(std::move(debugString))
    {}

    float calc(float t) override;

    Bounds bounds() override;

    std::string toDebugString() const override { return mDebugString; }

    bool operator==(const Easing& other) const override {
        return other == *this;
    }

    bool operator==(const CoreEasing& other) const override {
        return mSegments == other.mSegments && mPoints == other.mPoints;
    }

private:
    float calcInternal(float t);
    float segmentStartTime(std::vector<EasingSegment>::iterator it);
    std::shared_ptr<EasingApproximation> ensureApproximation(std::vector<EasingSegment>::iterator it);

    friend class PSegment;
    friend class BaseSegment;

private:
    std::vector<EasingSegment> mSegments;
    std::vector<float> mPoints;
    std::string mDebugString;

    float mLastTime = std::numeric_limits<float>::min();  // Magic number
    float mLastValue = 0;
};
} // namespace apl

#endif //_APL_CORE_EASING_H
