/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <cmath>
#include <clocale>

#include "apl/datagrammar/node.h"
#include "apl/graphic/graphic.h"
#include "apl/primitives/color.h"
#include "apl/primitives/dimension.h"
#include "apl/primitives/filter.h"
#include "apl/primitives/object.h"
#include "apl/primitives/mediasource.h"
#include "apl/primitives/transform.h"
#include "apl/utils/log.h"

namespace apl {

const bool OBJECT_DEBUG = false;

/****************************************************************************/

class ArrayData : public Object::Data {
public:
    ArrayData(const SharedVectorPtr& array) : mArray(array) {}

    const Object
    at(size_t index) const override
    {
        size_t len = mArray->size();
        if (index >= len)
            return Object::NULL_OBJECT();

        return mArray->at(index);
    }

    size_t size() const override { return mArray->size(); }
    bool empty() const override { return mArray->empty(); }

    void
    accept(Visitor<Object>& visitor) const override
    {
        visitor.push();
        for (auto it = mArray->begin() ; it != mArray->end() ; it++)
            it->accept(visitor);
        visitor.pop();
    }

    const std::vector<Object>& getArray() const override {
        return *mArray;
    }

    void symbols(std::set<std::string>& symbols) const override {
        if (mArray) {
            for (auto& m : *mArray)
                m.symbols(symbols);
        }
    }

    std::string toDebugString() const override {
        return "Array<size=" + std::to_string(mArray->size()) + ">";
    }

private:
    std::shared_ptr<std::vector<Object> > mArray;
};

/****************************************************************************/

class FixedArrayData : public Object::Data {
public:
    FixedArrayData(const std::vector<Object>&& array) : mArray(array) {}

    const Object
    at(size_t index) const override
    {
        size_t len = mArray.size();
        if (index >= len)
            return Object::NULL_OBJECT();

        return mArray.at(index);
    }

    size_t size() const override {
        return mArray.size();
    }

    bool empty() const override {
        return mArray.empty();
    }

    void
    accept(Visitor<Object>& visitor) const override
    {
        visitor.push();
        for (const auto& element : mArray)
            element.accept(visitor);
        visitor.pop();
    }

    const std::vector<Object>& getArray() const override {
        return mArray;
    }

    void symbols(std::set<std::string>& symbols) const override {
        for (auto& m : mArray)
            m.symbols(symbols);
    }

    std::string toDebugString() const override {
        return "FixedArray<size=" + std::to_string(mArray.size()) + ">";
    }

private:
    const std::vector<Object> mArray;
};

/****************************************************************************/

class MapData : public Object::Data {
public:
    MapData(const Object object) : mMap() {};

    MapData(const std::shared_ptr<ObjectMap>& map) : mMap(map) {}

    const Object
    get(const std::string& key) const override
    {
        auto it = mMap->find(key);
        if (it != mMap->end())
            return it->second;
        return Object::NULL_OBJECT();
    }

    size_t size() const override { return mMap->size(); }
    bool empty() const override { return mMap->empty(); }

    bool has(const std::string& key) const override { return mMap->count(key) != 0; }

    const std::map<std::string, Object>& getMap() const override {
        return *mMap;
    }

    void
    accept(Visitor<Object>& visitor) const override
    {
        visitor.push();
        for (auto m : *mMap) {
            Object(m.first).accept(visitor);
            visitor.push();
            m.second.accept(visitor);
            visitor.pop();
        }
        visitor.pop();
    }

    std::string toDebugString() const override {
        return "Map<size=" + std::to_string(mMap->size()) + ">";
    }
private:
    SharedMapPtr mMap;
};


/****************************************************************************/

class JSONData : public Object::Data {
public:
    JSONData(const rapidjson::Value *value) : mValue(value) {}

    const Object
    get(const std::string& key) const override
    {
        if (mValue->IsObject()) {
            auto it = mValue->FindMember(key.c_str());
            if (it != mValue->MemberEnd())
                return it->value;
        }
        return Object::NULL_OBJECT();
    }

    bool
    has(const std::string& key) const override {
        if (!mValue->IsObject())
            return false;

        return mValue->FindMember(key.c_str()) != mValue->MemberEnd();
    }

