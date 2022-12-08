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
 * in the Object.
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
    virtual bool isArray() const { return false; }
    virtual bool isMap() const { return false; }
    virtual bool isTrueMap() const { return false; }
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
    const T& get(const Object::DataHolder& dataHolder) const {
        assert(is<T>());
        assert(T::ObjectType::scStorageType == Object::StorageType::kStorageTypeReference);
        return *static_cast<const T*>(dataHolder.data->inner());
    }

    virtual const ObjectMap& getMap(const Object::DataHolder&) const { aplThrow(NOT_SUPPORTED_ERROR); }
    virtual ObjectMap& getMutableMap(const Object::DataHolder&) const { aplThrow(NOT_SUPPORTED_ERROR); }
    virtual const ObjectArray& getArray(const Object::DataHolder&) const { aplThrow(NOT_SUPPORTED_ERROR); }
    virtual ObjectArray& getMutableArray(const Object::DataHolder&) const { aplThrow(NOT_SUPPORTED_ERROR); }

    virtual std::shared_ptr<LiveDataObject> getLiveDataObject(const Object::DataHolder&) const {
        aplThrow(NOT_SUPPORTED_ERROR);
    }

    virtual bool truthy(const Object::DataHolder&) const { return false; }

    // MAP objects
    virtual Object get(const Object::DataHolder&, const std::string&) const { aplThrow(NOT_SUPPORTED_ERROR); }
    virtual bool has(const Object::DataHolder&, const std::string&) const { aplThrow(NOT_SUPPORTED_ERROR); }
    virtual Object opt(const Object::DataHolder&, const std::string&, const Object&) const {
        aplThrow(NOT_SUPPORTED_ERROR);
    }

    // ARRAY objects
    virtual Object at(const Object::DataHolder&, std::uint64_t) const { aplThrow(NOT_SUPPORTED_ERROR); }

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
     * @return
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
class BaseObjectType : public ObjectType {
public:
    static ObjectTypeRef instance() {
        static typename T::ObjectType sObjectType;
        return &sObjectType;
    }

    STORAGE_TYPE(kStorageTypeValue);
};

template<class T>
class TrueObjectType : public BaseObjectType<T> {
public:
    bool truthy(const Object::DataHolder&) const override { return true; }
};

template<class T>
class ReferenceHolderObjectType : public BaseObjectType<T> {
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
class PointerHolderObjectType : public TrueObjectType<T> {
public:
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
class MapLikeObjectType : public ContainerObjectType<T> {
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
};

template<class T>
class MapObjectType : public MapLikeObjectType<T> {
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
class ArrayObjectType : public ContainerObjectType<T> {
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

template<class T>
class EvaluableObjectType : public PointerHolderObjectType<T> {
public:
    bool isEvaluable() const final { return true; }

    Object eval(const Object::DataHolder& dataHolder) const final {
        return dataHolder.data->eval();
    }
};

/// Primitive types

class Null {
public:
    class ObjectType final : public BaseObjectType<Null> {
    public:
        bool empty(const Object::DataHolder&) const override { return true; }

        std::string toDebugString(const Object::DataHolder&) const override { return "null"; }

        bool equals(const Object::DataHolder&, const Object::DataHolder&) const override { return true; }

        STORAGE_TYPE(kStorageTypeEmpty);
    };
};

class Boolean {
public:
    class ObjectType final : public BaseObjectType<Boolean> {
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
    class ObjectType final : public BaseObjectType<Number> {
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
            return std::isfinite(dataHolder.value) ? rapidjson::Value(dataHolder.value)
                                                      : rapidjson::Value();
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
    class ObjectType final : public BaseObjectType<String> {
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
