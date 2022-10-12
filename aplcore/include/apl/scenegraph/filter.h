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

#ifndef _APL_SG_FILTER_H
#define _APL_SG_FILTER_H

#include "apl/common.h"
#include "apl/primitives/filter.h"
#include "apl/primitives/rect.h"
#include "apl/media/mediaobject.h"
#include "apl/scenegraph/paint.h"
#include "apl/utils/noncopyable.h"

namespace apl {
namespace sg {

class Filter : public NonCopyable {
public:
    enum Type {
        kBlend,
        kBlur,
        kGrayscale,
        kMediaObject,
        kNoise,
        kSaturate,
        kSolid,
    };

    explicit Filter(Type type) : type(type) {}
    virtual ~Filter() = default;
    virtual std::string toDebugString() const = 0;
    virtual Size size() const { return {}; }
    virtual bool visible() const = 0;
    virtual rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const = 0;

    Type type;
};

using FilterPtr = std::shared_ptr<Filter>;

#define FILTER_SUBCLASS(TYPE_NAME, TYPE_ENUM)                 \
  public:                                                   \
    TYPE_NAME() : Filter(TYPE_ENUM) {}                        \
    static TYPE_NAME *cast(Filter *filter) {                    \
        assert(filter != nullptr && filter->type == TYPE_ENUM); \
        return reinterpret_cast<TYPE_NAME*>(filter);          \
    }                                                       \
    static TYPE_NAME *cast(const FilterPtr& filter) {           \
        return cast(filter.get());                            \
    }                  \
    static bool is_type(const Filter *filter) { return filter && filter->type == TYPE_ENUM; } \
    static bool is_type(const FilterPtr& filter) { return is_type(filter.get()); }          \
    std::string toDebugString() const override; \
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override


class BlendFilter : public Filter {
    FILTER_SUBCLASS(BlendFilter, kBlend);

    Size size() const override { return back->size().empty() ? front->size() : back->size(); }
    bool visible() const override { return true; }

    FilterPtr back;
    FilterPtr front;
    BlendMode blendMode = kBlendModeNormal;
};

class BlurFilter : public Filter {
    FILTER_SUBCLASS(BlurFilter, kBlur);

    Size size() const override { return filter->size(); }
    bool visible() const override { return radius > 0.0f; }

    FilterPtr filter;
    float radius = 0.0f;
};

class GrayscaleFilter : public Filter {
    FILTER_SUBCLASS(GrayscaleFilter, kGrayscale);

    Size size() const override { return filter->size(); }
    bool visible() const override { return amount > 0.0f; }

    FilterPtr filter;
    float amount = 0.0f;
};

class MediaObjectFilter : public Filter {
    FILTER_SUBCLASS(MediaObjectFilter, kMediaObject);

    Size size() const override { return mediaObject->size(); }
    bool visible() const override { return true; }

    MediaObjectPtr mediaObject;
};

class NoiseFilter : public Filter {
    FILTER_SUBCLASS(NoiseFilter, kNoise);

    Size size() const override { return filter->size(); }
    bool visible() const override { return true; }

    FilterPtr filter;
    NoiseFilterKind kind = kFilterNoiseKindGaussian;
    bool useColor = false;
    float sigma = 0.0f;
};

class SaturateFilter : public Filter {
    FILTER_SUBCLASS(SaturateFilter, kSaturate);

    Size size() const override { return filter->size(); }
    bool visible() const override { return amount != 1.0f; }

    FilterPtr filter;
    float amount = 0.0f;
};

class SolidFilter : public Filter {
    FILTER_SUBCLASS(SolidFilter, kSolid);

    bool visible() const override { return paint->visible(); }

    PaintPtr paint;
};

} // namespace sg
} // namespace apl

#endif // _APL_FILTER_H
