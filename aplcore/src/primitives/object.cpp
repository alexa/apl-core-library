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

#include <clocale>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "apl/animation/easing.h"
#include "apl/datagrammar/boundsymbol.h"
#include "apl/datagrammar/bytecode.h"
#include "apl/graphic/graphic.h"
#include "apl/graphic/graphicfilter.h"
#include "apl/graphic/graphicpattern.h"
#include "apl/primitives/accessibilityaction.h"
#include "apl/primitives/filter.h"
#include "apl/primitives/functions.h"
#include "apl/primitives/gradient.h"
#include "apl/livedata/livedataobject.h"
#include "apl/primitives/object.h"
#include "apl/primitives/objectdata.h"
#include "apl/primitives/mediasource.h"
#include "apl/primitives/transform.h"
#include "apl/primitives/rangegenerator.h"
#include "apl/primitives/slicegenerator.h"
#include "apl/component/componenteventwrapper.h"
#include "apl/engine/contextwrapper.h"
#include "apl/utils/log.h"

namespace apl {

const bool OBJECT_DEBUG = false;

/****************************************************************************/

template<typename XData, typename T>
std::shared_ptr<ObjectData> create(T&& data) {
    return std::make_shared<XData>(std::move(data));
}

/****************************************************************************/

const Object& Object::TRUE_OBJECT() {
    static Object *answer = new Object(true);
    return *answer;
}

const Object& Object::FALSE_OBJECT() {
    static Object *answer = new Object(false);
    return *answer;
}

const Object& Object::NULL_OBJECT() {
    static Object *answer = new Object();
    return *answer;
}

Object Object::NAN_OBJECT() {
    return Object(std::numeric_limits<double>::quiet_NaN());
}

Object Object::AUTO_OBJECT() {
    return Object(Dimension());
}

Object Object::EMPTY_ARRAY() {
    return Object(std::vector<Object>{});
}

Object Object::EMPTY_MUTABLE_ARRAY() {
    return Object(std::vector<Object>{}, true);
}

Object Object::EMPTY_MAP() {
    return Object(std::make_shared<ObjectMap>());
}

Object Object::EMPTY_MUTABLE_MAP() {
    return Object(std::make_shared<ObjectMap>(), true);
}

Object Object::ZERO_ABS_DIMEN() {
    return Object(Dimension(DimensionType::Absolute, 0));
}

Object Object::EMPTY_RECT() {
    return Object(Rect(0,0,0,0));
}

Object Object::EMPTY_RADII() {
    return Object(Radii());
}

Object Object::IDENTITY_2D() {
    return Object(Transform2D());
}

Object Object::LINEAR_EASING() {
    return Object(Easing::linear());
}

/****************************************************************************/

Object::Object(const Object& object) noexcept
        : mType(object.mType)
{
    switch (mType) {
        case kNullType:
            break;
        case kBoolType:
        case kNumberType:
        case kAbsoluteDimensionType:
        case kRelativeDimensionType:
        case kAutoDimensionType:
        case kColorType:
            mU.value = object.mU.value;
            break;
        case kStringType:
            new(&mU.string) std::string(object.mU.string);
            break;
        default:
            new(&mU.data) std::shared_ptr<ObjectData>(object.mU.data);
            break;
    }
}

Object::Object(Object&& object) noexcept
        : mType(object.mType)
{
    switch (mType) {
        case kNullType:
            break;
        case kBoolType:
        case kNumberType:
        case kAbsoluteDimensionType:
        case kRelativeDimensionType:
        case kAutoDimensionType:
        case kColorType:
            mU.value = object.mU.value;
            break;
        case kStringType:
            new(&mU.string) std::string(std::move(object.mU.string));
            break;
        default:
            new(&mU.data) std::shared_ptr<ObjectData>(std::move(object.mU.data));
            break;
    }
}

Object& Object::operator=(const Object& rhs) noexcept
{
    if (this == &rhs)
        return *this;

    if (mType == rhs.mType) {
        switch (mType) {
            case kNullType:
                break;
            case kBoolType:
            case kNumberType:
            case kAbsoluteDimensionType:
            case kRelativeDimensionType:
            case kAutoDimensionType:
            case kColorType:
                mU.value = rhs.mU.value;
                break;
            case kStringType:
                mU.string = rhs.mU.string;
                break;
            default:
                mU.data = rhs.mU.data;
                break;
        }
    }
    else {
        // Delete the old item
        switch (mType) {
            case kNullType:
            case kBoolType:
            case kNumberType:
            case kAbsoluteDimensionType:
            case kRelativeDimensionType:
            case kAutoDimensionType:
            case kColorType:
                break;
            case kStringType:
                mU.string.~basic_string<char>();
                break;
            default:
                mU.data.~shared_ptr<ObjectData>();
                break;
        }

        // Construct the new
        mType = rhs.mType;
        switch (mType) {
            case kNullType:
                break;
            case kBoolType:
            case kNumberType:
            case kAbsoluteDimensionType:
            case kRelativeDimensionType:
            case kAutoDimensionType:
            case kColorType:
                mU.value = rhs.mU.value;
                break;
            case kStringType:
                new(&mU.string) std::string(rhs.mU.string);
                break;
            default:
                new(&mU.data) std::shared_ptr<ObjectData>(rhs.mU.data);
                break;
        }
    }

    return *this;
}

Object& Object::operator=(Object&& rhs) noexcept
{
    if (mType == rhs.mType) {
        switch (mType) {
            case kNullType:
                break;
            case kBoolType:
            case kNumberType:
            case kAbsoluteDimensionType:
            case kRelativeDimensionType:
            case kAutoDimensionType:
            case kColorType:
                mU.value = std::move(rhs.mU.value);
                break;
            case kStringType:
                mU.string = std::move(rhs.mU.string);
                break;
            default:
                mU.data = std::move(rhs.mU.data);
                break;
        }
    }
    else {
        // Delete the old item
        switch (mType) {
            case kNullType:
            case kBoolType:
            case kNumberType:
            case kAbsoluteDimensionType:
            case kRelativeDimensionType:
            case kAutoDimensionType:
            case kColorType:
                break;
            case kStringType:
                mU.string.~basic_string<char>();
                break;
            default:
                mU.data.~shared_ptr<ObjectData>();
                break;
        }

        // Construct the new
        mType = rhs.mType;
        switch (mType) {
            case kNullType:
                break;
            case kBoolType:
            case kNumberType:
            case kAbsoluteDimensionType:
            case kRelativeDimensionType:
            case kAutoDimensionType:
            case kColorType:
                mU.value = std::move(rhs.mU.value);
                break;
            case kStringType:
                new(&mU.string) std::string(std::move(rhs.mU.string));
                break;
            default:
                new(&mU.data) std::shared_ptr<ObjectData>(std::move(rhs.mU.data));
                break;
        }
    }

    return *this;
}

/****************************************************************************/

Object::~Object()
{
    LOG_IF(OBJECT_DEBUG) << "  --- Destroying " << *this;
    switch (mType) {
        case kNullType:
        case kBoolType:
        case kNumberType:
        case kAbsoluteDimensionType:
        case kRelativeDimensionType:
        case kAutoDimensionType:
        case kColorType:
            break;
        case kStringType:
            mU.string.~basic_string<char>();
            break;
        default:
            mU.data.~shared_ptr<ObjectData>();
            break;
    }
}

Object::Object()
    : mType(kNullType)
{
    LOG_IF(OBJECT_DEBUG) << "Object null constructor" << this;
}

Object::Object(ObjectType type)
    : mType(type)
{
    LOG_IF(OBJECT_DEBUG) << "Object type constructor" << this;
}

Object::Object(bool b)
    : mType(kBoolType),
      mU(b ? 1.0 : 0.0)
{
    LOG_IF(OBJECT_DEBUG) << "Object bool constructor: " << b << " this=" << this;
}

Object::Object(int i)
    : mType(kNumberType),
      mU(i)
{}

Object::Object(uint32_t u)
    : mType(kNumberType),
      mU(u)
{}

Object::Object(unsigned long l)
    : mType(kNumberType),
      mU(static_cast<double>(l))
{}

Object::Object(long l)
    : mType(kNumberType),
      mU(static_cast<double>(l))
{}

Object::Object(unsigned long long l)
    : mType(kNumberType),
      mU(static_cast<double>(l))
{}

Object::Object(long long l)
    : mType(kNumberType),
      mU(static_cast<double>(l))
{}

Object::Object(double d)
    : mType(kNumberType),
      mU(d)
{}

Object::Object(const char *s)
    : mType(kStringType),
      mU(s)
{}

Object::Object(const std::string& s)
    : mType(kStringType),
      mU(s)
{}

Object::Object(const ObjectMapPtr& m, bool isMutable)
    : mType(kMapType),
      mU(std::static_pointer_cast<ObjectData>(std::make_shared<MapData>(m, isMutable)))
{}

Object::Object(const ObjectArrayPtr& v, bool isMutable)
    : mType(kArrayType),
      mU(std::static_pointer_cast<ObjectData>(std::make_shared<ArrayData>(v, isMutable)))
{}

Object::Object(ObjectArray&& v, bool isMutable)
    : mType(kArrayType),
      mU(std::static_pointer_cast<ObjectData>(std::make_shared<FixedArrayData>(std::move(v),isMutable)))
{}

Object::Object(const std::shared_ptr<datagrammar::ByteCode>& n)
    : mType(kByteCodeType),
      mU(std::static_pointer_cast<ObjectData>(n))
{
    LOG_IF(OBJECT_DEBUG) << "Object constructor compiled byte code: " << this;
}

Object::Object(const std::shared_ptr<datagrammar::BoundSymbol>& bs)
    : mType(kBoundSymbolType),
      mU(std::static_pointer_cast<ObjectData>(bs))
{
    LOG_IF(OBJECT_DEBUG) << "Object constructor bound symbol: " << this;
}

Object::Object(const std::shared_ptr<LiveDataObject>& d)
    : mType(d->getType()),
      mU(d)
{
    LOG_IF(OBJECT_DEBUG) << "Object live data: " << this;
}

Object::Object(const std::shared_ptr<ComponentEventWrapper>& d)
    : mType(kComponentType),
      mU(d)
{
    LOG_IF(OBJECT_DEBUG) << "Object component event data: " << this;
}

Object::Object(const std::shared_ptr<ContextWrapper>& c)
    : mType(kContextType),
      mU(c)
{
    LOG_IF(OBJECT_DEBUG) << "Object context: " << this;
}

Object::Object(const rapidjson::Value& value)
{
    LOG_IF(OBJECT_DEBUG) << "Object constructor value: " << this;

    switch(value.GetType()) {
    case rapidjson::kNullType:
        mType = kNullType;
        break;
    case rapidjson::kFalseType:
        mType = kBoolType;
        mU.value = 0;
        break;
    case rapidjson::kTrueType:
        mType = kBoolType;
        mU.value = 1;
        break;
    case rapidjson::kNumberType:
        mType = kNumberType;
        mU.value = value.GetDouble();
        break;
    case rapidjson::kStringType:
        mType = kStringType;
        new(&mU.string) std::string(value.GetString());  // TODO: Should we keep the string in place?
        break;
    case rapidjson::kObjectType:
        mType = kMapType;
        new(&mU.data) std::shared_ptr<ObjectData>(std::make_shared<JSONData>(&value));
        break;
    case rapidjson::kArrayType:
        mType = kArrayType;
        new(&mU.data) std::shared_ptr<ObjectData>(std::make_shared<JSONData>(&value));
        break;
    }
}

Object::Object(rapidjson::Document&& value)
{
    if (OBJECT_DEBUG) LOG(LogLevel::kDebug) << "Object constructor value: " << this;

    switch(value.GetType()) {
        case rapidjson::kNullType:
            mType = kNullType;
            break;
        case rapidjson::kFalseType:
            mType = kBoolType;
            mU.value = 0;
            break;
        case rapidjson::kTrueType:
            mType = kBoolType;
            mU.value = 1;
            break;
        case rapidjson::kNumberType:
            mType = kNumberType;
            mU.value = value.GetDouble();
            break;
        case rapidjson::kStringType:
            mType = kStringType;
            mU.string = value.GetString();  // TODO: Should we keep the string in place?
            break;
        case rapidjson::kObjectType:
            mType = kMapType;
            new(&mU.data) std::shared_ptr<ObjectData>(std::make_shared<JSONDocumentData>(std::move(value)));
            break;
        case rapidjson::kArrayType:
            mType = kArrayType;
            new(&mU.data) std::shared_ptr<ObjectData>(std::make_shared<JSONDocumentData>(std::move(value)));
            break;
    }
}

Object::Object(const std::shared_ptr<Function>& f)
    : mType(kFunctionType),
      mU(std::static_pointer_cast<ObjectData>(f))
{
    LOG_IF(OBJECT_DEBUG) << "User Function constructor";
}

Object::Object(const Dimension& d)
    : mType(d.isAuto() ? kAutoDimensionType :
            (d.isRelative() ? kRelativeDimensionType : kAbsoluteDimensionType)),
      mU(d.getValue())
{
    if (OBJECT_DEBUG)
        LOG(LogLevel::kDebug) << "Object dimension constructor: " << this << " is " << *this << " dimension=" << d;
}

Object::Object(const Color& color)
    : mType(kColorType),
      mU(color.get())
{
    LOG_IF(OBJECT_DEBUG) << "Object color constructor " << this;
}

Object::Object(Filter&& filter)
    : mType(DirectObjectData<Filter>::sType),
      mU(DirectObjectData<Filter>::create(std::move(filter)))
{
    LOG_IF(OBJECT_DEBUG) << "Object filter constructor " << this;
}

Object::Object(GraphicFilter&& graphicFilter)
        : mType(DirectObjectData<GraphicFilter>::sType),
          mU(DirectObjectData<GraphicFilter>::create(std::move(graphicFilter)))
{
    LOG_IF(OBJECT_DEBUG) << "Object graphic filter constructor " << this;
}

Object::Object(Gradient&& gradient)
    : mType(DirectObjectData<Gradient>::sType),
      mU(DirectObjectData<Gradient>::create(std::move(gradient)))
{
    LOG_IF(OBJECT_DEBUG) << "Object gradient constructor " << this;
}


Object::Object(MediaSource&& mediaSource)
    : mType(DirectObjectData<MediaSource>::sType),
      mU(DirectObjectData<MediaSource>::create(std::move(mediaSource)))
{
    LOG_IF(OBJECT_DEBUG) << "Object MediaSource constructor " << this;
}

Object::Object(Rect&& rect)
    : mType(DirectObjectData<Rect>::sType),
      mU(DirectObjectData<Rect>::create(std::move(rect)))
{
    LOG_IF(OBJECT_DEBUG) << "Object Rect constructor " << this;
}

Object::Object(Radii&& radii)
    : mType(DirectObjectData<Radii>::sType),
      mU(DirectObjectData<Radii>::create(std::move(radii)))
{
    LOG_IF(OBJECT_DEBUG) << "Object Radii constructor " << this;
}

Object::Object(StyledText&& styledText)
    : mType(DirectObjectData<StyledText>::sType),
      mU(DirectObjectData<StyledText>::create(std::move(styledText)))
{
    LOG_IF(OBJECT_DEBUG) << "Object StyledText constructor " << this;
}

Object::Object(const GraphicPtr& graphic)
    : mType(kGraphicType),
      mU(std::make_shared<GraphicData>(graphic))
{
    LOG_IF(OBJECT_DEBUG) << "Object Graphic constructor " << this;
}

Object::Object(const GraphicPatternPtr& graphicPattern)
    : mType(kGraphicPatternType),
      mU(graphicPattern)
{
    LOG_IF(OBJECT_DEBUG) << "Object GraphicPattern constructor " << this;
}

Object::Object(const std::shared_ptr<Transformation>& transform)
    : mType(kTransformType),
      mU(std::make_shared<TransformData>(transform))
{
    LOG_IF(OBJECT_DEBUG) << "Object transform constructor " << this;
}

Object::Object(Transform2D&& transform)
    : mType(DirectObjectData<Transform2D>::sType),
      mU(DirectObjectData<Transform2D>::create(std::move(transform)))
{
    LOG_IF(OBJECT_DEBUG) << "Object transform 2D constructor " << this;
}

Object::Object(const EasingPtr& easing)
    : mType(kEasingType),
      mU(std::static_pointer_cast<ObjectData>(easing))
{
    LOG_IF(OBJECT_DEBUG) << "Object easing constructor " << this;
}

Object::Object(const std::shared_ptr<AccessibilityAction>& accessibilityAction)
    : mType(kAccessibilityActionType),
      mU(std::static_pointer_cast<ObjectData>(accessibilityAction))
{
    LOG_IF(OBJECT_DEBUG) << "Object accessibility action constructor " << this;
}

Object::Object(const std::shared_ptr<RangeGenerator>& range)
    : mType(kArrayType),
      mU(std::static_pointer_cast<ObjectData>(range))
{
    LOG_IF(OBJECT_DEBUG) << "Object range generator " << this;
}

Object::Object(const std::shared_ptr<SliceGenerator>& range)
    : mType(kArrayType),
      mU(std::static_pointer_cast<ObjectData>(range))
{
    LOG_IF(OBJECT_DEBUG) << "Object slice generator " << this;
}

bool
Object::operator==(const Object& rhs) const
{
    LOG_IF(OBJECT_DEBUG) << "comparing " << *this << " to " << rhs;

    if (mType != rhs.mType)
        return false;

    switch (mType) {
        case kNullType:
        case kAutoDimensionType:
            return true;

        case kBoolType:
        case kNumberType:
        case kAbsoluteDimensionType:
        case kRelativeDimensionType:
        case kColorType:
            return mU.value == rhs.mU.value;

        case kStringType:
            return mU.string == rhs.mU.string;

        case kMapType: {
            if (mU.data->size() != rhs.mU.data->size())
                return false;

            auto left = mU.data->getMap();
            auto right = rhs.mU.data->getMap();
            for (auto &m : left) {
                auto it = right.find(m.first);
                if (it == right.end())
                    return false;
                if (m.second != it->second)
                    return false;
            }
            return true;
        }

        case kArrayType: {
            const auto len = mU.data->size();
            if (len != rhs.mU.data->size())
                return false;

            for (size_t i = 0 ; i < len ; i++)
                if (mU.data->at(i) != rhs.mU.data->at(i))
                    return false;

            return true;
        }

        case kByteCodeType:
        case kFunctionType:
            return mU.data == rhs.mU.data;

        case kEasingType:
            return *std::static_pointer_cast<Easing>(mU.data) ==
                   *std::static_pointer_cast<Easing>(rhs.mU.data);

        case kGradientType:
        case kFilterType:
        case kGraphicFilterType:
        case kMediaSourceType:
        case kRectType:
        case kRadiiType:
        case kTransform2DType:
        case kStyledTextType:
            return *(mU.data.get()) == *(rhs.mU.data.get());

        case kAccessibilityActionType:
            return *std::static_pointer_cast<AccessibilityAction>(mU.data) ==
                   *std::static_pointer_cast<AccessibilityAction>(rhs.mU.data);

        case kGraphicType:
            return mU.data == rhs.mU.data;
        case kGraphicPatternType:
            return mU.data == rhs.mU.data;
        case kTransformType:
            return mU.data == rhs.mU.data;
        case kBoundSymbolType:
            return *std::static_pointer_cast<datagrammar::BoundSymbol>(mU.data) ==
                   *std::static_pointer_cast<datagrammar::BoundSymbol>(rhs.mU.data);
        case kComponentType:
            return *std::static_pointer_cast<ComponentEventWrapper>(mU.data) ==
                   *std::static_pointer_cast<ComponentEventWrapper>(rhs.mU.data);
        case kContextType:
            return *std::static_pointer_cast<ContextWrapper>(mU.data) ==
                   *std::static_pointer_cast<ContextWrapper>(rhs.mU.data);
    }

    return false;  // Shouldn't ever get here
}

bool
Object::operator!=(const Object& rhs) const
{
    return !operator==(rhs);
}

bool
Object::isJson() const
{
    switch(mType) {
        case kMapType:
        case kArrayType:
            return mU.data->getJson() != nullptr;
        default:
            return false;
    }
}

/**
 * Return an attractively formatted double for display.
 * We drop trailing zeros for decimal numbers.  If the number is an integer or rounds
 * to an integer, we drop the decimal point as well.
 * Scientific notation numbers are not handled attractively.
 * @param value The value to format
 * @return A suitable string
 */
static inline std::string
doubleToString(double value)
{
    if (value < static_cast<double>(std::numeric_limits<std::int64_t>::max())
     && value > static_cast<double>(std::numeric_limits<std::int64_t>::min())) {
        auto iValue = static_cast<std::int64_t>(value);
        if (value == iValue)
            return std::to_string(iValue);
    }

    // TODO: Is this cheap enough to run each time?
    //       If so, we could unit test other languages more easily
    static char *separator = std::localeconv()->decimal_point;

    auto s = std::to_string(value);
    auto it = s.find_last_not_of('0');
    if (it != s.find(separator))   // Remove a trailing decimal point
        it++;
    s.erase(it, std::string::npos);
    return s;
}

/**
 * This method is used when coercing an object to a string.  This can be used
 * by an APL author to display information in a Text component, so we deliberately
 * do not return values for many of the internal object types.  Please use
 * Object::toDebugString() to return strings suitable for writing to the system log.
 *
 * @return The user-friendly value of the object as a string.
 */
std::string
Object::asString() const
{
    switch (mType) {
        case kNullType: return "";
        case kBoolType: return mU.value ? "true": "false";
        case kStringType: return mU.string;
        case kNumberType: return doubleToString(mU.value);
        case kAutoDimensionType: return "auto";
        case kAbsoluteDimensionType: return doubleToString(mU.value)+"dp";
        case kRelativeDimensionType: return doubleToString(mU.value)+"%";
        case kColorType: return Color(mU.value).asString();
        case kMapType: return "";
        case kArrayType: return "";
        case kByteCodeType: return "";
        case kFunctionType: return "";
        case kFilterType: return "";
        case kGraphicFilterType: return "";
        case kGradientType: return "";
        case kMediaSourceType: return "";
        case kRectType: return "";
        case kRadiiType: return "";
        case kStyledTextType: return as<StyledText>().asString();
        case kGraphicType: return "";
        case kGraphicPatternType: return "";
        case kTransformType: return "";
        case kTransform2DType: return "";
        case kEasingType: return "";
        case kBoundSymbolType: return "";
        case kComponentType: return "";
        case kContextType: return "";
        case kAccessibilityActionType: return "";
    }

    return "ERROR";  // TODO: Fix up the string type
}

inline double
stringToDouble(const std::string& string)
{
    try {
        auto len = string.size();
        auto idx = len;
        double result = std::stod(string, &idx);
        // Handle percentages.  We skip over whitespace and stop on any other character
        while (idx < len) {
            auto c = string[idx];
            if (c == '%') {
                result *= 0.01;
                break;
            }
            if (!std::isspace(c))
                break;
            idx++;
        }
        return result;
    } catch (...) {}
    return std::numeric_limits<double>::quiet_NaN();
}

double
Object::asNumber() const
{
    switch (mType) {
        case kBoolType:
        case kNumberType:
            return mU.value;
        case kStringType:
            return stringToDouble(mU.string);
        case kStyledTextType:
            return stringToDouble(as<StyledText>().asString());
        case kAbsoluteDimensionType:
            return mU.value;
        default:
            return std::numeric_limits<double>::quiet_NaN();
    }
}

int
Object::asInt(int base) const
{
    switch (mType) {
        case kBoolType:
            return static_cast<int>(mU.value);
        case kNumberType:
            return std::lround(mU.value);
        case kStringType:
            try { return std::stoi(mU.string, nullptr, base); } catch(...) {}
            return 0;
        case kStyledTextType:
            try { return std::stoi(as<StyledText>().asString(), nullptr, base); } catch(...) {}
            return 0;
        case kAbsoluteDimensionType:
            return std::lround(mU.value);
        default:
            return 0;
    }
}

std::int64_t
Object::asInt64(int base) const
{
    switch (mType) {
        case kBoolType:
            return static_cast<std::int64_t>(mU.value);
        case kNumberType:
            return std::llround(mU.value);
        case kStringType:
            try { return std::stoll(mU.string, nullptr, base); } catch(...) {}
            return 0;
        case kStyledTextType:
            try { return std::stoll(as<StyledText>().asString(), nullptr, base); } catch(...) {}
            return 0;
        case kAbsoluteDimensionType:
            return std::llround(mU.value);
        default:
            return 0;
    }
}

/// @deprecated
Color
Object::asColor() const
{
    return asColor(nullptr);
}

Color
Object::asColor(const SessionPtr& session) const
{
    switch (mType) {
        case kNumberType:
        case kColorType:
            return Color(mU.value);
        case kStringType:
            return Color(session, mU.string);
        case kStyledTextType:
            return Color(session, as<StyledText>().asString());
        default:
            return Color();  // Transparent
    }
}

Color
Object::asColor(const Context& context) const
{
    return asColor(context.session());
}

Dimension
Object::asDimension(const Context& context) const
{
    switch (mType) {
        case kNumberType:
            return Dimension(DimensionType::Absolute, mU.value);
        case kStringType:
            return Dimension(context, mU.string);
        case kAbsoluteDimensionType:
            return Dimension(DimensionType::Absolute, mU.value);
        case kRelativeDimensionType:
            return Dimension(DimensionType::Relative, mU.value);
        case kAutoDimensionType:
            return Dimension(DimensionType::Auto, 0);
        case kStyledTextType:
            return Dimension(context, as<StyledText>().asString());
        default:
            return Dimension(DimensionType::Absolute, 0);
    }
}

Dimension
Object::asAbsoluteDimension(const Context& context) const
{
    switch (mType) {
        case kNumberType:
            return Dimension(DimensionType::Absolute, mU.value);
        case kStringType: {
            auto d = Dimension(context, mU.string);
            return (d.getType() == DimensionType::Absolute ? d : Dimension(DimensionType::Absolute, 0));
        }
        case kStyledTextType: {
            auto d = Dimension(context, as<StyledText>().asString());
            return (d.getType() == DimensionType::Absolute ? d : Dimension(DimensionType::Absolute, 0));
        }
        case kAbsoluteDimensionType:
            return Dimension(DimensionType::Absolute, mU.value);
        default:
            return Dimension(DimensionType::Absolute, 0);
    }
}

Dimension
Object::asNonAutoDimension(const Context& context) const
{
    switch (mType) {
        case kNumberType:
            return Dimension(DimensionType::Absolute, mU.value);
        case kStringType: {
            auto d = Dimension(context, mU.string);
            return (d.getType() == DimensionType::Auto ? Dimension(DimensionType::Absolute, 0) : d);
        }
        case kStyledTextType: {
            auto d = Dimension(context, as<StyledText>().asString());
            return (d.getType() == DimensionType::Auto ? Dimension(DimensionType::Absolute, 0) : d);
        }
        case kAbsoluteDimensionType:
            return Dimension(DimensionType::Absolute, mU.value);
        case kRelativeDimensionType:
            return Dimension(DimensionType::Relative, mU.value);
        default:
            return Dimension(DimensionType::Absolute, 0);
    }
}

Dimension
Object::asNonAutoRelativeDimension(const Context& context) const
{
    switch (mType) {
        case kNumberType:
            return Dimension(DimensionType::Relative, mU.value * 100);
        case kStringType: {
            auto d = Dimension(context, mU.string, true);
            return (d.getType() == DimensionType::Auto ? Dimension(DimensionType::Relative, 0) : d);
        }
        case kStyledTextType: {
            auto d = Dimension(context, as<StyledText>().asString(), true);
            return (d.getType() == DimensionType::Auto ? Dimension(DimensionType::Relative, 0) : d);
        }
        case kAbsoluteDimensionType:
            return Dimension(DimensionType::Absolute, mU.value);
        case kRelativeDimensionType:
            return Dimension(DimensionType::Relative, mU.value);
        default:
            return Dimension(DimensionType::Relative, 0);
    }
}

std::shared_ptr<Function>
Object::getFunction() const
{
    assert(mType == kFunctionType);
    return std::static_pointer_cast<Function>(mU.data);
}

std::shared_ptr<datagrammar::BoundSymbol>
Object::getBoundSymbol() const
{
    assert(mType == kBoundSymbolType);
    return std::static_pointer_cast<datagrammar::BoundSymbol>(mU.data);
}

std::shared_ptr<LiveDataObject>
Object::getLiveDataObject() const
{
    assert(mType == kArrayType || mType == kMapType);
    return std::dynamic_pointer_cast<LiveDataObject>(mU.data);
}

std::shared_ptr<datagrammar::ByteCode>
Object::getByteCode() const
{
    assert(mType == kByteCodeType);
    return std::static_pointer_cast<datagrammar::ByteCode>(mU.data);
}

std::shared_ptr<AccessibilityAction>
Object::getAccessibilityAction() const {
    assert(mType == kAccessibilityActionType);
    return std::static_pointer_cast<AccessibilityAction>(mU.data);
}

const ObjectMap&
Object::getMap() const {
    assert(isMap()); return mU.data->getMap();
}

ObjectMap&
Object::getMutableMap() {
    assert(mType == kMapType); return mU.data->getMutableMap();
}

const ObjectArray&
Object::getArray() const {
    assert(mType == kArrayType); return mU.data->getArray();
}

ObjectArray&
Object::getMutableArray() {
    assert(mType == kArrayType); return mU.data->getMutableArray();
}


const Filter&
Object::getFilter() const {
    return as<Filter>();
}

const GraphicFilter&
Object::getGraphicFilter() const {
    return as<GraphicFilter>();
}

const Gradient&
Object::getGradient() const {
    return as<Gradient>();
}

const MediaSource&
Object::getMediaSource() const {
    return as<MediaSource>();
}

GraphicPtr
Object::getGraphic() const {
    assert(mType == kGraphicType); return mU.data->getGraphic();
}

GraphicPatternPtr
Object::getGraphicPattern() const {
    assert(mType == kGraphicPatternType); return std::static_pointer_cast<GraphicPattern>(mU.data);
}

Rect
Object::getRect() const {
    return as<Rect>();
}

Radii
Object::getRadii() const {
    return as<Radii>();
}

const StyledText&
Object::getStyledText() const {
    return as<StyledText>();
}

std::shared_ptr<Transformation>
Object::getTransformation() const {
    assert(mType == kTransformType); return mU.data->getTransform();
}

Transform2D
Object::getTransform2D() const {
    return as<Transform2D>();
}

EasingPtr
Object::getEasing() const {
    assert(mType == kEasingType);
    return std::static_pointer_cast<Easing>(mU.data);
}

const rapidjson::Value&
Object::getJson() const {
    assert(isJson());
    return *(mU.data->getJson());
}


bool
Object::truthy() const
{
    switch (mType) {
        case kNullType:
            return false;
        case kBoolType:
        case kNumberType:
            return mU.value != 0;
        case kStringType:
            return mU.string.size() != 0;
        case kArrayType:
        case kMapType:
        case kByteCodeType:
        case kFunctionType:
            return true;
        case kAbsoluteDimensionType:
        case kRelativeDimensionType:
            return mU.value != 0;
        case kAutoDimensionType:
        case kColorType:
            return true;

        case kFilterType:
        case kGraphicFilterType:
        case kGradientType:
        case kMediaSourceType:
        case kEasingType:
        case kRectType:
        case kRadiiType:
        case kTransform2DType:
        case kStyledTextType:
        case kGraphicPatternType:
            return mU.data->truthy();

        case kGraphicType:
            return true;
        case kTransformType:
            return true;
        case kBoundSymbolType:
            return true;
        case kComponentType:
            return std::static_pointer_cast<ComponentEventWrapper>(mU.data)->getComponent() != nullptr;
        case kContextType:
        case kAccessibilityActionType:
            return mU.data->truthy();
    }

    // Should never be reached.
    return true;
}

// Methods for MAP objects
Object
Object::get(const std::string& key) const
{
    assert(mType == kMapType || mType == kComponentType || mType == kContextType);
    return mU.data->get(key);
}

bool
Object::has(const std::string& key) const
{
    assert(mType == kMapType || mType == kComponentType || mType == kContextType);
    return mU.data->has(key);
}

Object
Object::opt(const std::string& key, const Object& def) const
{
    assert(mType == kMapType || mType == kComponentType || mType == kContextType);
    return mU.data->opt(key, def);
}

// Methods for ARRAY objects
Object
Object::at(std::uint64_t index) const
{
    assert(mType == kArrayType);
    return mU.data->at(index);
}

Object::ObjectType
Object::getType() const
{
    return mType;
}


std::uint64_t
Object::size() const
{
    switch (mType) {
        case kArrayType:
        case kMapType:
        case kComponentType:
        case kContextType:
        case kGraphicPatternType:
            return mU.data->size();
        case kStringType:
            return mU.string.size();
        case kStyledTextType:
            return as<StyledText>().asString().size();  // Size of the raw text
        default:
            return 0;
    }
}

bool
Object::empty() const
{
    switch (mType) {
        case kNullType:
            return true;
        case kArrayType:
        case kMapType:
        case kRectType:
        case kGraphicPatternType:
            return mU.data->empty();
        case kStringType:
            return mU.string.empty();
        case kStyledTextType:
            return mU.data->empty();
        default:
            return false;
    }
}

bool
Object::isMutable() const
{
    switch (mType) {
        case kArrayType:
        case kMapType:
            return mU.data->isMutable();
        default:
            return false;
    }
}

Object
Object::eval() const
{
    return (mType == kByteCodeType || mType == kBoundSymbolType) ? mU.data->eval() : *this;
}

/**
 * Internal visitor class used to check if an equation is "pure" - that is, if the result
 * of the equation does not change each time you calculate it.
 */
class PureVisitor : public Visitor<Object> {
public:
    void visit(const Object& object) override {
        if (object.isFunction() && !object.getFunction()->isPure())
            mIsPure = false;
    }

