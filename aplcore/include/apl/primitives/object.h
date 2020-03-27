/*
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
 *    Node
 *    JSONObject
 *    JSONArray
 *    IOptArray
 *    IOptMap
 *    Function
 */

#ifndef _APL_OBJECT_H
#define _APL_OBJECT_H

#include <memory>
#include <string>
#include <exception>
#include <memory>
#include <map>
#include <vector>
#include <cassert>
#include <cmath>
#include <set>

#include "rapidjson/document.h"

#include "apl/common.h"
#include "apl/primitives/gradient.h"
#include "apl/utils/visitor.h"
#include "apl/primitives/styledtext.h"
#include "apl/primitives/radii.h"
#include "apl/primitives/rect.h"
#include "apl/primitives/symbolreferencemap.h"
#include "apl/primitives/transform2d.h"
#include "apl/animation/easing.h"

#ifdef APL_CORE_UWP
    #undef TRUE
    #undef FALSE
#endif

namespace apl {

namespace datagrammar {
    class Node;
    class BoundSymbol;
}

class Object;
class Color;
class Dimension;
class Filter;
class Function;
class Transformation;
class MediaSource;
class LiveDataObject;

using ObjectMap = std::map<std::string, Object>;
using ObjectMapPtr = std::shared_ptr<ObjectMap>;
using ObjectArray = std::vector<Object>;
using ObjectArrayPtr = std::shared_ptr<ObjectArray>;

/// @deprecated
using SharedMapPtr = ObjectMapPtr;
/// @deprecated
using SharedVectorPtr = ObjectArrayPtr;

/**
 * A single Object which can hold a variety of types.
 * Most objects are of type null, boolean, number, or string.  They all fit within
 * this basic object.  Other possibilities include nodes (for expression evaluate),
 * maps (for context and for JSONObject) and arrays (vectors or JSONArray).
 *
 * To avoid dynamic casting, the base object has methods for manipulating all of these
 * types.  The types that require additional storage put a shared_ptr in a single data
 * property.
 *
 * Note that certain types stored in Objects are treated as immutable and certain types
 * are mutable.  The immutable types are:
 *
 * - Null
 * - Boolean
 * - Number
 * - String
 * - Array
 * - Map (object)
 * - Function
 * - Dimensions (absolute, relative, and auto)
 * - Colors
 * - Filters
 * - Gradients
 * - Media sources
 * - Rectangles
 * - Radii
 * - Styled Text
 * - 2D Transformations
 * - Bound Symbol
 *
 * The mutable types are:
 * - Vector graphic
 * - Generalized transformation
 * - Node
 */
class Object
{
public:
    enum ObjectType {
        kNullType,
        kBoolType,
        kNumberType,
        kStringType,
        kArrayType,
        kMapType,
        kNodeType,
        kFunctionType,
        kAbsoluteDimensionType,
        kRelativeDimensionType,
        kAutoDimensionType,
        kColorType,
        kFilterType,
        kGradientType,
        kMediaSourceType,
        kRectType,
        kRadiiType,
        kStyledTextType,
        kGraphicType,
        kTransformType,
        kTransform2DType,
        kEasingType,
        kBoundSymbolType
    };

    class Data;  // Forward declaration

    // Constructors
    Object();
    Object(ObjectType type);
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
    Object(const std::shared_ptr<datagrammar::Node>& n);
    Object(const ObjectMapPtr& m, bool isMutable=false);
    Object(const ObjectArrayPtr& v, bool isMutable=false);
    Object(ObjectArray&& v, bool isMutable=false);
    Object(const rapidjson::Value& v);
    Object(rapidjson::Document&& doc);
    Object(const std::shared_ptr<Function>& f);
    Object(const Color& color);
    Object(const Dimension& dimension);
    Object(Filter&& filter);
    Object(Gradient&& gradient);
    Object(MediaSource&& mediaSource);
    Object(Rect&& rect);
    Object(Radii&& radii);
    Object(StyledText&& styledText);
    Object(const GraphicPtr& graphic);
    Object(const std::shared_ptr<Transformation>& transform);
    Object(Transform2D&& transform2d);
    Object(Easing&& easingCurve);
    Object(const std::shared_ptr<datagrammar::BoundSymbol>& b);
    Object(const std::shared_ptr<LiveDataObject>& d);

    // Statically initialized objects.
    static const Object& TRUE_OBJECT();
    static const Object& FALSE_OBJECT();
    static const Object& NULL_OBJECT();
    static Object NAN_OBJECT();
    static Object AUTO_OBJECT();
    static Object EMPTY_ARRAY();
    static Object EMPTY_MUTABLE_ARRAY();
    static Object EMPTY_MAP();
    static Object EMPTY_MUTABLE_MAP();
    static Object ZERO_ABS_DIMEN();
    static Object EMPTY_RECT();
    static Object EMPTY_RADII();
    static Object IDENTITY_2D();
    static Object LINEAR_EASING();

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

