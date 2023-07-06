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
 *
 * Object system
 *
 * Option #1 is a traditional object system with String, Boolean as subclasses
 * Option #2 is a variant object system with fixed size objects.
 *
 * These types should be supported in the resources system
 *
 *    Null      (singleton)
 *    Boolean
 *    Number (integer or double)
 *    Dimension
 *    String
 *    Gradient
 *    MediaSource
 *    JSONObject
 *    JSONArray
 *    IOptArray
 *    IOptMap
 *    Function
 */

#ifndef _APL_OBJECT_H
#define _APL_OBJECT_H

#include <map>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "rapidjson/document.h"

#include "apl/common.h"
#include "apl/utils/deprecated.h"
#include "apl/utils/visitor.h"

#ifdef APL_CORE_UWP
    #undef TRUE
    #undef FALSE
#endif

namespace apl {

namespace datagrammar {
    class ByteCode;
}

class Object;
class Color;
class Dimension;
class LiveDataObject;
class RangeGenerator;
class SliceGenerator;
class ObjectData;
class ObjectType;

class streamer;

using ObjectMap = std::map<std::string, Object>;
using ObjectMapPtr = std::shared_ptr<ObjectMap>;
using ObjectArray = std::vector<Object>;
using ObjectArrayPtr = std::shared_ptr<ObjectArray>;

/**
 * A single Object which can hold a variety of types.
 * Most objects are of type null, boolean, number, or string.  They all fit within
 * this basic object.  Other possibilities include byte code (for expression evaluate),
 * maps (for context and for JSONObject) and arrays (vectors or JSONArray).
 *
 * To avoid dynamic casting, the base object has methods for manipulating all of these
 * types.  The types that require additional storage put a shared_ptr in a single data
 * property.
 *
 * Note that certain types stored in Objects are treated as immutable and certain types
 * are mutable.  Examples of immutable types are:
 *
 * - Null
 * - Boolean
 * - Number
 * - String
 * - Array
 * - Map (object)
 * - Dimensions (absolute, relative, and auto)
 * - Colors
 *
 * Example of mutable types are:
 * - Vector graphic
 * - Generalized transformation
 */
class Object
{
public:
    /// ObjectData held types.
    enum class StorageType {
        kStorageTypeEmpty,
        kStorageTypeValue,
        kStorageTypeString,
        kStorageTypeReference,
        kStorageTypePointer
    };

    // Constructors
    Object();
    Object(bool b);
    Object(int i);
    Object(uint32_t u);
    Object(unsigned long l);
    Object(long l);
    Object(unsigned long long l);
    Object(long long l);
    Object(double d);
    Object(const char *s);
    Object(const std::string& s);
    Object(const ObjectMapPtr& m, bool isMutable=false);
    Object(const ObjectArrayPtr& v, bool isMutable=false);
    Object(ObjectArray&& v, bool isMutable=false);
    Object(const Color& color);
    Object(const Dimension& dimension);
    Object(const rapidjson::Value& v);
    Object(rapidjson::Document&& doc);

    template<
        class T,
        StorageType ST = T::ObjectType::scStorageType,
        typename = typename std::enable_if<ST == StorageType::kStorageTypePointer>::type
        >
    Object(const std::shared_ptr<T>& content) : mType(T::ObjectType::instance()), mU(content) {}

    template<
        class T,
        StorageType ST = T::ObjectType::scStorageType,
        typename = typename std::enable_if<ST == StorageType::kStorageTypeReference>::type
        >
    Object(T&& content) : mType(T::ObjectType::instance()), mU(T::ObjectType::createDirectObjectData(std::move(content))) {}

    Object(const std::shared_ptr<RangeGenerator>& range);
    Object(const std::shared_ptr<SliceGenerator>& slice);

    // Statically initialized objects.
    static const Object& TRUE_OBJECT();
    static const Object& FALSE_OBJECT();
    static const Object& NULL_OBJECT();
    static Object NAN_OBJECT();
    static Object EMPTY_ARRAY();
    static Object EMPTY_MUTABLE_ARRAY();
    static Object EMPTY_MAP();
    static Object EMPTY_MUTABLE_MAP();

    // Destructor
    ~Object();

    // Copy operations
    Object(const Object& object) noexcept;      // Copy-constructor
    Object(Object&& object) noexcept;           // Move-constructor
    Object& operator=(const Object& rhs) noexcept;     // Assignment
    Object& operator=(Object&& rhs) noexcept;

    // Comparisons
    bool operator==(const Object& rhs) const;
    bool operator!=(const Object& rhs) const;

    /**
     * Check if object contains data of provided type.
     * @tparam T type
     * @return true if contains requested type, false otherwise.
     */
    template <class T> bool is() const { return mType == T::ObjectType::instance(); }