    bool isPure() const { return mIsPure; }
    bool isAborted() const override { return !mIsPure; }  // Abort if the visitor has found a non-pure value

private:
    bool mIsPure = true;
};

bool
Object::isPure() const
{
    PureVisitor visitor;
    accept(visitor);
    return visitor.isPure();
}

const bool DEBUG_SYMBOL_VISITOR = false;

/**
 * Internal visitor class used to extract all symbols and symbol paths from within
 * an equation.
 */
class SymbolVisitor : public Visitor<Object> {
public:
    SymbolVisitor(SymbolReferenceMap& map) : mMap(map) {}

    /**
     * Visit an individual object.  At the end of this visit, the mCurrentSuffix
     * should be set to a valid suffix (either a continuation of the parent or empty).
     * @param object The object to visit
     */
    void visit(const Object& object) override {
        LOG_IF(DEBUG_SYMBOL_VISITOR) << object.toDebugString()
                                     << " mParentSuffix=" << mParentSuffix
                                     << " mIndex=" << mIndex;

        mCurrentSuffix.clear();  // In the majority of cases there will be no suffix

        if (object.isBoundSymbol()) {  // A bound symbol should be added to the map with existing suffixes
            auto symbol = object.getBoundSymbol()->getSymbol();
            if (symbol.second)  // An invalid bound symbol will not have a context
                mMap.emplace(symbol.first + (mIndex == 0 ? mParentSuffix : ""), symbol.second);
        }
        else if (object.isByteCode()) {
            object.symbols(mMap);
        }

        mIndex++;
    }