    const Object
    at(size_t index) const override {
        if (!mValue->IsArray() || index >= mValue->Size())
            return Object::NULL_OBJECT();
        return (*mValue)[index];
    }

    size_t
    size() const override {
        if (mValue->IsArray())
            return mValue->Size();
        if (mValue->IsObject())
            return mValue->MemberCount();
        return 0;
    }

    bool
    empty() const override {
        if (mValue->IsArray())
            return mValue->Empty();
        if (mValue->IsObject())
            return mValue->MemberCount() == 0;
        return false;
    }

    const std::vector<Object>& getArray() const override {
        assert(mValue->IsArray());

        if (mValue->Size() != mVector.size())
            for (const auto& v : mValue->GetArray())
                const_cast<std::vector<Object>&>(mVector).emplace_back(v);
        return mVector;
    }

    const std::map<std::string, Object>& getMap() const override {
        assert(mValue->IsObject());

        if (mValue->MemberCount() != mMap.size())
            for (const auto& v : mValue->GetObject())
                const_cast<std::map<std::string, Object>&>(mMap).emplace(v.name.GetString(), v.value);
        return mMap;
    }

    const rapidjson::Value* getJson() const override {
        return mValue;
    }

    std::string toDebugString() const override {
        return "JSON<size=" + std::to_string(size()) + ">";
    }

private:
    const rapidjson::Value *mValue;
    const std::map<std::string, Object> mMap;
    const std::vector<Object> mVector;
};

/****************************************************************************/

class FunctionData : public Object::Data {
public:
    FunctionData(Object (*f)(const std::vector<Object>&)) : mFunction(f) {};

    Object
    call(const std::vector<Object>& args) const override {
        return mFunction(args);
    }

    std::string toDebugString() const override {
        return "Function<>";
    }

private:
    std::function<Object(const std::vector<Object>&)> mFunction;
};

/****************************************************************************/

class FilterData : public Object::Data {
public:
    FilterData(const Filter&& filter) : mFilter(std::move(filter)) {}
    const Filter& getFilter() const override { return mFilter; }

    std::string toDebugString() const override {
        return "Filter<>";
    }

private:
    const Filter mFilter;
};

/****************************************************************************/

class GradientData : public Object::Data {
public:
    GradientData(const Gradient&& gradient) : mGradient(std::move(gradient)) {}
    const Gradient& getGradient() const override { return mGradient; }

    std::string toDebugString() const override {
        return "Gradient<>";
    }

private:
    const Gradient mGradient;
};


/****************************************************************************/

class MediaSourceData : public Object::Data {
public:
    MediaSourceData(const MediaSource&& mediaSource) : mMediaSource(std::move(mediaSource)) {}

    const MediaSource& getMediaSource() const override { return mMediaSource; }

    std::string toDebugString() const override {
        return "MediaSource<>";
    }

private:
    const MediaSource mMediaSource;
};

/****************************************************************************/

class RectData : public Object::Data {
public:
    RectData(Rect&& rect) : mRect(std::move(rect)) {}
    Rect getRect() const override { return mRect; }

    bool empty() const override { return mRect.isEmpty(); }

    std::string toDebugString() const override {
        return "Rect<"+mRect.toString()+">";
    }
private:
    Rect mRect;
};

/****************************************************************************/

class RadiiData : public Object::Data {
public:
    RadiiData(Radii&& radii) : mRadii(std::move(radii)) {}
    Radii getRadii() const override { return mRadii; }

    std::string toDebugString() const override {
        return "Radii<"+mRadii.toString()+">";
    }
private:
    Radii mRadii;
};

/****************************************************************************/

class StyledTextData : public Object::Data {
public:
    StyledTextData(StyledText&& styledText) : mStyledText(std::move(styledText)) {}

    const StyledText& getStyledText() const override {
        return mStyledText;
    }

    std::string toDebugString() const override {
        return "StyledText<"+mStyledText.asString()+">";
    }

private:
    const StyledText mStyledText;
};

/****************************************************************************/

class GraphicData : public Object::Data {
public:
    GraphicData(const GraphicPtr graphic) : mGraphic(graphic) {}
    GraphicPtr getGraphic() const override { return mGraphic; }

