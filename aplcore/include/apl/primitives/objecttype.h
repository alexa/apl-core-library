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

#ifndef _APL_OBJECT_MAGIC_TYPE_H
#define _APL_OBJECT_MAGIC_TYPE_H

#include "apl/primitives/objectdata.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/stringfunctions.h"
#include "apl/utils/throw.h"

namespace apl {

static const char *NOT_SUPPORTED_ERROR = "Operation not supported on this type.";

/**
 * Object type class. Should be extended by specific type/class implementations, which may be stored
 * in the Object.  A number of subclasses of ObjectType are provided to simplify the implementation
 * of new Object types.  The hierarchy of abstract ObjectTypes is:
 *
 * - ObjectType
 *   - BaseObjectType<T>                   Provides the instance() method
 *     - SimpleObjectType<T>               Disallows Map and Array methods
 *       - ReferenceHolderObjectType<T>    For objects of storage type kStorageTypeReference
 *   - PointerHolderObjectType<T>          For objects of storage type kStorageTypePointer
 *     - SimplePointerHolderObjectType<T>  Disallows map and array methods
 *     - ContainerObjectType<T>            Support visitor method on containers
 *       - AbstractMapObjectType<T>        Disallows array methods
 *         - MapLikeObjectType<T>          For "almost-map" objects which support map methods but you can't directly read the map
 *         - MapObjectType<T>              For true maps where you can read the map directly
 *       - ArrayObjectType<T>              Disallows maps.  Supports basic array operations.
 *
 * The concrete types are defined inline to a class and inherit from one of the abstract types.
 *
 * SimpleObjectType:              Boolean, Color, Null, Number, String, Dimension, AutoDimension
 * ReferenceHolderObjectType:     MediaSource, Radii, Range, Rect, StyledText, Transform2D, URLRequest,
 *                                BoundSymbol, Filter, Gradient, GraphicFilter
 * SimplePointerHolderObjectType: GraphicPattern, Transform, AccessibilityAction, ByteCode, Easing,
 *                                Function, Graphic
 * MapLikeObjectType:             ComponentEventWrapper, ContextWrapper
 * MapObjectType:                 LiveMapObject, Map
 * ArrayObjectType:               LiveArrayObject, Array
 */
class ObjectType : public NonCopyable {
public:
    virtual ~ObjectType() = default;

    /// Type checks.

    /**
     * Check Object-held type.
     * @tparam T type.
     * @return true if requested type/false otherwise.
     */
    template <class T> bool is() const { return (T::ObjectType::instance() == this); }

    /// Complex type checks.
    virtual bool isCallable() const { return false; }
    virtual bool isEvaluable() const { return false; }
    virtual bool isAbsoluteDimension() const { return false; }
    virtual bool isRelativeDimension() const { return false; }
    virtual bool isAutoDimension() const { return false; }
    virtual bool isNonAutoDimension() const { return false; }
    virtual bool isDimension() const { return false; }

    /// All methods below that Object::DataHolder which is union holding actual object content.
    /// Read as "data for Object of current type".

    // These methods force a conversion to the appropriate type.  We try to return
    // plausible defaults whenever possible.
    virtual std::string asString(const Object::DataHolder&) const { return ""; }
    virtual bool asBoolean(const Object::DataHolder& dataHolder) const { return truthy(dataHolder); }
    virtual double asNumber(const Object::DataHolder&) const {
        return std::numeric_limits<double>::quiet_NaN();
    }
    virtual int asInt(const Object::DataHolder&, int base) const { return 0; }
    virtual int64_t asInt64(const Object::DataHolder&, int base) const { return 0; }
    virtual std::pair<int, bool> asValidInt(const Object::DataHolder&, int base) const {
        return { 0, false };
    }
    virtual std::pair<int64_t, bool> asValidInt64(const Object::DataHolder&, int base) const {
        return { 0, false };
    }
    virtual Color asColor(const Object::DataHolder&, const SessionPtr&) const;
    virtual Dimension asDimension(const Object::DataHolder&, const Context&) const;
    virtual Dimension asAbsoluteDimension(const Object::DataHolder&, const Context&) const;
    virtual Dimension asNonAutoDimension(const Object::DataHolder&, const Context&) const;
    virtual Dimension asNonAutoRelativeDimension(const Object::DataHolder&, const Context&) const;