    /**
     * Move down to the child nodes below the current node.  Stash information on the
     * stack so we can recover state
     */
    void push() override {
        mStack.push({mIndex, mParentSuffix});
        mParentSuffix = mCurrentSuffix;
        mIndex = 0;
    }

    /**
     * Pop up one level, restoring the state
     */
    void pop() override {
        const auto& ref = mStack.top();
        mIndex = ref.first;
        mParentSuffix = ref.second;
        mStack.pop();
    }

private:
    SymbolReferenceMap& mMap;
    int mIndex = 0;              // The index of the child being visited.
    std::string mParentSuffix;   // The suffix created by the parent of this object
    std::string mCurrentSuffix;  // The suffix calculated visiting the current object
    std::stack<std::pair<int, std::string>> mStack;  // Old indexes and parent suffixes
};

void
Object::symbols(SymbolReferenceMap& symbols) const
{
    if (mType == kByteCodeType)
        std::dynamic_pointer_cast<datagrammar::ByteCode>(getByteCode())->symbols(symbols);
    else {
        SymbolVisitor visitor(symbols);
        accept(visitor);
    }
}

Object
Object::call(const ObjectArray& args) const
{
    assert(mType == kFunctionType || mType == kEasingType);
    LOG_IF(OBJECT_DEBUG) << "Calling user function";
    return mU.data->call(args);
}

// Visitor pattern
void
Object::accept(Visitor<Object>& visitor) const
{
    visitor.visit(*this);
    if (!visitor.isAborted() && (mType == kArrayType || mType == kMapType))
        mU.data->accept(visitor);
}

rapidjson::Value
Object::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    switch (mType) {
        case kNullType:
            return rapidjson::Value();
        case kBoolType:
            return rapidjson::Value(static_cast<bool>(mU.value));
        case kNumberType:
            return std::isfinite(mU.value) ? rapidjson::Value(mU.value) : rapidjson::Value();
        case kStringType:
            return rapidjson::Value(mU.string.c_str(), allocator);
        case kArrayType: {
            rapidjson::Value v(rapidjson::kArrayType);
            for (int i = 0 ; i < size() ; i++)
                v.PushBack(at(i).serialize(allocator), allocator);
            return v;
        }
        case kMapType: {
            rapidjson::Value m(rapidjson::kObjectType);
            for (auto &kv : mU.data->getMap())
                m.AddMember(rapidjson::Value(kv.first.c_str(), allocator), kv.second.serialize(allocator).Move(), allocator);
            return m;
        }
        case kByteCodeType:
            return rapidjson::Value("UNABLE TO SERIALIZE COMPILED BYTE CODE", allocator);
        case kFunctionType:
            return rapidjson::Value("UNABLE TO SERIALIZE FUNCTION", allocator);
        case kAbsoluteDimensionType:
            return rapidjson::Value(std::isfinite(mU.value) ? mU.value : 0);
        case kRelativeDimensionType:
            return rapidjson::Value((doubleToString(mU.value)+"%").c_str(), allocator);
        case kAutoDimensionType:
            return rapidjson::Value("auto", allocator);
        case kColorType:
            return rapidjson::Value(asString().c_str(), allocator);
        case kFilterType:
        case kGraphicFilterType:
        case kGradientType:
        case kMediaSourceType:
        case kRectType:
        case kRadiiType:
        case kEasingType:
        case kTransform2DType:
        case kStyledTextType:
        case kGraphicPatternType:
            return mU.data->serialize(allocator);
        case kGraphicType:
            return getGraphic()->serialize(allocator);
        case kTransformType:
            return rapidjson::Value("UNABLE TO SERIALIZE TRANSFORM", allocator);
        case kBoundSymbolType:
            return rapidjson::Value("UNABLE TO SERIALIZE BOUND SYMBOL", allocator);
        case kComponentType:
            return std::static_pointer_cast<ComponentEventWrapper>(mU.data)->serialize(allocator);
        case kContextType:
        case kAccessibilityActionType:
            return mU.data->serialize(allocator);
    }

