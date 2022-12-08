/*
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

#ifndef _APL_DIMENSION_H
#define _APL_DIMENSION_H

#include <string>
#include "apl/utils/streamer.h"
#include "apl/primitives/objecttype.h"

namespace apl {

class Context;

/**
 * The type of the dimension.
 */
enum class DimensionType {
    /// An absolute, measurable unit.  Stored internally as dp.
    Absolute,
    /// A size, stored as a percentage of the parent component.
    Relative,
    /// Automatically size to fit the contents.
    Auto
};

/**
 * A dimension may be absolute, relative, or auto.
 */
class Dimension
{
public:
    /**
     * Creates an auto-sized dimension.
     */
    Dimension() : mType(DimensionType::Auto), mValue(0) {}

    /**
     * Creates an absolute dimension of a specific size.
     * @param size Size, in display-independent pixels.
     */
    Dimension(double size) : mType(DimensionType::Absolute), mValue(size) {}

    /**
     * General constructor for building any type of dimension.
     * @param type The dimension type.
     * @param value The dimension size.  Ignored for auto.  Relative sizes should be in the range [0, 1].
     */
    Dimension(DimensionType type, double value) : mType(type), mValue(value) {}

    /**
     * Construct a dimension by parsing a string.  Accepts units such as "22px", "3dp", "auto", and "10%".
     * @param context The data-binding context.
     * @param str The string to parse.
     */
    Dimension(const Context& context, const std::string& str, bool preferRelative=false);

    // Copy, move, and assignment
    Dimension(const Dimension& rhs) = default;
    Dimension(Dimension&& rhs) = default;
    Dimension& operator=(const Dimension& rhs) = default;

    // equality
    bool operator==(const Dimension& rhs) const {
        return rhs.getType() == getType() && rhs.getValue() == getValue();
    }
    bool operator!=(const Dimension& rhs) const {
        return rhs.getType() != getType() || rhs.getValue() != getValue();
    }

    /**
     * @return True if this is an "auto" dimension.
     */
    bool isAuto() const { return mType == DimensionType::Auto; }

    /**
     * @return True if this is a relative dimension.
     */
    bool isRelative() const { return mType == DimensionType::Relative; }

    /**
     * @return True if this is an absolute dimension
     */
    bool isAbsolute() const { return mType == DimensionType::Absolute; }

    /**
     * @return The internal value of the dimension.  Undefined for auto dimensions.
     */
    double getValue() const { return mValue; }

    /**
     * @return The type of the dimension (auto, relative, absolute).
     */
    DimensionType getType() const { return mType; }

    friend streamer& operator<<(streamer&, const Dimension&);

    class DimensionObjectType : public BaseObjectType<Dimension> {
    public:
        bool isDimension() const final { return true; }

        bool equals(const Object::DataHolder& lhs, const Object::DataHolder& rhs) const override {
            return lhs.value == rhs.value;
        }
    };

    class AutoDimensionObjectType final : public TrueObjectType<Dimension> {
    public:
        static ObjectTypeRef instance() {
            static Dimension::AutoDimensionObjectType sType;
            return &sType;
        }

        bool isAutoDimension() const override { return true; }

        std::string asString(const Object::DataHolder& dataHolder) const override { return "auto"; }

        Dimension asDimension(const Object::DataHolder& dataHolder, const Context&) const override {
            return Dimension(DimensionType::Auto, 0);
        }

        std::size_t hash(const Object::DataHolder&) const override {
            return std::hash<std::string>{}("auto");
        }

        rapidjson::Value serialize(
            const Object::DataHolder&,
            rapidjson::Document::AllocatorType& allocator) const override
        {
            return rapidjson::Value("auto", allocator);
        }

        std::string toDebugString(const Object::DataHolder&) const override { return "AutoDim"; }

        bool equals(const Object::DataHolder&, const Object::DataHolder&) const override {
            return true;
        }
    };

    class RelativeDimensionObjectType final : public DimensionObjectType {
    public:
        static ObjectTypeRef instance() {
            static Dimension::RelativeDimensionObjectType sType;
            return &sType;
        }