    // These methods return the actual contents of the Object
    /// Primitive types
    virtual const std::string& getString(const Object::DataHolder&) const { aplThrow(NOT_SUPPORTED_ERROR); }
    virtual bool getBoolean(const Object::DataHolder&) const { aplThrow(NOT_SUPPORTED_ERROR); }
    virtual double getDouble(const Object::DataHolder&) const { aplThrow(NOT_SUPPORTED_ERROR); }
    virtual double getAbsoluteDimension(const Object::DataHolder&) const { aplThrow(NOT_SUPPORTED_ERROR); }
    virtual double getRelativeDimension(const Object::DataHolder&) const { aplThrow(NOT_SUPPORTED_ERROR); }
    virtual uint32_t getColor(const Object::DataHolder&) const { aplThrow(NOT_SUPPORTED_ERROR); }

    /// ObjectData held types.
    template<class T>
    const T& getReferenced(const Object::DataHolder& dataHolder) const {
        assert(is<T>());
        assert(T::ObjectType::scStorageType == Object::StorageType::kStorageTypeReference);
        return *static_cast<const T*>(dataHolder.data->inner());
    }


    virtual std::shared_ptr<LiveDataObject> getLiveDataObject(const Object::DataHolder&) const {
        aplThrow(NOT_SUPPORTED_ERROR);
    }

    virtual bool truthy(const Object::DataHolder&) const { return false; }

    // MAP objects.  Subclasses must override these.  Use the MapFreeMixin class if not a map
    virtual bool isMap() const = 0;
    virtual bool isTrueMap() const = 0;
    virtual const ObjectMap& getMap(const Object::DataHolder&) const = 0;
    virtual ObjectMap& getMutableMap(const Object::DataHolder&) const = 0;
    virtual Object get(const Object::DataHolder&, const std::string&) const = 0;
    virtual bool has(const Object::DataHolder&, const std::string&) const = 0;
    virtual Object opt(const Object::DataHolder&, const std::string&, const Object&) const = 0;
    virtual std::pair<std::string, Object> keyAt(const Object::DataHolder&, std::size_t offset) const = 0;

    // ARRAY objects.  Subclasses must override these.  Use the ArrayFreeMixin class if not an array
    virtual bool isArray() const = 0;
    virtual const ObjectArray& getArray(const Object::DataHolder&) const = 0;
    virtual ObjectArray& getMutableArray(const Object::DataHolder&) const = 0;
    virtual Object at(const Object::DataHolder&, std::uint64_t) const = 0;

    // MAP, ARRAY, and STRING objects
    virtual std::uint64_t size(const Object::DataHolder&) const { return 0; }

    // NULL, MAP, ARRAY, RECT, and STRING objects
    virtual  bool empty(const Object::DataHolder&) const { return false; }

    // Mutable objects
    virtual bool isMutable(const Object::DataHolder&) const { return false; }

    // BoundSymbol, and compiled ByteCodeInstruction objects
    virtual Object eval(const Object::DataHolder&) const { aplThrow(NOT_SUPPORTED_ERROR); }

    // FUNCTION & Easing objects
    virtual Object call(const Object::DataHolder&, const ObjectArray&) const { aplThrow(NOT_SUPPORTED_ERROR); }

    // Current object hash
    virtual std::size_t hash(const Object::DataHolder&) const { return 0; }

    // Visitor pattern
    virtual void accept(const Object::DataHolder&, Visitor<Object>& visitor) const {}

    // Serialize to JSON format
    virtual rapidjson::Value serialize(
        const Object::DataHolder&,
        rapidjson::Document::AllocatorType&) const { return {}; }

    // Convert this to a printable string. Not to be confused with "asString" or "getString"
    virtual std::string toDebugString(const Object::DataHolder&) const { return ""; }

