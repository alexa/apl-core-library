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

#ifndef _APL_OBJECT_DATA_H
#define _APL_OBJECT_DATA_H

#include <stdexcept>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "apl/primitives/object.h"
#include "apl/primitives/styledtext.h"
#include "apl/utils/noncopyable.h"

namespace apl {

// Internal class for holding a shared pointer
class ObjectData : public NonCopyable {
public:
    virtual ~ObjectData() = default;

    /**
     * Get a value from the internal map.  Only works if the object is a map (see isMap()).
     * @param key The key to retrieve.
     * @return The value of the mapped key.  Returns NULL if the key doesn't exist.
     */
    virtual Object get(const std::string &key) const { return Object::NULL_OBJECT(); }

    /**
     * Check if the object has a key in the internal map.  Only works if the object is a map (see isMap()).
     * @param key The key to check.
     * @return True if the key exists.
     */
    virtual bool has(const std::string &key) const { return false; }

    /**
     * Get a value from the internal map or return a default value if it doesn't exist.
     * Only works if the object is a map (see isMap()).
     * @param key The key to retrieve
     * @param def The default value to return
     * @return The value of the mapped key or def if the key doesn't exist
     */
    virtual Object opt(const std::string &key, const Object &def) const { return def; }

    /**
     * Return a value at the given index in the array.  Only works if the object is an array (see isArray()).
     * @param index The index to return.
     * @return The value at that point in the array or NULL if the array index is out of bounds.
     */
    virtual Object at(std::uint64_t index) const { return Object::NULL_OBJECT(); }

    /**
     * @return The size of the array or map.  Not all maps will return an accurate size.
     */
    virtual std::uint64_t size() const { return 0; }

    /**
     * @return True if the array or map is empty; false otherwise.
     */
    virtual bool empty() const { return false; }

    /**
     * @return The truthy value of this object
     */
    virtual bool truthy() const { return true; }

    /**
     * @return True if this object is mutable
     */
    virtual bool isMutable() const { return false; }

    /**
     * @return The evaluation of this object.  Most objects return NULL.
     */
    virtual Object eval() const { return Object::NULL_OBJECT(); }

    /**
     * Call this object like a function, passing an array of arguments.  Most objects return NULL.
     * @param args The arguments to pass.
     * @return The returned value or NULL.
     */
    virtual Object call(const ObjectArray& args) const { return Object::NULL_OBJECT(); }

    /**
     * Accept a visitor pattern to iterate over the object.
     * @param visitor The visitor.
     */
    virtual void accept(Visitor<Object> &visitor) const {}

    /**
     * An internal method to return a pointer to the contained object.  This method is an implementation
     * detail for certain templated classes.
     * @return The inner object.
     */
    virtual const void *inner() const {
        throw std::runtime_error("Illegal inner reference");
    }

    /**
     * Compare two ObjectData objects.  This routine REQUIRES that the two objects share a common type.
     * @param rhs The object to compare against.
     * @return True if the objects are equal.
     */
    virtual bool operator==(const ObjectData& rhs) const {
        return this == &rhs;
    }

    virtual rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const {
        throw std::runtime_error("Illegal serialize call");
    }

    virtual const ObjectArray &getArray() const {
        throw std::runtime_error("Illegal array");
    }

    virtual ObjectArray &getMutableArray() {
        throw std::runtime_error("Illegal mutable array");
    }

    virtual const ObjectMap &getMap() const {
        throw std::runtime_error("Illegal map");
    }

    virtual ObjectMap &getMutableMap() {
        throw std::runtime_error("Illegal mutable map");
    }

    virtual GraphicPtr getGraphic() const {
        throw std::runtime_error("Illegal graphic");
    }

    virtual std::shared_ptr<Transformation> getTransform() const {
        throw std::runtime_error("Illegal transform");
    }

    virtual const rapidjson::Value *getJson() const {
        return nullptr;
    }

    virtual std::string toDebugString() const {
        return "Unknown type";
    }
};

/****************************************************************************/

class ArrayData : public ObjectData {
public:
    ArrayData(const ObjectArrayPtr& array, bool isMutable) : mArray(array), mIsMutable(isMutable) {
        assert(array);
    }

    Object
    at(std::uint64_t index) const override
    {
        std::uint64_t len = mArray->size();
        if (index >= len)
            return Object::NULL_OBJECT();

        return mArray->at(index);
    }

    std::uint64_t size() const override { return mArray->size(); }
    bool empty() const override { return mArray->empty(); }

    bool isMutable() const override { return mIsMutable; }

    void
    accept(Visitor<Object>& visitor) const override
    {
        visitor.push();
        for (auto it = mArray->begin() ; !visitor.isAborted() && it != mArray->end() ; it++)
            it->accept(visitor);
        visitor.pop();
    }