    bool isNull() const { return mType == kNullType; }
    bool isBoolean() const { return mType == kBoolType; }
    bool isString() const { return mType == kStringType; }
    bool isNumber() const { return mType == kNumberType; }
    bool isNaN() const { return mType == kNumberType && std::isnan(mValue); }
    bool isArray() const { return mType == kArrayType; }
    bool isMap() const { return mType == kMapType; }
    bool isNode() const { return mType == kNodeType; }
    bool isBoundSymbol() const { return mType == kBoundSymbolType; }
    bool isEvaluable() const { return mType == kNodeType || mType == kBoundSymbolType; }
    bool isFunction() const { return mType == kFunctionType; }
    bool isAbsoluteDimension() const { return mType == kAbsoluteDimensionType; }
    bool isRelativeDimension() const { return mType == kRelativeDimensionType; }
    bool isAutoDimension() const { return mType == kAutoDimensionType; }
    bool isNonAutoDimension() const { return mType == kAbsoluteDimensionType ||
                                             mType == kRelativeDimensionType; }
    bool isDimension() const { return mType == kAutoDimensionType ||
                                      mType == kRelativeDimensionType ||
                                      mType == kAbsoluteDimensionType; }
    bool isColor() const { return mType == kColorType; }
    bool isFilter() const { return mType == kFilterType; }
    bool isGradient() const { return mType == kGradientType; }
    bool isMediaSource() const { return mType == kMediaSourceType; }
    bool isRect() const { return mType == kRectType; }
    bool isRadii() const { return mType == kRadiiType; }
    bool isStyledText() const { return mType == kStyledTextType; }
    bool isGraphic() const { return mType == kGraphicType; }
    bool isTransform() const { return mType == kTransformType; }
    bool isTransform2D() const { return mType == kTransform2DType; }
    bool isEasing() const { return mType == kEasingType; }
    bool isJson() const;

    // These methods force a conversion to the appropriate type.  We try to return
    // plausible defaults whenever possible.
    std::string asString() const;
    bool asBoolean() const { return truthy(); }
    double asNumber() const;
    int asInt() const;
    Dimension asDimension(const Context& ) const;
    Dimension asAbsoluteDimension(const Context&) const;
    Dimension asNonAutoDimension(const Context&) const;
    Dimension asNonAutoRelativeDimension(const Context&) const;
    /// @deprecated This method will be removed soon.
    Color asColor() const;
    Color asColor(const SessionPtr&) const;
    Color asColor(const Context&) const;

    // These methods return the actual contents of the Object
    const std::string& getString() const { assert(mType == kStringType); return mString; }
    bool getBoolean() const { assert(mType == kBoolType); return mValue != 0; }
    double getDouble() const { assert(mType == kNumberType); return mValue; }
    int getInteger() const { assert(mType == kNumberType); return static_cast<int>(std::rint(mValue)); }
    unsigned getUnsigned() const { assert(mType == kNumberType); return static_cast<unsigned>(mValue);}
    double getAbsoluteDimension() const { assert(mType == kAbsoluteDimensionType); return mValue; }
    double getRelativeDimension() const { assert(mType == kRelativeDimensionType); return mValue; }
    uint32_t getColor() const { assert(mType == kColorType); return static_cast<uint32_t>(mValue); }

    std::shared_ptr<Function> getFunction() const;
    std::shared_ptr<datagrammar::BoundSymbol> getBoundSymbol() const;
    std::shared_ptr<LiveDataObject> getLiveDataObject() const;
    std::shared_ptr<datagrammar::Node> getNode() const;

    const ObjectMap& getMap() const {
        assert(mType == kMapType); return mData->getMap();
    }

    ObjectMap& getMutableMap() {
        assert(mType == kMapType); return mData->getMutableMap();
    }

    const ObjectArray& getArray() const {
        assert(mType == kArrayType); return mData->getArray();
    }

    ObjectArray& getMutableArray() {
        assert(mType == kArrayType); return mData->getMutableArray();
    }

    const Filter& getFilter() const {
        assert(mType == kFilterType); return mData->getFilter();
    }

    const Gradient& getGradient() const {
        assert(mType == kGradientType); return mData->getGradient();
    }
    const MediaSource& getMediaSource() const {
        assert(mType == kMediaSourceType); return mData->getMediaSource();
    }

    GraphicPtr getGraphic() const {
        assert(mType == kGraphicType); return mData->getGraphic();
    }

    Rect getRect() const {
        assert(mType == kRectType); return mData->getRect();
    }

    Radii getRadii() const {
        assert(mType == kRadiiType); return mData->getRadii();
    }