    /**
     * Check if data in 2 objects are equal.
     * @param lhs DataHolder of object of current type.
     * @param rhs Other Object's DataHolder.
     * @return true if equal, false otherwise.
     */
    virtual bool equals(const Object::DataHolder& lhs, const Object::DataHolder& rhs) const { return false; }

private:
    friend class Object;

    virtual Object::StorageType storageType() const = 0;
};

using ObjectTypeRef = const ObjectType*;

/// Base type "classes"

#define STORAGE_TYPE(T) \
        static const Object::StorageType scStorageType = Object::StorageType::T; \
    private: \
        Object::StorageType storageType() const override { return Object::StorageType::T; }

template<class T>
class BaseObjectType : public virtual ObjectType {
public:
    static ObjectTypeRef instance() {
        static typename T::ObjectType sObjectType;
        return &sObjectType;
    }

    STORAGE_TYPE(kStorageTypeValue);
};

/**
 * Mixin this class to mark an object type as NOT supporting map functions.
 */
class MapFreeMixin : public virtual ObjectType {
public:
    bool isMap() const final { return false; }
    bool isTrueMap() const final { return false; }
    const ObjectMap& getMap(const Object::DataHolder&) const final {
        aplThrow(NOT_SUPPORTED_ERROR);
    }
    ObjectMap& getMutableMap(const Object::DataHolder&) const final {
        aplThrow(NOT_SUPPORTED_ERROR);
    }
    Object get(const Object::DataHolder&, const std::string&) const final {
        aplThrow(NOT_SUPPORTED_ERROR);
    }
    bool has(const Object::DataHolder&, const std::string&) const final {
        aplThrow(NOT_SUPPORTED_ERROR);
    }
    Object opt(const Object::DataHolder&, const std::string&, const Object&) const final {
        aplThrow(NOT_SUPPORTED_ERROR);
    }
    std::pair<std::string, Object> keyAt(const Object::DataHolder&,
                                         std::size_t offset) const final {
        aplThrow(NOT_SUPPORTED_ERROR);
    }
};

/**
 * Mixin this class to mark an object type as NOT supporting array functions
 */
class ArrayFreeMixin : public virtual ObjectType {
public:
    bool isArray() const final { return false; }
    const ObjectArray& getArray(const Object::DataHolder&) const final {
        aplThrow(NOT_SUPPORTED_ERROR);
    }
    ObjectArray& getMutableArray(const Object::DataHolder&) const final {
        aplThrow(NOT_SUPPORTED_ERROR);
    }
    Object at(const Object::DataHolder&, std::uint64_t) const final {
        aplThrow(NOT_SUPPORTED_ERROR);
    }
};

/**
 * A simple extension of the base object that does not support maps or arrays
 */
template<class T>
class SimpleObjectType : public virtual BaseObjectType<T>,
                         public virtual MapFreeMixin,
                         public virtual ArrayFreeMixin {
};

template<class T>
class ReferenceHolderObjectType : public virtual SimpleObjectType<T> {
public:
    bool truthy(const Object::DataHolder& dataHolder) const final {
        return dataHolder.data->truthy();
    }

    rapidjson::Value serialize(
        const Object::DataHolder& dataHolder,
        rapidjson::Document::AllocatorType& allocator) const final {
        return dataHolder.data->serialize(allocator);
    }

    std::string toDebugString(const Object::DataHolder& dataHolder) const final {
        return dataHolder.data->toDebugString();
    }

    bool equals(const Object::DataHolder& lhs, const Object::DataHolder& rhs) const override {
        return *lhs.data == *rhs.data;
    }

    bool empty(const Object::DataHolder& dataHolder) const override {
        return dataHolder.data->empty();
    }

    static std::shared_ptr<ObjectData> createDirectObjectData(T&& content) {
        return DirectObjectData<T>::create(std::move(content));
    }

    STORAGE_TYPE(kStorageTypeReference);
};

template<class T>
class PointerHolderObjectType : public BaseObjectType<T> {
public:
    bool truthy(const Object::DataHolder&) const override { return true; }