    std::string toDebugString() const override {
        return "Graphic<>";
    }

private:
    const GraphicPtr mGraphic;
};

/****************************************************************************/

class TransformData : public Object::Data {
public:
    TransformData(const std::shared_ptr<Transformation> transform) : mTransform(transform) {}
    std::shared_ptr<Transformation> getTransform() const override { return mTransform; }

    std::string toDebugString() const override {
        return "Transform<>";
    }

private:
    std::shared_ptr<Transformation> mTransform;
};

/****************************************************************************/

class Transform2DData : public Object::Data {
public:
    Transform2DData(Transform2D&& transform) : mTransform(std::move(transform)) {}
    Transform2D getTransform2D() const override { return mTransform; }

    std::string toDebugString() const override {
        streamer s;
        s << mTransform;
        return "Transform2D<"+s.str()+">";
    }

private:
    const Transform2D mTransform;
};

/****************************************************************************/

class EasingData : public Object::Data {
public:
    EasingData(Easing&& easing) : mEasing(std::move(easing)) {}
    Easing getEasing() const override { return mEasing; }

    std::string toDebugString() const override {
        return "Easing<>";
    }

private:
    const Easing mEasing;
};

/****************************************************************************/

class AnimationData : public Object::Data {
public:
    AnimationData(const Animation&& Animation) : mAnimation(std::move(Animation)) {}
    const Animation& getAnimation() const override { return mAnimation; }

    std::string toDebugString() const override {
        return "Animation<>";
    }

private:
    const Animation mAnimation;
};
/****************************************************************************/

template<typename XData, typename T>
std::shared_ptr<Object::Data> create(T&& data) {
    return std::make_shared<XData>(std::move(data));
}

/****************************************************************************/

Object& Object::TRUE() {
    static Object* ans = new Object(true);
    return *ans;
}

Object& Object::FALSE() {
    static Object* ans = new Object(false);
    return *ans;
}

Object& Object::NULL_OBJECT() {
    static Object* ans = new Object();
    return *ans;
}

Object& Object::NAN_OBJECT() {
    static Object *ans = new Object(std::numeric_limits<double>::quiet_NaN());
    return *ans;
}

Object& Object::AUTO_OBJECT() {
    static Object *ans = new Object(Dimension());
    return *ans;
}

Object& Object::EMPTY_ARRAY() {
    static Object *ans = new Object(std::vector<Object>{});
    return *ans;
}

Object& Object::ZERO_ABS_DIMEN() {
    static Object *ans = new Object(Dimension(DimensionType::Absolute, 0));
    return *ans;
}

Object& Object::EMPTY_RECT() {
    static Object *ans = new Object(Rect(0,0,0,0));
    return *ans;
}

Object& Object::EMPTY_RADII() {
    static Object *ans = new Object(Radii());
    return *ans;
}

Object& Object::IDENTITY_2D() {
    static Object *ans = new Object(Transform2D());
    return *ans;
}

Object& Object::LINEAR_EASING() {
    static Object *ans = new Object(Easing::linear());
    return *ans;
}

/****************************************************************************/

Object::~Object() {
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "  --- Destroying " << *this;
}

Object::Object()
    : mType(kNullType)
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object null construtor" << this;
}

Object::Object(ObjectType type)
    : mType(type)
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object type construtor" << this;
}

Object::Object(bool b)
    : mType(kBoolType),
      mValue(b ? 1 : 0)
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object bool constructor: " << b << " this=" << this;
}

Object::Object(int i)
    : mType(kNumberType),
      mValue(i)
{}

Object::Object(uint32_t u)
    : mType(kNumberType),
      mValue(u)
{}

Object::Object(unsigned long l)
    : mType(kNumberType),
      mValue(l)
{}

Object::Object(double d)
    : mType(kNumberType),
      mValue(d)
{}

Object::Object(const char *s)
    : mType(kStringType),
      mString(s)
{}

Object::Object(const std::string& s)
    : mType(kStringType),
      mString(s)
{}

Object::Object(const SharedMapPtr& m)
    : mType(kMapType),
      mData(std::static_pointer_cast<Data>(std::make_shared<MapData>(m)))
{}

Object::Object(const SharedVectorPtr& v)
    : mType(kArrayType),
      mData(std::static_pointer_cast<Data>(std::make_shared<ArrayData>(v)))
{}

