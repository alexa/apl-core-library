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

#include "apl/primitives/object.h"

#include <stack>

#include "apl/engine/context.h"
#include "apl/primitives/boundsymbol.h"
#include "apl/primitives/color.h"
#include "apl/primitives/dimension.h"
#include "apl/primitives/functions.h"
#include "apl/primitives/objecttype.h"
#include "apl/primitives/rangegenerator.h"
#include "apl/primitives/slicegenerator.h"
#include "apl/utils/log.h"

namespace apl {

const bool OBJECT_DEBUG = false;

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

/****************************************************************************/

Object::Object(const Object& object) noexcept
        : mType(object.mType)
{
    switch(mType->storageType()) {
        case StorageType::kStorageTypeEmpty:
            break;
        case StorageType::kStorageTypeValue:
            mU.value = object.mU.value;
            break;
        case StorageType::kStorageTypeString:
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
    switch(mType->storageType()) {
        case StorageType::kStorageTypeEmpty:
            break;
        case StorageType::kStorageTypeValue:
            mU.value = object.mU.value;
            break;
        case StorageType::kStorageTypeString:
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
        switch(mType->storageType()) {
            case StorageType::kStorageTypeEmpty:
                break;
            case StorageType::kStorageTypeValue:
                mU.value = rhs.mU.value;
                break;
            case StorageType::kStorageTypeString:
                mU.string = rhs.mU.string;
                break;
            default:
                mU.data = rhs.mU.data;
                break;
        }
    }
    else {
        // Delete the old item
        switch(mType->storageType()) {
            case StorageType::kStorageTypeEmpty: // FALL_THROUGH
            case StorageType::kStorageTypeValue:
                break;
            case StorageType::kStorageTypeString:
                mU.string.~basic_string<char>();
                break;
            default:
                mU.data.~shared_ptr<ObjectData>();
                break;
        }

        // Construct the new
        mType = rhs.mType;
        switch(mType->storageType()) {
            case StorageType::kStorageTypeEmpty:
                break;
            case StorageType::kStorageTypeValue:
                mU.value = rhs.mU.value;
                break;
            case StorageType::kStorageTypeString:
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
        switch(mType->storageType()) {
            case StorageType::kStorageTypeEmpty:
                break;
            case StorageType::kStorageTypeValue:
                mU.value = rhs.mU.value;
                break;
            case StorageType::kStorageTypeString:
                mU.string = std::move(rhs.mU.string);
                break;
            default:
                mU.data = std::move(rhs.mU.data);
                break;
        }
    }
    else {
        // Delete the old item
        switch(mType->storageType()) {
            case StorageType::kStorageTypeEmpty: // FALL_THROUGH
            case StorageType::kStorageTypeValue:
                break;
            case StorageType::kStorageTypeString:
                mU.string.~basic_string<char>();
                break;
            default:
                mU.data.~shared_ptr<ObjectData>();
                break;
        }

        // Construct the new
        mType = rhs.mType;
        switch(mType->storageType()) {
            case StorageType::kStorageTypeEmpty:
                break;
            case StorageType::kStorageTypeValue:
                mU.value = rhs.mU.value;
                break;
            case StorageType::kStorageTypeString:
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
    switch(mType->storageType()) {
        case StorageType::kStorageTypeEmpty: // FALL_THROUGH
        case StorageType::kStorageTypeValue:
            break;
        case StorageType::kStorageTypeString:
            mU.string.~basic_string<char>();
            break;
        default:
            mU.data.~shared_ptr<ObjectData>();
            break;
    }
}

Object::Object()
    : mType(Null::ObjectType::instance())
{
    LOG_IF(OBJECT_DEBUG) << "Object null constructor" << this;
}

Object::Object(bool b)
    : mType(Boolean::ObjectType::instance()),
      mU(b ? 1.0 : 0.0)
{
    LOG_IF(OBJECT_DEBUG) << "Object bool constructor: " << b << " this=" << this;
}

Object::Object(int i)
    : mType(Number::ObjectType::instance()),
      mU(i)
{}

Object::Object(uint32_t u)
    : mType(Number::ObjectType::instance()),
      mU(u)
{}

Object::Object(unsigned long l)
    : mType(Number::ObjectType::instance()),
      mU(static_cast<double>(l))
{}

Object::Object(long l)
    : mType(Number::ObjectType::instance()),
      mU(static_cast<double>(l))
{}

Object::Object(unsigned long long l)
    : mType(Number::ObjectType::instance()),
      mU(static_cast<double>(l))
{}

Object::Object(long long l)
    : mType(Number::ObjectType::instance()),
      mU(static_cast<double>(l))
{}

Object::Object(double d)
    : mType(Number::ObjectType::instance()),
      mU(d)
{}

Object::Object(const char *s)
    : mType(String::ObjectType::instance()),
      mU(s)
{}

Object::Object(const std::string& s)
    : mType(String::ObjectType::instance()),
      mU(s)
{}

Object::Object(const ObjectMapPtr& m, bool isMutable)
    : mType(Map::ObjectType::instance()),
      mU(std::static_pointer_cast<ObjectData>(std::make_shared<MapData>(m, isMutable)))
{}

Object::Object(const ObjectArrayPtr& v, bool isMutable)
    : mType(Array::ObjectType::instance()),
      mU(std::static_pointer_cast<ObjectData>(std::make_shared<ArrayData>(v, isMutable)))
{}

Object::Object(ObjectArray&& v, bool isMutable)
    : mType(Array::ObjectType::instance()),
      mU(std::static_pointer_cast<ObjectData>(std::make_shared<FixedArrayData>(std::move(v), isMutable)))
{}

Object::Object(const rapidjson::Value& value) : mType(Null::ObjectType::instance())
{
    LOG_IF(OBJECT_DEBUG) << "Object constructor value: " << this;

    switch(value.GetType()) {
    case rapidjson::kNullType:
        mType = Null::ObjectType::instance();
        break;
    case rapidjson::kFalseType:
        mType = Boolean::ObjectType::instance();
        mU.value = 0;
        break;
    case rapidjson::kTrueType:
        mType = Boolean::ObjectType::instance();
        mU.value = 1;
        break;
    case rapidjson::kNumberType:
        mType = Number::ObjectType::instance();
        mU.value = value.GetDouble();
        break;
    case rapidjson::kStringType:
        mType = String::ObjectType::instance();
        new(&mU.string) std::string(value.GetString());  // TODO: Should we keep the string in place?
        break;
    case rapidjson::kObjectType:
        mType = Map::ObjectType::instance();
        new(&mU.data) std::shared_ptr<ObjectData>(std::make_shared<JSONData>(&value));
        break;
    case rapidjson::kArrayType:
        mType = Array::ObjectType::instance();
        new(&mU.data) std::shared_ptr<ObjectData>(std::make_shared<JSONData>(&value));
        break;
    }
}

Object::Object(rapidjson::Document&& value) : mType(Null::ObjectType::instance())
{
    if (OBJECT_DEBUG) LOG(LogLevel::kDebug) << "Object constructor value: " << this;

    switch(value.GetType()) {
        case rapidjson::kNullType:
            mType = Null::ObjectType::instance();
            break;
        case rapidjson::kFalseType:
            mType = Boolean::ObjectType::instance();
            mU.value = 0;
            break;
        case rapidjson::kTrueType:
            mType = Boolean::ObjectType::instance();
            mU.value = 1;
            break;
        case rapidjson::kNumberType:
            mType = Number::ObjectType::instance();
            mU.value = value.GetDouble();
            break;
        case rapidjson::kStringType:
            mType = String::ObjectType::instance();
            new(&mU.string) std::string(value.GetString());
            break;
        case rapidjson::kObjectType:
            mType = Map::ObjectType::instance();
            new(&mU.data) std::shared_ptr<ObjectData>(std::make_shared<JSONDocumentData>(std::move(value)));
            break;
        case rapidjson::kArrayType:
            mType = Array::ObjectType::instance();
            new(&mU.data) std::shared_ptr<ObjectData>(std::make_shared<JSONDocumentData>(std::move(value)));
            break;
    }
}

Object::Object(const Dimension& d)
    : mType(d.isAuto() ? Dimension::AutoDimensionObjectType::instance() :
           (d.isRelative() ? Dimension::RelativeDimensionObjectType::instance()
                           : Dimension::AbsoluteDimensionObjectType::instance())),
      mU(d.getValue())
{
    if (OBJECT_DEBUG)
        LOG(LogLevel::kDebug) << "Object dimension constructor: " << this << " is " << *this << " dimension=" << d;
}

Object::Object(const Color& color)
    : mType(Color::ObjectType::instance()),
      mU(color.get())
{
    LOG_IF(OBJECT_DEBUG) << "Object color constructor " << this;
}

Object::Object(const std::shared_ptr<RangeGenerator>& range)
    : mType(Array::ObjectType::instance()),
      mU(std::static_pointer_cast<ObjectData>(range))
{
    LOG_IF(OBJECT_DEBUG) << "Object range generator " << this;
}

Object::Object(const std::shared_ptr<SliceGenerator>& range)
    : mType(Array::ObjectType::instance()),
      mU(std::static_pointer_cast<ObjectData>(range))
{
    LOG_IF(OBJECT_DEBUG) << "Object slice generator " << this;
}

bool
Object::comparableWith(const Object& rhs) const {
    // Same type is comparable
    if (mType == rhs.mType) return true;

    // Any true maps considered comparable
    if (mType->isTrueMap() && rhs.mType->isTrueMap()) return true;

    // Any arrays considered comparable.
    if (mType->isArray() && rhs.mType->isArray()) return true;

    return false;
}

bool
Object::operator==(const Object& rhs) const
{
    LOG_IF(OBJECT_DEBUG) << "comparing " << *this << " to " << rhs;

    if (!comparableWith(rhs)) return false;

    return mType->equals(mU, rhs.mU);
}

bool
Object::operator!=(const Object& rhs) const
{
    return !operator==(rhs);
}

bool Object::isNull() const { return mType->is<Null>(); }
bool Object::isBoolean() const { return mType->is<Boolean>();}
bool Object::isString() const { return mType->is<String>(); }
bool Object::isNumber() const { return mType->is<Number>(); }
bool Object::isNaN() const { return isNumber() && std::isnan(getDouble()); }
bool Object::isArray() const { return mType->isArray(); }
bool Object::isMap() const { return mType->isMap(); }
bool Object::isTrueMap() const { return mType->isTrueMap(); }
bool Object::isCallable() const { return mType->isCallable(); }
bool Object::isEvaluable() const { return mType->isEvaluable(); }
bool Object::isAbsoluteDimension() const { return mType->isAbsoluteDimension(); }
bool Object::isRelativeDimension() const { return mType->isRelativeDimension(); }
bool Object::isAutoDimension() const { return mType->isAutoDimension(); }
bool Object::isNonAutoDimension() const { return mType->isNonAutoDimension(); }
bool Object::isDimension() const { return mType->isDimension(); }

std::string Object::asString() const { return mType->asString(mU); }
double Object::asNumber() const { return mType->asNumber(mU); }
float Object::asFloat() const { return static_cast<float>(asNumber()); }
int Object::asInt(int base) const { return mType->asInt(mU, base); }
std::int64_t Object::asInt64(int base) const { return mType->asInt64(mU, base); }
Color Object::asColor() const { return asColor(nullptr); }
Color Object::asColor(const SessionPtr& session) const { return mType->asColor(mU, session); }
Color Object::asColor(const Context& context) const { return asColor(context.session()); }
Dimension Object::asDimension(const Context& context) const { return mType->asDimension(mU, context); }
Dimension Object::asAbsoluteDimension(const Context& context) const { return mType->asAbsoluteDimension(mU, context); }
Dimension Object::asNonAutoDimension(const Context& context) const { return mType->asNonAutoDimension(mU, context); }
Dimension Object::asNonAutoRelativeDimension(const Context& context) const { return mType->asNonAutoRelativeDimension(mU, context); }

const std::string& Object::getString() const { return mType->getString(mU); }
bool Object::getBoolean() const { return mType->getBoolean(mU); }
double Object::getDouble() const { return mType->getDouble(mU); }
int Object::getInteger() const { return static_cast<int>(mType->getDouble(mU)); }
double Object::getAbsoluteDimension() const { return mType->getAbsoluteDimension(mU); }
double Object::getRelativeDimension() const { return mType->getRelativeDimension(mU); }
uint32_t Object::getColor() const { return mType->getColor(mU); }
const ObjectMap& Object::getMap() const { return mType->getMap(mU); }
ObjectMap& Object::getMutableMap() { return mType->getMutableMap(mU); }
const ObjectArray& Object::getArray() const { return mType->getArray(mU); }
ObjectArray& Object::getMutableArray() { return mType->getMutableArray(mU); }
std::shared_ptr<LiveDataObject> Object::getLiveDataObject() const { return mType->getLiveDataObject(mU); }

bool Object::truthy() const { return mType->truthy(mU); }

// Methods for MAP-like objects
Object Object::get(const std::string& key) const { return mType->get(mU, key); }
bool Object::has(const std::string& key) const { return mType->has(mU, key); }
Object Object::opt(const std::string& key, const Object& def) const { return mType->opt(mU, key, def); }
std::pair<std::string, Object> Object::keyAt(std::size_t offset) const { return mType->keyAt(mU, offset); }

// Methods for ARRAY objects
Object Object::at(std::uint64_t index) const { return mType->at(mU, index); }

const apl::ObjectType& Object::type() const { return *mType; }

std::uint64_t Object::size() const { return mType->size(mU); }

bool Object::empty() const { return mType->empty(mU); }

bool Object::isMutable() const { return mType->isMutable(mU); }

Object Object::eval() const { return mType->isEvaluable() ? mType->eval(mU) : *this; }

/**
 * Internal visitor class used to check if an equation is "pure" - that is, if the result
 * of the equation does not change each time you calculate it.
 */
class PureVisitor : public Visitor<Object> {
public:
    void visit(const Object& object) override {
        if (object.is<Function>() && !object.get<Function>()->isPure())
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

Object Object::call(const ObjectArray& args) const { return mType->call(mU, args); }

size_t Object::hash() const { return mType->hash(mU); }

// Visitor pattern
void
Object::accept(Visitor<Object>& visitor) const
{
    visitor.visit(*this);
    if (!visitor.isAborted())
        mType->accept(mU, visitor);
}

rapidjson::Value Object::serialize(rapidjson::Document::AllocatorType& allocator) const { return mType->serialize(mU, allocator); }

rapidjson::Value
Object::serializeDirty(rapidjson::Document::AllocatorType& allocator) const
{
    // TODO: Fix this - should return just the dirty bits
    return serialize(allocator);
}

std::string Object::toDebugString() const { return mType->toDebugString(mU); }

const void*
Object::extractDataInner(const Object::DataHolder& dataHolder)
{
    return dataHolder.data->inner();
}

streamer&
operator<<(streamer& os, const Object& object)
{
    os << object.toDebugString();
    return os;
}

}  // namespace apl