    rapidjson::Value serialize(
        const Object::DataHolder& dataHolder,
        rapidjson::Document::AllocatorType& allocator) const override {
        return dataHolder.data->serialize(allocator);
    }

    std::string toDebugString(const Object::DataHolder& dataHolder) const override {
        return dataHolder.data->toDebugString();
    }

    bool equals(const Object::DataHolder& lhs, const Object::DataHolder& rhs) const override {
        return *lhs.data == *rhs.data;
    }

    bool empty(const Object::DataHolder& dataHolder) const override {
        return dataHolder.data->empty();
    }

    std::uint64_t size(const Object::DataHolder& dataHolder) const override {
        return dataHolder.data->size();
    }

    STORAGE_TYPE(kStorageTypePointer);
};

template<class T>
class SimplePointerHolderObjectType : public virtual PointerHolderObjectType<T>,
                                      public virtual MapFreeMixin,
                                      public virtual ArrayFreeMixin {
};

template<class T>
class ContainerObjectType : public PointerHolderObjectType<T> {
public:
    bool isMutable(const Object::DataHolder& dataHolder) const final {
        return dataHolder.data->isMutable();
    }

    void accept(const Object::DataHolder& dataHolder, Visitor<Object>& visitor) const final {
        dataHolder.data->accept(visitor);
    }
};

template<class T>
class AbstractMapObjectType : public virtual ContainerObjectType<T>,
                              public virtual ArrayFreeMixin {
public:
    bool isMap() const final { return true; }

    Object get(const Object::DataHolder& dataHolder, const std::string& key) const final {
        return dataHolder.data->get(key);
    }

    bool has(const Object::DataHolder& dataHolder, const std::string& key) const final {
        return dataHolder.data->has(key);
    }

    Object opt(const Object::DataHolder& dataHolder, const std::string& key, const Object& def) const final {
        return dataHolder.data->opt(key, def);
    }

    std::pair<std::string, Object> keyAt(const Object::DataHolder& dataHolder, std::size_t offset) const final {
        return dataHolder.data->keyAt(offset);
    }
};

template<class T>
class MapLikeObjectType : public AbstractMapObjectType<T> {
public:
    // Map-like objects do not allow you to return the map
    bool isTrueMap() const final { return false; }
    const ObjectMap& getMap(const Object::DataHolder&) const final { aplThrow(NOT_SUPPORTED_ERROR); }
    ObjectMap& getMutableMap(const Object::DataHolder&) const final { aplThrow(NOT_SUPPORTED_ERROR); }
};

template<class T>
class MapObjectType : public AbstractMapObjectType<T> {
public:
    bool isTrueMap() const final { return true; }

    const ObjectMap& getMap(const Object::DataHolder& dataHolder) const final {
        return dataHolder.data->getMap();
    }

    ObjectMap& getMutableMap(const Object::DataHolder& dataHolder) const final {
        return dataHolder.data->getMutableMap();
    }

    std::shared_ptr<LiveDataObject>
    getLiveDataObject(const Object::DataHolder& dataHolder) const override { return nullptr; }

    rapidjson::Value serialize(
        const Object::DataHolder& dataHolder,
        rapidjson::Document::AllocatorType& allocator) const final {
        rapidjson::Value m(rapidjson::kObjectType);
        for (auto& kv : dataHolder.data->getMap())
            m.AddMember(rapidjson::Value(kv.first.c_str(), allocator),
                        kv.second.serialize(allocator).Move(), allocator);
        return m;
    }
};

template<class T>
class ArrayObjectType : public virtual ContainerObjectType<T>,
                        public virtual MapFreeMixin {
public:
    bool isArray() const final { return true; }

    const ObjectArray& getArray(const Object::DataHolder& dataHolder) const final {
        return dataHolder.data->getArray();
    }

    ObjectArray& getMutableArray(const Object::DataHolder& dataHolder) const final {
        return dataHolder.data->getMutableArray();
    }

    std::shared_ptr<LiveDataObject> getLiveDataObject(
        const Object::DataHolder& dataHolder) const override
    {
        return nullptr;
    }

    Object at(const Object::DataHolder& dataHolder, std::uint64_t index) const final {
        return dataHolder.data->at(index);
    }

    rapidjson::Value serialize(
        const Object::DataHolder& dataHolder,
        rapidjson::Document::AllocatorType& allocator) const final
    {
        rapidjson::Value v(rapidjson::kArrayType);
        for (std::uint64_t i = 0 ; i < ContainerObjectType<T>::size(dataHolder) ; i++)
            v.PushBack(at(dataHolder, i).serialize(allocator), allocator);
        return v;
    }
};

/// Primitive types

class Null {
public:
    class ObjectType final : public SimpleObjectType<Null> {
    public:
        bool empty(const Object::DataHolder&) const override { return true; }