Object::Object(std::vector<Object>&& v)
    : mType(kArrayType),
      mData(std::static_pointer_cast<Data>(std::make_shared<FixedArrayData>(std::move(v))))
{}

Object::Object(const std::shared_ptr<datagrammar::Node>& n)
    : mType(kNodeType),
      mData(std::static_pointer_cast<Data>(n))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object constructor node: " << this;
}

Object::Object(const rapidjson::Value& value)
    : mValue(0)
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object constructor value: " << this;

    switch(value.GetType()) {
    case rapidjson::kNullType:
        mType = kNullType;
        break;
    case rapidjson::kFalseType:
        mType = kBoolType;
        mValue = 0;
        break;
    case rapidjson::kTrueType:
        mType = kBoolType;
        mValue = 1;
        break;
    case rapidjson::kNumberType:
        mType = kNumberType;
        mValue = value.GetDouble();
        break;
    case rapidjson::kStringType:
        mType = kStringType;
        mString = value.GetString();  // TODO: Should we keep the string in place?
        break;
    case rapidjson::kObjectType:
        mType = kMapType;
        mData = std::static_pointer_cast<Data>(std::make_shared<JSONData>(&value));
        break;
    case rapidjson::kArrayType:
        mType = kArrayType;
        mData = std::static_pointer_cast<Data>(std::make_shared<JSONData>(&value));
        break;
    }
}

Object::Object(UserFunction f)
    : mType(kFunctionType),
      mData(std::static_pointer_cast<Data>(std::make_shared<FunctionData>(f)))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "User Function constructor";
}

Object::Object(const Dimension& d)
    : mType(d.isAuto() ? kAutoDimensionType :
            (d.isRelative() ? kRelativeDimensionType : kAbsoluteDimensionType)),
      mValue(d.getValue())
{
    if (OBJECT_DEBUG)
        LOG(LogLevel::DEBUG) << "Object dimension constructor: " << this << " is " << *this << " dimension=" << d;
}

Object::Object(const Color& color)
    : mType(kColorType),
      mValue(color.get())
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object color constructor " << this;
}

Object::Object(Filter&& filter)
    : mType(kFilterType),
      mData(create<FilterData, Filter>(std::move(filter)))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object filter constructor " << this;
}

Object::Object(Gradient&& gradient)
    : mType(kGradientType),
      mData(create<GradientData, Gradient>(std::move(gradient)))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object gradient constructor " << this;
}


Object::Object(MediaSource&& mediaSource)
    : mType(kMediaSourceType),
      mData(create<MediaSourceData, MediaSource>(std::move(mediaSource)))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object MediaSource constructor " << this;
}

Object::Object(Rect&& rect)
    : mType(kRectType),
      mData(create<RectData, Rect>(std::move(rect)))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object Rect constructor " << this;
}

Object::Object(Radii&& radii)
    : mType(kRadiiType),
      mData(create<RadiiData, Radii>(std::move(radii)))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object Radii constructor " << this;
}

Object::Object(StyledText&& styledText)
    : mType(kStyledTextType),
      mData(create<StyledTextData, StyledText>(std::move(styledText)))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object StyledText constructor " << this;
}

Object::Object(const GraphicPtr& graphic)
    : mType(kGraphicType),
      mData(std::make_shared<GraphicData>(graphic))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object Graphic constructor " << this;
}

Object::Object(const std::shared_ptr<Transformation>& transform)
    : mType(kTransformType),
      mData(std::make_shared<TransformData>(transform))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object transform constructor " << this;
}

Object::Object(Transform2D&& transform)
    : mType(kTransform2DType),
      mData(create<Transform2DData, Transform2D>(std::move(transform)))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object transform 2D constructor " << this;
}

Object::Object(Easing&& easing)
    : mType(kEasingType),
      mData(create<EasingData, Easing>(std::move(easing)))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object easing constructor " << this;
}

Object::Object(Animation&& animation)
    : mType(kAnimationType),
      mData(create<AnimationData, Animation>(std::move(animation)))
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Object animation constructor " << this;
}