    bool isNull() const;
    bool isBoolean() const;
    bool isString() const;
    bool isNumber() const;
    bool isNaN() const;
    bool isArray() const;
    bool isMap() const;
    bool isTrueMap() const;
    bool isDimension() const;
    bool isAbsoluteDimension() const;
    bool isRelativeDimension() const;
    bool isAutoDimension() const;
    bool isNonAutoDimension() const;

    bool isCallable() const;
    bool isEvaluable() const;

    /// These methods force a conversion to the appropriate type.  We try to return
    /// plausible defaults whenever possible.
    /**
     * This method is used when coercing an object to a string.  This can be used
     * by an APL author to display information in a Text component, so we deliberately
     * do not return values for many of the internal object types.  Please use
     * Object::toDebugString() to return strings suitable for writing to the system log.
     *
     * @return The user-friendly value of the object as a string.
     */
    std::string asString() const;
    bool asBoolean() const { return truthy(); }
    double asNumber() const;
    float asFloat() const;
    int asInt(int base=10) const;
    int64_t asInt64(int base = 10) const;
    Dimension asDimension(const Context& ) const;
    Dimension asAbsoluteDimension(const Context&) const;
    Dimension asNonAutoDimension(const Context&) const;
    Dimension asNonAutoRelativeDimension(const Context&) const;
    /// @deprecated This method will be removed soon.
    APL_DEPRECATED Color asColor() const;
    Color asColor(const SessionPtr&) const;
    Color asColor(const Context&) const;

    union DataHolder {
        double value;
        std::string string;
        std::shared_ptr<ObjectData> data;

        DataHolder() : value(0.0) {}
        DataHolder(double v) : value(v) {}
        DataHolder(const std::string& s) : string(s) {}
        DataHolder(const std::shared_ptr<ObjectData>& d) : data(d) {}
        ~DataHolder() {}
    };

    // These methods return the actual contents of the Object
    /**
     * Get data of provided type from the object. Applicable only to non-primitive types.
     * @tparam T requested type.
     * @return reference to data of requested type.
     */
    template<
        class T,
        StorageType ST = T::ObjectType::scStorageType,
        typename = typename std::enable_if<ST == StorageType::kStorageTypeReference>::type
        >
    const T& get() const {
        assert(is<T>());
        return *static_cast<const T*>(extractDataInner(mU));
    }

    /**
     * Get data of provided type from the object. Applicable only to non-primitive types.
     * @tparam T requested type.
     * @return pointer to data of requested type.
     */
    template<
        class T,
        StorageType ST = T::ObjectType::scStorageType,
        typename = typename std::enable_if<ST == StorageType::kStorageTypePointer>::type
        >
    std::shared_ptr<T> get() const {
        assert(is<T>());
        return std::static_pointer_cast<T>(mU.data);
    }

    const std::string& getString() const;
    bool getBoolean() const;
    double getDouble() const;
    int getInteger() const;
    double getAbsoluteDimension() const;
    double getRelativeDimension() const;
    uint32_t getColor() const;
    const ObjectMap& getMap() const;
    ObjectMap& getMutableMap();
    const ObjectArray& getArray() const;
    ObjectArray& getMutableArray();
    std::shared_ptr<LiveDataObject> getLiveDataObject() const;

    bool truthy() const;

    // MAP objects
    Object get(const std::string& key) const;
    bool has(const std::string& key) const;
    Object opt(const std::string& key, const Object& def) const;

    // ARRAY objects
    Object at(std::uint64_t index) const;

    // Get object type
    const apl::ObjectType& type() const;

    // MAP, ARRAY, and STRING objects
    std::uint64_t size() const;

    // NULL, MAP, ARRAY, RECT, and STRING objects
    bool empty() const;

    // Mutable objects
    bool isMutable() const;

    // BoundSymbol, and compiled ByteCodeInstruction objects
    Object eval() const;

    // BoundSymbol, and compiled ByteCodeInstruction objects
    bool isPure() const;

    // FUNCTION & Easing objects
    Object call(const ObjectArray& args) const;

    // Current object hash
    std::size_t hash() const;

    // Visitor pattern
    void accept(Visitor<Object>& visitor) const;

    // Convert this to a printable string. Not to be confused with "asString" or "getString"
    std::string toDebugString() const;

    friend streamer& operator<<(streamer&, const Object&);

    // Serialize to JSON format
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    // Serialize just the dirty bits to JSON format
    rapidjson::Value serializeDirty(rapidjson::Document::AllocatorType& allocator) const;

    template<typename T> T asEnum() const { return static_cast<T>(getInteger()); }

private:
    static const void* extractDataInner(const Object::DataHolder& dataHolder);
    bool comparableWith(const Object& rhs) const;

private:
    const apl::ObjectType* mType;
    union DataHolder mU;
};

}  // namespace apl

namespace std {
template<> struct hash<apl::Object> { std::size_t operator()(apl::Object const& s) const noexcept { return s.hash(); } };
} // namespace std

#endif // _APL_OBJECT_H