        bool isRelativeDimension() const override { return true; }

        bool isNonAutoDimension() const override { return true; }

        std::string asString(const Object::DataHolder& dataHolder) const override {
            return doubleToAplFormattedString(dataHolder.value) + "%";
        }

        Dimension asDimension(const Object::DataHolder& dataHolder, const Context&) const override {
            return Dimension(DimensionType::Relative, dataHolder.value);
        }

        Dimension asNonAutoDimension(
            const Object::DataHolder& dataHolder,
            const Context&) const override
        {
            return Dimension(DimensionType::Relative, dataHolder.value);
        }

        Dimension asNonAutoRelativeDimension(
            const Object::DataHolder& dataHolder,
            const Context&) const override
        {
            return Dimension(DimensionType::Relative, dataHolder.value);
        }

        double getRelativeDimension(const Object::DataHolder& dataHolder) const override {
            return dataHolder.value;
        }

        bool truthy(const Object::DataHolder& dataHolder) const override {
            return dataHolder.value != 0;
        }

        std::size_t hash(const Object::DataHolder& dataHolder) const override {
            return std::hash<double>{}(dataHolder.value);
        }

        rapidjson::Value serialize(
            const Object::DataHolder& dataHolder,
            rapidjson::Document::AllocatorType& allocator) const override
        {
            return rapidjson::Value((doubleToAplFormattedString(dataHolder.value)+"%").c_str(), allocator);
        }

        std::string toDebugString(const Object::DataHolder& dataHolder) const override {
            return "RelDim<" + sutil::to_string(dataHolder.value) + ">";
        }
    };

    class AbsoluteDimensionObjectType final : public DimensionObjectType {
    public:
        static ObjectTypeRef instance() {
            static Dimension::AbsoluteDimensionObjectType sType;
            return &sType;
        }

        bool isAbsoluteDimension() const override { return true; }

        bool isNonAutoDimension() const override { return true; }

        std::string asString(const Object::DataHolder& dataHolder) const override {
            return doubleToAplFormattedString(dataHolder.value) + "dp";
        }

        double asNumber(const Object::DataHolder& dataHolder) const override {
            return dataHolder.value;
        }

        int asInt(const Object::DataHolder& dataHolder, int base) const override {
            return std::lround(dataHolder.value);
        }

        int64_t asInt64(const Object::DataHolder& dataHolder, int base) const override {
            return std::llround(dataHolder.value);
        }

        Dimension asDimension(const Object::DataHolder& dataHolder, const Context&) const override {
            return Dimension(DimensionType::Absolute, dataHolder.value);
        }

        Dimension asAbsoluteDimension(
            const Object::DataHolder& dataHolder,
            const Context&) const override
        {
            return Dimension(DimensionType::Absolute, dataHolder.value);
        }

        Dimension asNonAutoDimension(
            const Object::DataHolder& dataHolder,
            const Context&) const override
        {
            return Dimension(DimensionType::Absolute, dataHolder.value);
        }

        Dimension asNonAutoRelativeDimension(
            const Object::DataHolder& dataHolder,
            const Context&) const override
        {
            return Dimension(DimensionType::Absolute, dataHolder.value);
        }

        double getAbsoluteDimension(const Object::DataHolder& dataHolder) const override {
            return dataHolder.value;
        }

        bool truthy(const Object::DataHolder& dataHolder) const override {
            return dataHolder.value != 0;
        }

        std::size_t hash(const Object::DataHolder& dataHolder) const override {
            return std::hash<double>{}(dataHolder.value);
        }

        rapidjson::Value serialize(
            const Object::DataHolder& dataHolder,
            rapidjson::Document::AllocatorType& allocator) const override
        {
            return rapidjson::Value(std::isfinite(dataHolder.value) ? dataHolder.value : 0);
        }

        std::string toDebugString(const Object::DataHolder& dataHolder) const override {
            return "AbsDim<" + sutil::to_string(dataHolder.value) + ">";
        }
    };

private:
    DimensionType mType;
    double mValue;
};

}  // namespace apl

#endif // _APL_DIMENSION_H