        std::string toDebugString(const Object::DataHolder&) const override { return "null"; }

        bool equals(const Object::DataHolder&, const Object::DataHolder&) const override { return true; }

        STORAGE_TYPE(kStorageTypeEmpty);
    };
};

class Boolean {
public:
    class ObjectType final : public SimpleObjectType<Boolean> {
    public:
        std::string asString(const Object::DataHolder& dataHolder) const override {
            return static_cast<bool>(dataHolder.value) ? "true": "false";
        }

        double asNumber(const Object::DataHolder& dataHolder) const override {
            return dataHolder.value;
        }

        int asInt(const Object::DataHolder& dataHolder, int base) const override {
            return static_cast<int>(dataHolder.value);
        }

        int64_t asInt64(const Object::DataHolder& dataHolder, int base) const override {
            return static_cast<std::int64_t>(dataHolder.value);
        }

        std::pair<int, bool> asValidInt(const Object::DataHolder& dataHolder, int base) const override {
            return { static_cast<int>(dataHolder.value), true };
        }

        std::pair<int64_t, bool> asValidInt64(const Object::DataHolder& dataHolder, int base) const override {
            return { static_cast<std::int64_t>(dataHolder.value), true };
        }

        bool getBoolean(const Object::DataHolder& dataHolder) const override {
            return dataHolder.value != 0;
        }

        bool truthy(const Object::DataHolder& dataHolder) const override {
            return dataHolder.value != 0;
        }

        std::size_t hash(const Object::DataHolder& dataHolder) const override {
            return std::hash<bool>{}(truthy(dataHolder));
        }

        rapidjson::Value serialize(
            const Object::DataHolder& dataHolder,
            rapidjson::Document::AllocatorType& allocator) const override
        {
            return rapidjson::Value(static_cast<bool>(dataHolder.value));
        }

        std::string toDebugString(const Object::DataHolder& dataHolder) const override {
            return (bool)(dataHolder.value) ? "true" : "false";
        }

        bool equals(const Object::DataHolder& lhs, const Object::DataHolder& rhs) const override {
            return lhs.value == rhs.value;
        }
    };
};

class Number {
public:
    class ObjectType final : public SimpleObjectType<Number> {
    public:
        std::string asString(const Object::DataHolder& dataHolder) const override {
            return doubleToAplFormattedString(dataHolder.value);
        }

        double asNumber(const Object::DataHolder& dataHolder) const override {
            return dataHolder.value;
        }

        int asInt(const Object::DataHolder& dataHolder, int base) const override {
            return static_cast<int>(std::lround(dataHolder.value));
        }

        int64_t asInt64(const Object::DataHolder& dataHolder, int base) const override {
            return std::llround(dataHolder.value);
        }

        std::pair<int, bool> asValidInt(const Object::DataHolder& dataHolder, int base) const override {
            return { static_cast<int>(std::lround(dataHolder.value)), true };
        }

        std::pair<int64_t, bool> asValidInt64(const Object::DataHolder& dataHolder, int base) const override {
            return { std::llround(dataHolder.value), true };
        }

        Color asColor(const Object::DataHolder& dataHolder, const SessionPtr&) const override;

        Dimension asDimension(const Object::DataHolder& dataHolder, const Context&) const override;

        Dimension asAbsoluteDimension(const Object::DataHolder& dataHolder, const Context&) const override;