    return rapidjson::Value();  // Never should be reached
}

rapidjson::Value
Object::serializeDirty(rapidjson::Document::AllocatorType& allocator) const
{
    switch (mType) {
        case kGraphicType:
            // TODO: Fix this - should return just the dirty bits
            return serialize(allocator);
            break;
        default:
            return serialize(allocator);
    }
}

template<typename T> const T& Object::as() const {
    assert(mType == DirectObjectData<T>::sType);
    return *static_cast<const T*>(mU.data->inner());
}

std::string
Object::toDebugString() const
{
    switch (mType) {
        case Object::kNullType:
            return "null";
        case Object::kBoolType:
            return (mU.value ? "true" : "false");
        case Object::kNumberType:
            return std::to_string(mU.value);
        case Object::kStringType:
            return "'" + mU.string + "'";
        case Object::kMapType:
        case Object::kArrayType:
        case Object::kByteCodeType:
        case Object::kFunctionType:
            return mU.data->toDebugString();
        case Object::kAbsoluteDimensionType:
            return "AbsDim<" + std::to_string(mU.value) + ">";
        case Object::kRelativeDimensionType:
            return "RelDim<" + std::to_string(mU.value) + ">";
        case Object::kAutoDimensionType:
            return "AutoDim";
        case Object::kColorType:
            return asString();
        case Object::kFilterType:
        case Object::kGraphicFilterType:
        case Object::kGradientType:
        case Object::kMediaSourceType:
        case Object::kRectType:
        case Object::kRadiiType:
        case Object::kStyledTextType:
        case Object::kGraphicType:
        case Object::kGraphicPatternType:
        case Object::kTransformType:
        case Object::kTransform2DType:
        case Object::kEasingType:
        case Object::kBoundSymbolType:
        case Object::kComponentType:
        case Object::kContextType:
        case Object::kAccessibilityActionType:
            return mU.data->toDebugString();
    }
    return "";
}

streamer&
operator<<(streamer& os, const Object& object)
{
    os << object.toDebugString();
    return os;
}

}  // namespace apl