    const std::vector<Object>& getArray() const override {
        return *mArray;
    }

    std::vector<Object>& getMutableArray() override {
        if (!mIsMutable)
            throw std::runtime_error("Attempted to retrieve mutable array for non-mutable object");
        return *mArray;
    }

    std::string toDebugString() const override {
        std::string result = "Array<size=" + std::to_string(mArray->size()) + ">[";
        for (const auto& m : *mArray) {
            result += m.toDebugString();
            result += ", ";
        }
        result += "]";

        return result;
    }

private:
    ObjectArrayPtr mArray;
    bool mIsMutable;
};

/****************************************************************************/

class FixedArrayData : public ObjectData {
public:
    FixedArrayData(ObjectArray&& array, bool isMutable) : mArray(std::move(array)), mIsMutable(isMutable) {}

    Object
    at(std::uint64_t index) const override
    {
        std::uint64_t len = mArray.size();
        if (index >= len)
            return Object::NULL_OBJECT();

        return mArray.at(index);
    }

    std::uint64_t size() const override {
        return mArray.size();
    }

    bool empty() const override {
        return mArray.empty();
    }

    bool isMutable() const override {
        return mIsMutable;
    }

    void
    accept(Visitor<Object>& visitor) const override
    {
        visitor.push();
        for (auto it = mArray.begin() ; !visitor.isAborted() && it != mArray.end() ; it++)
            it->accept(visitor);
        visitor.pop();
    }

    const std::vector<Object>& getArray() const override {
        return mArray;
    }

    std::vector<Object>& getMutableArray() override {
        if (!mIsMutable)
            throw std::runtime_error("Attempted to retrieve mutable array for non-mutable object");
        return mArray;
    }

    std::string toDebugString() const override {
        std::string result = "FixedArray<size=" + std::to_string(mArray.size()) + ">[";
        for (const auto& m : mArray) {
            result += m.toDebugString();
            result += ", ";
        }
        result += "]";

        return result;
    }

private:
    ObjectArray mArray;
    bool mIsMutable;
};

/****************************************************************************/

class MapData : public ObjectData {
public:
    explicit MapData(const std::shared_ptr<ObjectMap>& map, bool isMutable) : mMap(map), mIsMutable(isMutable) {
        assert(map);
    }

    Object
    get(const std::string& key) const override
    {
        auto it = mMap->find(key);
        if (it != mMap->end())
            return it->second;
        return Object::NULL_OBJECT();
    }

    Object
    opt(const std::string& key, const Object& def) const override
    {
        auto it = mMap->find(key);
        if (it != mMap->end())
            return it->second;
        return def;
    }

    std::uint64_t size() const override { return mMap->size(); }
    bool empty() const override { return mMap->empty(); }
    bool isMutable() const override { return mIsMutable; }
    bool has(const std::string& key) const override { return mMap->count(key) != 0; }

    const ObjectMap& getMap() const override {
        return *mMap;
    }

    ObjectMap& getMutableMap() override {
        if (!mIsMutable)
            throw std::runtime_error("Attempted to retrieve mutable map for non-mutable object");
        return *mMap;
    }

    void
    accept(Visitor<Object>& visitor) const override
    {
        visitor.push();
        for (auto it = mMap->begin() ; !visitor.isAborted() && it != mMap->end() ; it++) {
            Object(it->first).accept(visitor);
            if (!visitor.isAborted()) {
                visitor.push();
                it->second.accept(visitor);
                visitor.pop();
            }
        }
        visitor.pop();
    }

    std::string toDebugString() const override {
        std::string result = "Map<size=" + std::to_string(mMap->size()) + ">[";
        for (const auto& m : *mMap) {
            result += "{'" + m.first + "': " + m.second.toDebugString() + "}, ";
        }
        result += "]";
        return result;
    }
private:
    ObjectMapPtr mMap;
    bool mIsMutable;
};


/****************************************************************************/

class JSONData : public ObjectData {
public:
    JSONData(const rapidjson::Value *value) : mValue(value) { assert(value); }

    Object
    get(const std::string& key) const override
    {
        if (mValue->IsObject()) {
            auto it = mValue->FindMember(key.c_str());
            if (it != mValue->MemberEnd())
                return it->value;
        }
        return Object::NULL_OBJECT();
    }

    Object
    opt(const std::string& key, const Object& def) const override
    {
        if (mValue->IsObject()) {
            auto it = mValue->FindMember(key.c_str());
            if (it != mValue->MemberEnd())
                return it->value;
        }
        return def;
    }

    bool
    has(const std::string& key) const override {
        if (!mValue->IsObject())
            return false;

        return mValue->FindMember(key.c_str()) != mValue->MemberEnd();
    }

    Object
    at(std::uint64_t index) const override {
        if (!mValue->IsArray() || index >= mValue->Size())
            return Object::NULL_OBJECT();
        return (*mValue)[index];
    }