        Dimension asNonAutoDimension(const Object::DataHolder& dataHolder, const Context&) const override;

        Dimension asNonAutoRelativeDimension(const Object::DataHolder& dataHolder, const Context&) const override;

        double getDouble(const Object::DataHolder& dataHolder) const override {
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
            if (!std::isfinite(dataHolder.value))
                return {};

            // Check to see if this value is an integer
            double intPart;
            double remainder = std::modf(dataHolder.value, &intPart);

            // 2^53: Largest integer such that it and all smaller integers can
            //       be stored in a double without losing precision.
            if (remainder == 0.0f && std::abs(intPart) <= 2e53)
                return rapidjson::Value(static_cast<int64_t>(intPart));

            // If all else fails, store it as a double value
            return rapidjson::Value(dataHolder.value);
        }

        std::string toDebugString(const Object::DataHolder& dataHolder) const override {
            return sutil::to_string(dataHolder.value);
        }

        bool equals(const Object::DataHolder& lhs, const Object::DataHolder& rhs) const override {
            return lhs.value == rhs.value;
        }
    };
};

class String {
public:
    class ObjectType final : public SimpleObjectType<String> {
    public:
        std::string asString(const Object::DataHolder& dataHolder) const override {
            return dataHolder.string;
        }

        double asNumber(const Object::DataHolder& dataHolder) const override {
            return aplFormattedStringToDouble(dataHolder.string);
        }

        int asInt(const Object::DataHolder& dataHolder, int base) const override {
            return sutil::stoi(dataHolder.string, nullptr, base);
        }

        int64_t asInt64(const Object::DataHolder& dataHolder, int base) const override {
            return sutil::stoll(dataHolder.string, nullptr, base);
        }

        std::pair<int, bool> asValidInt(const Object::DataHolder& dataHolder, int base) const override {
            std::size_t pos = 0;
            auto result = sutil::stoi(dataHolder.string, &pos, base);
            return { result, pos != 0 };
        }

        std::pair<int64_t, bool> asValidInt64(const Object::DataHolder& dataHolder, int base) const override {
            std::size_t pos = 0;
            auto result = sutil::stoll(dataHolder.string, &pos, base);
            return { result, pos != 0 };
        }

        Color asColor(const Object::DataHolder& dataHolder, const SessionPtr& session) const override;

        Dimension asDimension(const Object::DataHolder& dataHolder, const Context& context) const override;

        Dimension asAbsoluteDimension(const Object::DataHolder& dataHolder, const Context& context) const override;

        Dimension asNonAutoDimension(const Object::DataHolder& dataHolder, const Context& context) const override;

        Dimension asNonAutoRelativeDimension(const Object::DataHolder& dataHolder, const Context& context) const override;

        const std::string& getString(const Object::DataHolder& dataHolder) const override {
            return dataHolder.string;
        }

        bool truthy(const Object::DataHolder& dataHolder) const override {
            return !dataHolder.string.empty();
        }

        std::uint64_t size(const Object::DataHolder& dataHolder) const override {
            return dataHolder.string.size();
        }

        bool empty(const Object::DataHolder& dataHolder) const override {
            return dataHolder.string.empty();
        }

        std::size_t hash(const Object::DataHolder& dataHolder) const override {
            return std::hash<std::string>{}(dataHolder.string);
        }

        rapidjson::Value serialize(
            const Object::DataHolder& dataHolder,
            rapidjson::Document::AllocatorType& allocator) const override
        {
            return {dataHolder.string.c_str(), allocator};
        }

        std::string toDebugString(const Object::DataHolder& dataHolder) const override {
            return "'" + dataHolder.string + "'";
        }

        bool equals(const Object::DataHolder& lhs, const Object::DataHolder& rhs) const override {
            return lhs.string == rhs.string;
        }

        STORAGE_TYPE(kStorageTypeString);
    };
};

class Map {
public:
    class ObjectType : public MapObjectType<Map> {};
};

class Array {
public:
    class ObjectType : public ArrayObjectType<Array> {};
};

} // namespace apl

#endif // _APL_OBJECT_MAGIC_TYPE_H