bool
Object::operator==(const Object& rhs) const
{
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "comparing " << *this << " to " << rhs;

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
            return mValue == rhs.mValue;

        case kStringType:
            return mString == rhs.mString;

        case kMapType: {
            if (mData->size() != rhs.mData->size())
                return false;

            auto left = mData->getMap();
            auto right = rhs.mData->getMap();
            for (auto& m : left) {
                auto it = right.find(m.first);
                if (it == right.end())
                    return false;
                if (m.second != it->second)
                    return false;
            }
            return true;
        }

        case kArrayType: {
            if (mData->size() != rhs.mData->size())
                return false;

            auto left = mData->getArray();
            auto right = rhs.mData->getArray();
            for (int i = 0 ; i < mData->size() ; i++)
                if (left.at(i) != right.at(i))
                    return false;

            return true;
        }

        case kNodeType:
        case kFunctionType:
        case kGradientType:
            return mData == rhs.mData;
        case kFilterType:
            return mData->getFilter() == rhs.mData->getFilter();
        case kMediaSourceType:
            return mData == rhs.mData;
        case kRectType:
            return mData->getRect() == rhs.mData->getRect();
        case kRadiiType:
            return mData->getRadii() == rhs.mData->getRadii();
        case kStyledTextType:
            return mData->getStyledText() == rhs.mData->getStyledText();
        case kGraphicType:
            return mData == rhs.mData;
        case kTransformType:
            return mData == rhs.mData;
        case kTransform2DType:
            return mData->getTransform2D() == rhs.mData->getTransform2D();
        case kEasingType:
            return mData->getEasing() == rhs.mData->getEasing();
        case kAnimationType:
            return mData == rhs.mData;
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
            return mData->getJson() != nullptr;
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

std::string
Object::asString() const
{
    switch (mType) {
        case kNullType: return "";
        case kBoolType: return mValue ? "true": "false";
        case kStringType: return mString;
        case kNumberType: return doubleToString(mValue);
        case kAutoDimensionType: return "auto";
        case kAbsoluteDimensionType: return doubleToString(mValue)+"dp";
        case kRelativeDimensionType: return doubleToString(mValue)+"%";
        case kColorType: return Color(mValue).asString();
        case kMapType: return "";
        case kArrayType: return "";
        case kNodeType: return "node<>";
        case kFunctionType: return "";
        case kFilterType: return "";
        case kGradientType: return "";
        case kMediaSourceType: return "";
        case kRectType: return "";
        case kRadiiType: return "";
        case kStyledTextType: return mData->getStyledText().asString();
        case kGraphicType: return "";
        case kTransformType: return "";
        case kTransform2DType: return "";
        case kEasingType: return "";
        case kAnimationType: return "";
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
            return mValue;
        case kStringType:
            return stringToDouble(mString);
        case kStyledTextType:
            return stringToDouble(mData->getStyledText().asString());
        default:
            return std::numeric_limits<double>::quiet_NaN();
    }
}

int
Object::asInt() const
{
    switch (mType) {
        case kBoolType:
            return static_cast<int>(mValue);
        case kNumberType:
            return std::rint(mValue);
        case kStringType:
            try { return std::stoi(mString); } catch(...) {}
            return std::numeric_limits<int>::quiet_NaN();
        case kStyledTextType:
            try { return std::stoi(mData->getStyledText().asString()); } catch(...) {}
            return std::numeric_limits<int>::quiet_NaN();
        default:
            return std::numeric_limits<int>::quiet_NaN();
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
            return Color(mValue);
        case kStringType:
            return Color(session, mString);
        case kStyledTextType:
            return Color(session, mData->getStyledText().asString());
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
            return Dimension(DimensionType::Absolute, mValue);
        case kStringType:
            return Dimension(context, mString);
        case kAbsoluteDimensionType:
            return Dimension(DimensionType::Absolute, mValue);
        case kRelativeDimensionType:
            return Dimension(DimensionType::Relative, mValue);
        case kAutoDimensionType:
            return Dimension(DimensionType::Auto, 0);
        case kStyledTextType:
            return Dimension(context, mData->getStyledText().asString());
        default:
            return Dimension(DimensionType::Absolute, 0);
    }
}

Dimension
Object::asAbsoluteDimension(const Context& context) const
{
    switch (mType) {
        case kNumberType:
            return Dimension(DimensionType::Absolute, mValue);
        case kStringType: {
            auto d = Dimension(context, mString);
            return (d.getType() == DimensionType::Absolute ? d : Dimension(DimensionType::Absolute, 0));
        }
        case kStyledTextType: {
            auto d = Dimension(context, mData->getStyledText().asString());
            return (d.getType() == DimensionType::Absolute ? d : Dimension(DimensionType::Absolute, 0));
        }
        case kAbsoluteDimensionType:
            return Dimension(DimensionType::Absolute, mValue);
        default:
            return Dimension(DimensionType::Absolute, 0);
    }
}

Dimension
Object::asNonAutoDimension(const Context& context) const
{
    switch (mType) {
        case kNumberType:
            return Dimension(DimensionType::Absolute, mValue);
        case kStringType: {
            auto d = Dimension(context, mString);
            return (d.getType() == DimensionType::Auto ? Dimension(DimensionType::Absolute, 0) : d);
        }
        case kStyledTextType: {
            auto d = Dimension(context, mData->getStyledText().asString());
            return (d.getType() == DimensionType::Auto ? Dimension(DimensionType::Absolute, 0) : d);
        }
        case kAbsoluteDimensionType:
            return Dimension(DimensionType::Absolute, mValue);
        case kRelativeDimensionType:
            return Dimension(DimensionType::Relative, mValue);
        default:
            return Dimension(DimensionType::Absolute, 0);
    }
}

Dimension
Object::asNonAutoRelativeDimension(const Context& context) const
{
    switch (mType) {
        case kNumberType:
            return Dimension(DimensionType::Relative, mValue * 100);
        case kStringType: {
            auto d = Dimension(context, mString, true);
            return (d.getType() == DimensionType::Auto ? Dimension(DimensionType::Relative, 0) : d);
        }
        case kStyledTextType: {
            auto d = Dimension(context, mData->getStyledText().asString(), true);
            return (d.getType() == DimensionType::Auto ? Dimension(DimensionType::Relative, 0) : d);
        }
        case kAbsoluteDimensionType:
            return Dimension(DimensionType::Absolute, mValue);
        case kRelativeDimensionType:
            return Dimension(DimensionType::Relative, mValue);
        default:
            return Dimension(DimensionType::Relative, 0);
    }
}


bool
Object::truthy() const
{
    switch (mType) {
        case kNullType:
            return false;
        case kBoolType:
        case kNumberType:
            return mValue != 0;
        case kStringType:
            return mString.size() != 0;
        case kArrayType:
        case kMapType:
        case kNodeType:
        case kFunctionType:
            return true;
        case kAbsoluteDimensionType:
        case kRelativeDimensionType:
            return mValue != 0;
        case kAutoDimensionType:
        case kColorType:
        case kFilterType:
        case kGradientType:
        case kMediaSourceType:
            return true;
        case kRectType:
            return !mData->getRect().isEmpty();
        case kRadiiType:
            return !mData->getRadii().isEmpty();
        case kStyledTextType:
            return mData->getStyledText().getText().size() != 0 || !mData->getStyledText().getSpans().empty();
        case kGraphicType:
            return true;
        case kTransformType:
            return true;
        case kTransform2DType:
            return true;
        case kEasingType:
            return true;
        case kAnimationType:
            return true;
    }

    // Should never be reached.
    return true;
}

// Methods for MAP objects
const Object
Object::get(const std::string& key) const
{
    assert(mType == kMapType);
    return mData->get(key);
}

bool
Object::has(const std::string& key) const
{
    assert(mType == kMapType);
    return mData->has(key);
}

// Methods for ARRAY objects
const Object
Object::at(size_t index) const
{
    assert(mType == kArrayType);
    return mData->at(index);
}

const Object::ObjectType
Object::getType() const
{
    return mType;
}


size_t
Object::size() const
{
    switch (mType) {
        case kArrayType:
        case kMapType:
            return mData->size();
        case kStringType:
            return mString.size();
        case kStyledTextType:
            return mData->getStyledText().asString().size();  // Size of the raw text
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
            return mData->empty();
        case kStringType:
            return mString.empty();
        case kStyledTextType:
            return mData->getStyledText().asString().empty();  // Only true if the raw text is empty
        default:
            return false;
    }
}

Object
Object::eval(const Context& context) const
{
    return mType == kNodeType ? mData->eval(context) : *this;
}

void
Object::symbols(std::set<std::string>& symbols) const {
    if (mType == kArrayType || mType == kNodeType)
        mData->symbols(symbols);
}

Object
Object::call(const std::vector<Object>& args) const {
    assert(mType == kFunctionType);
    if (OBJECT_DEBUG) LOG(LogLevel::DEBUG) << "Calling user function";
    return mData->call(args);
}

// Visitor pattern
void
Object::accept(Visitor<Object>& visitor) const
{
    visitor.visit(*this);
    if (mType == kArrayType || mType == kMapType || mType == kNodeType)
        mData->accept(visitor);
}

rapidjson::Value
Object::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    switch (mType) {
        case kNullType:
            return rapidjson::Value();
        case kBoolType:
            return rapidjson::Value(static_cast<bool>(mValue));
        case kNumberType:
            return rapidjson::Value(mValue);
        case kStringType:
            return rapidjson::Value(mString.c_str(), allocator);
        case kArrayType: {
            rapidjson::Value v(rapidjson::kArrayType);
            for (int i = 0 ; i < size() ; i++)
                v.PushBack(at(i).serialize(allocator), allocator);
            return v;
        }
        case kMapType: {
            rapidjson::Value m(rapidjson::kObjectType);
            for (auto &kv : mData->getMap())
                m.AddMember(rapidjson::Value(kv.first.c_str(), allocator), kv.second.serialize(allocator).Move(), allocator);
            return m;
        }
        case kNodeType:
            return rapidjson::Value("UNABLE TO SERIALIZE NODE", allocator);
        case kFunctionType:
            return rapidjson::Value("UNABLE TO SERIALIZE FUNCTION", allocator);
        case kAbsoluteDimensionType:
            return rapidjson::Value(mValue);
        case kRelativeDimensionType:
            return rapidjson::Value((doubleToString(mValue)+"%").c_str(), allocator);
        case kAutoDimensionType:
            return rapidjson::Value("auto", allocator);
        case kColorType:
            return rapidjson::Value(asString().c_str(), allocator);
        case kFilterType:
            return getFilter().serialize(allocator);
        case kGradientType:
            return getGradient().serialize(allocator);
        case kMediaSourceType:
            return getMediaSource().serialize(allocator);
        case kRectType:
            return getRect().serialize(allocator);
        case kRadiiType:
            return getRadii().serialize(allocator);
        case kStyledTextType:
            return getStyledText().serialize(allocator);
        case kGraphicType:
            return getGraphic()->serialize(allocator);
        case kTransformType:
            return rapidjson::Value("UNABLE TO SERIALIZE TRANSFORM", allocator);
        case kTransform2DType:
            return getTransform2D().serialize(allocator);
        case kEasingType:
            return rapidjson::Value("UNABLE TO SERIALIZE EASING", allocator);
        case kAnimationType:
            return rapidjson::Value("UNABLE TO SERIALIZE ANIMATION", allocator);
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

std::string
Object::toDebugString() const
{
    switch (mType) {
        case Object::kNullType:
            return "null";
        case Object::kBoolType:
            return (mValue ? "true" : "false");
        case Object::kNumberType:
            return std::to_string(mValue);
        case Object::kStringType:
            return mString;
        case Object::kMapType:
        case Object::kArrayType:
        case Object::kNodeType:
        case Object::kFunctionType:
            return mData->toDebugString();
        case Object::kAbsoluteDimensionType:
            return "AbsDim<" + std::to_string(mValue) + ">";
        case Object::kRelativeDimensionType:
            return "RelDim<" + std::to_string(mValue) + ">";
        case Object::kAutoDimensionType:
            return "AutoDim";
        case Object::kColorType:
            return asString();
        case Object::kFilterType:
        case Object::kGradientType:
        case Object::kMediaSourceType:
        case Object::kRectType:
        case Object::kRadiiType:
        case Object::kStyledTextType:
        case Object::kGraphicType:
        case Object::kTransformType:
        case Object::kTransform2DType:
        case Object::kEasingType:
        case Object::kAnimationType:
            return mData->toDebugString();
    }
}

streamer&
operator<<(streamer& os, const Object& object)
{
    os << object.toDebugString();
    return os;
}

}  // namespace apl