    const StyledText& getStyledText() const {
        assert(mType == kStyledTextType); return mData->getStyledText();
    }

    std::shared_ptr<Transformation> getTransformation() const {
        assert(mType == kTransformType); return mData->getTransform();
    }

    Transform2D getTransform2D() const {
        assert(mType == kTransform2DType); return mData->getTransform2D();
    }

    Easing getEasing() const {
        assert(mType == kEasingType); return mData->getEasing();
    }

    const rapidjson::Value& getJson() const {
        assert(isJson());
        return *(mData->getJson());
    }

    bool truthy() const;

    // MAP objects
    const Object get(const std::string& key) const;
    bool has(const std::string& key) const;
    const Object opt(const std::string& key, const Object& def) const;

    // ARRAY objects
    const Object at(size_t index) const;

    const ObjectType getType() const;

    // MAP, ARRAY, and STRING objects
    size_t size() const;

    // NULL, MAP, ARRAY, RECT, and STRING objects
    bool empty() const;

    // Mutable objects
    bool isMutable() const;

    // NODE & BoundSymbol objects
    Object eval() const;

    // NODE & BoundSymbol objects
    bool isPure() const;

    // NODE & BoundSymbol: Add any symbols defined by this node to the "symbols" set
    void symbols(SymbolReferenceMap& symbols) const;

    // FUNCTION objects
    Object call(ObjectArray&& args) const;

    // Visitor pattern
    void accept(Visitor<Object>& visitor) const;

    // Convert this to a printable string. Not to be confused with "asString" or "getString"
    std::string toDebugString() const;

    friend streamer& operator<<(streamer&, const Object&);

    // Serialize to JSON format
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    // Serialize just the dirty bits to JSON format
    rapidjson::Value serializeDirty(rapidjson::Document::AllocatorType& allocator) const;

    // Internal class for holding a shared pointer
    class Data {
    public:
        virtual ~Data() = default;

        virtual const Object get(const std::string& key) const { return Object::NULL_OBJECT(); }
        virtual bool has(const std::string& key) const { return false; }
        virtual const Object opt(const std::string& key, const Object& def) const { return def; }

        virtual const Object at(size_t index) const { return Object::NULL_OBJECT(); }
        virtual size_t size() const { return 0; }
        virtual bool empty() const { return false; }

        virtual bool isMutable() const { return false; }

        virtual Object eval() const { return Object::NULL_OBJECT(); }

        virtual Object call(const ObjectArray& args) const { return Object::NULL_OBJECT(); }

        virtual void accept(Visitor<Object>& visitor) const {}

        virtual const ObjectArray& getArray() const {
            throw std::runtime_error("Illegal array");
        }

        virtual ObjectArray& getMutableArray() {
            throw std::runtime_error("Illegal mutable array");
        }

        virtual const ObjectMap& getMap() const {
            throw std::runtime_error("Illegal map");
        }

        virtual ObjectMap& getMutableMap() {
            throw std::runtime_error("Illegal mutable map");
        }

        virtual const Filter& getFilter() const {
            throw std::runtime_error("Illegal filter");
        }

        virtual const Gradient& getGradient() const {
            throw std::runtime_error("Illegal gradient");
        }

        virtual const MediaSource& getMediaSource() const {
            throw std::runtime_error("Illegal media source");
        }

        virtual Rect getRect() const {
            throw std::runtime_error("Illegal rectangle");
        }

        virtual Radii getRadii() const {
            throw std::runtime_error("Illegal radii");
        }

        virtual const StyledText& getStyledText() const {
            throw std::runtime_error("Illegal styled text");
        }

        virtual GraphicPtr getGraphic() const {
            throw std::runtime_error("Illegal graphic");
        }

        virtual std::shared_ptr<Transformation> getTransform() const {
            throw std::runtime_error("Illegal transform");
        }

        virtual Transform2D getTransform2D() const {
            throw std::runtime_error("Illegal transform 2D");
        }

        virtual Easing getEasing() const {
            throw std::runtime_error("Illegal easing curve");
        }

        virtual const rapidjson::Value* getJson() const {
            return nullptr;
        }

        virtual std::string toDebugString() const {
            return "Unknown type";
        }
    };

private:
    // In the future we should use a union, but that precludes common std library use
    // If we can bump to C++17, we can get std::variant
    // For now, we'll run fast and dirty

    ObjectType mType;
    double mValue = 0;
    std::string mString;
    std::shared_ptr<Data> mData;

    // TODO:  Many of our children are just shared pointers.  Rather than stash them
    // TODO:  inside of mData, let's make a common base class that anything stored in
    // TODO:  an object can inherit from: std::shared_ptr<CommonBase> mData.
    // TODO:  Then getGraphic becomes return std::dynamic_pointer_cast<Graphic>(mData);
};


}  // namespace apl

#endif // _APL_OBJECT_H