    std::uint64_t
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
        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        mValue->Accept(writer);
        return "JSON<" + std::string(buffer.GetString()) + ">";
    }

private:
    const rapidjson::Value *mValue;
    const std::map<std::string, Object> mMap;
    const std::vector<Object> mVector;
};

/****************************************************************************/

class JSONDocumentData : public ObjectData {
public:
    JSONDocumentData(rapidjson::Document&& doc) : mDoc(std::move(doc)) {}

    Object
    get(const std::string& key) const override
    {
        if (mDoc.IsObject()) {
            auto it = mDoc.FindMember(key.c_str());
            if (it != mDoc.MemberEnd())
                return it->value;
        }
        return Object::NULL_OBJECT();
    }

    Object
    opt(const std::string& key, const Object& def) const override
    {
        if (mDoc.IsObject()) {
            auto it = mDoc.FindMember(key.c_str());
            if (it != mDoc.MemberEnd())
                return it->value;
        }
        return def;
    }

    bool
    has(const std::string& key) const override {
        if (!mDoc.IsObject())
            return false;

        return mDoc.FindMember(key.c_str()) != mDoc.MemberEnd();
    }

    Object
    at(std::uint64_t index) const override {
        if (!mDoc.IsArray() || index >= mDoc.Size())
            return Object::NULL_OBJECT();
        return mDoc[index];
    }

    std::uint64_t
    size() const override {
        if (mDoc.IsArray())
            return mDoc.Size();
        if (mDoc.IsObject())
            return mDoc.MemberCount();
        return 0;
    }

    bool
    empty() const override {
        if (mDoc.IsArray())
            return mDoc.Empty();
        if (mDoc.IsObject())
            return mDoc.MemberCount() == 0;
        return false;
    }

    const std::vector<Object>& getArray() const override {
        assert(mDoc.IsArray());

        if (mDoc.Size() != mVector.size())
            for (const auto& v : mDoc.GetArray())
                const_cast<std::vector<Object>&>(mVector).emplace_back(v);
        return mVector;
    }

    const std::map<std::string, Object>& getMap() const override {
        assert(mDoc.IsObject());

        if (mDoc.MemberCount() != mMap.size())
            for (const auto& v : mDoc.GetObject())
                const_cast<std::map<std::string, Object>&>(mMap).emplace(v.name.GetString(), v.value);
        return mMap;
    }

    const rapidjson::Value* getJson() const override {
        return &mDoc;
    }

    std::string toDebugString() const override {
        return "JSONDoc<size=" + std::to_string(size()) + ">";
    }

private:
    const rapidjson::Document mDoc;
    const std::map<std::string, Object> mMap;
    const std::vector<Object> mVector;
};

/****************************************************************************/

class GraphicData : public ObjectData {
public:
    GraphicData(const GraphicPtr& graphic) : mGraphic(graphic) {}
    GraphicPtr getGraphic() const override { return mGraphic; }

    std::string toDebugString() const override {
        return "Graphic<>";
    }

private:
    const GraphicPtr mGraphic;
};

/****************************************************************************/

class TransformData : public ObjectData {
public:
    TransformData(const std::shared_ptr<Transformation>& transform) : mTransform(transform) {}
    std::shared_ptr<Transformation> getTransform() const override { return mTransform; }

    std::string toDebugString() const override {
        return "Transform<>";
    }

private:
    std::shared_ptr<Transformation> mTransform;
};

/****************************************************************************/

/**
 * Objects stored inside of DirectObjectData should support the following methods:
 *
 *   std::string toDebugString() const;
 *   bool operator==( const T& other ) const;
 *   bool empty() const;
 *   rapidjson::Value serialize(rapidjson::document::AllocatorType& allocator) const;
 *
 * @tparam T
 */
template<typename T>
class DirectObjectData : public ObjectData {
public:
    static std::shared_ptr<DirectObjectData<T>> create(T &&data) {
        return std::make_shared<DirectObjectData<T>>(std::move(data));
    }

    explicit DirectObjectData(T &&data) : mData(std::move(data)) {}

    /**
     * Internal method for accessing the inner object stored here.
     * Eventually we will shift this to return const T&
     * @return
     */
    const void *inner() const override { return &mData; }

    bool empty() const override { return mData.empty(); }

    bool truthy() const override { return mData.truthy(); }

    std::string toDebugString() const override { return mData.toDebugString(); }

    bool operator==(const ObjectData& other) const override {
        return mData == static_cast<const DirectObjectData<T> &>(other).mData;
    }

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override {
        return mData.serialize(allocator);
    }

    /**
     * Static type information for this direct object category.
     */
    static const Object::ObjectType sType;

private:
    T mData;
};

} // namespace apl

#endif // _APL_OBJECT_DATA_H
