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

#ifndef _APL_PROPERTY_DEF_H
#define _APL_PROPERTY_DEF_H

#include "apl/engine/arrayify.h"
#include "apl/engine/properties.h"

#include "apl/animation/easing.h"
#include "apl/primitives/object.h"
#include "apl/primitives/filter.h"
#include "apl/primitives/gradient.h"
#include "apl/primitives/transform.h"
#include "apl/primitives/mediasource.h"
#include "apl/primitives/styledtext.h"
#include "apl/graphic/graphicfilter.h"
#include "apl/graphic/graphicpattern.h"

#include "apl/utils/bimap.h"

namespace apl {

inline Object asString(const Context& context, const Object& object) {
    return object.asString();
}

inline Object asBoolean(const Context &context, const Object &object) {
    return object.asBoolean();
}

inline Object asInteger(const Context &context, const Object &object) {
    return object.asInt64();
}

inline Object asArray(const Context &context, const Object &object) {
    return Object(arrayify(context, object));
}

/**
 * Return this object as an array.  For version 1.0 documents we force all arguments
 * to string form.
 * @param context The context to evaluate within.
 * @param object The object that will be returned as an array
 * @return The object as an array
 */
extern Object asOldArray(const Context &context, const Object &object);

/**
 * Return this object as a boolean.  For version 1.0 documents we'll allow the string "false"
 * to be evaluated as false (by the specification it should be true).
 * @param context The context to evaluate within.
 * @param object The object that will be checked for truthiness.
 * @return The Object::TRUE_OBJECT() or Object::FALSE_OBJECT()
 */
extern Object asOldBoolean(const Context& context, const Object& object);

inline Object asAny(const Context &context, const Object &object) {
    return object;
}

inline Object asNumber(const Context& context, const Object& object) {
    return object.asNumber();
}

inline Object asNonNegativeNumber(const Context& context, const Object& object) {
    double value = object.asNumber();
    return value < 0.0 ? 0.0 : value;
}

inline Object asNonNegativeInteger(const Context& context, const Object& object) {
    auto value = object.asInt64();
    return value < 0 ? 0 : value;
}

inline Object asPositiveInteger(const Context& context, const Object& object) {
    auto value = object.asInt64();
    return value < 1 ? 1 : value;
}

inline Object asDimension(const Context& context, const Object& object) {
    return object.asDimension(context);
}

inline Object asAbsoluteDimension(const Context& context, const Object& object) {
    return object.asAbsoluteDimension(context);
}

inline Object asNonNegativeAbsoluteDimension(const Context& context, const Object& object) {
    auto d = object.asAbsoluteDimension(context);
    if (d.getType() != DimensionType::Absolute || d.getValue() < 0)
        return Dimension(0);
    return d;
}

inline Object asNonAutoDimension(const Context& context, const Object& object) {
    return object.asNonAutoDimension(context);
}

inline Object asNonAutoRelativeDimension(const Context& context, const Object& object) {
    return object.asNonAutoRelativeDimension(context);
}

inline Object asColor(const Context& context, const Object& object) {
    return object.asColor(context);
}

inline Object asOpacity(const Context&, const Object& object) {
    double value = object.asNumber();
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

inline Object asCommand(const Context& context, const Object& object) {
    return Object(arrayify(context, object));
}


inline Object asFilterArray(const Context& context, const Object& object) {
    std::vector<Object> data;
    for (auto& m : arrayify(context, object)) {
        auto f = Filter::create(context, m);
        if (f.isFilter())
            data.push_back(std::move(f));
    }
    return Object(std::move(data));
}

inline Object asGraphicFilterArray(const Context& context, const Object& object) {
    std::vector<Object> data;
    for (auto& m : arrayify(context, object)) {
        auto f = GraphicFilter::create(context, m);
        if (f.isGraphicFilter())
            data.push_back(std::move(f));
    }
    return Object(std::move(data));
}

inline Object asStringOrArray(const Context& context, const Object& object) {
    std::vector<Object> data;
    for (auto& m : arrayify(context, object))
        data.emplace_back(m.asString());

    if (data.empty())
        return "";

    if (data.size() == 1)
        return data.front();

    return Object(std::move(data));
}

inline Object asMapped(const Context& context, const Object& object,
                       const Bimap<int, std::string>* map, const Object& defvalue )
{
    int value = map->get(object.asString(), -1);
    if (value != -1)
        return Object(value);

    return defvalue;
}

inline Object asGradient(const Context& context, const Object& object) {
    return Gradient::create(context, object);
}

inline Object asFill(const Context& context, const Object& object) {
    auto gradient = asGradient(context, object);
    return gradient.isGradient() ? gradient : asColor(context, object);
}

inline Object asMediaSourceArray(const Context& context, const Object& object) {
    std::vector<Object> data;
    for (auto& m : arrayify(context, object)) {
        auto ms = MediaSource::create(context, m);
        if (ms.isMediaSource())
            data.push_back(std::move(ms));
    }
    return Object(std::move(data));
}

inline Object asDashArray(const Context& context, const Object& object) {
    std::vector<Object> data = arrayify(context, object);
    auto size = data.size();
    if (size % 2 == 1) {
        data.resize(size * 2);
        std::copy_n(data.begin(), size, data.begin() + size);
    }
    return Object(std::move(data));
}

inline Object asStyledText(const Context& context, const Object& object) {
    return StyledText::create(context, object);
}

inline Object asFilteredText(const Context& context, const Object& object) {
    return StyledText::create(context, object).getStyledText().getText();
}

inline Object asTransformOrArray(const Context& context, const Object& object) {
    if (object.isTransform())
        return object;

    return evaluateRecursive(context, arrayify(context, object));
}

inline Object asEasing(const Context& context, const Object& object) {
    if (object.isEasing())
        return object;

    return Easing::parse(context.session(), object.asString());
}

inline Object asDeepArray(const Context &context, const Object &object) {
    return evaluateRecursive(context, arrayify(context, object));
}

inline Object asGraphicPattern(const Context& context, const Object& object) {
    return GraphicPattern::create(context, object);
}

inline Object asAvgGradient(const Context& context, const Object& object) {
    return Gradient::createAVG(context, object);
}

Object asPaddingArray(const Context& context, const Object& object);


/**
 * Flags that specify how the property definition will be used.
 */
enum PropertyDefFlags : uint32_t {
    /// This property has no flags set
    kPropNone = 0x0,
    /// This property is required. Not specifying it will prevent a component or command from inflating.
    kPropRequired = 0x01,
    /// This property is an ID.
    kPropId = 0x02,
    /// Convenience flag indicating a required ID.
    kPropRequiredId = kPropRequired | kPropId,
    /// This property is styled and may be changed when the state changes (components only).
    kPropStyled = 0x04,
    /// This property is dynamic and may be changed by the SetValue command (component only).
    kPropDynamic = 0x08,
    /// This property may be set directly by the user.
    kPropIn = 0x10,
    /// This property is passed to the view host layer - it's used to render the component on the screen.
    kPropOut = 0x20,
    /// Convenience flag for properties that are both set by the user and used to render on the screen.
    kPropInOut = kPropIn | kPropOut,
    /// This property affects the layout of components.
    kPropLayout = 0x40,
    /// This property can only be set when we're attached to the Yoga Flexbox parent
    kPropNeedsNode = 0x80,
    /// This property is a mixed property and state (such as "checked" or "disabled")
    kPropMixedState = 0x100,
    /// This property should be reset on ::remove()
    kPropResetOnRemove = 0x200,
    /// This property is used by the view host layer to retrieve the component state.
    kPropRuntimeState = 0x400,
    /// This property should be evaluated recursively as it can contain data bindings.
    kPropEvaluated = 0x800,
    /// This property may influence the visual context dirty state.
    kPropVisualContext = 0x1000,
    /// This property can only be set once the children of this component have been laid out
    kPropSetAfterLayout = 0x2000,
    /// This property takes part in text measurement request hash
    kPropTextHash = 0x4000,
    /// This property takes part in visual hash
    kPropVisualHash = 0x8000,
};

/**
 * Templated version of a property definition.
 * @tparam K The enumerated class defining the keys
 * @tparam bimap The bimap definition
 */
template<class K, Bimap<int, std::string> &bimap>
struct PropDef {
public:

    /**
     * Create a property definition of a typed property.
     * @param key The key for the property.  The PropDef bimap will be used to retrieve the string name.
     * @param defvalue The default value for the property. This will be used if it is not specified by the end user.
     * @param func A conversion function that takes an Object and converts it into the correct type for this property.
     * @param flags A collection of flags specifying how to handle this property.
     * @param trigger An optional trigger function to execute whenever this property changes value.
     */
    PropDef(K key, const Object& defvalue, BindingFunction func, int flags=0)
        : key(key),
          names(bimap.all(key)),
          defvalue(defvalue),
          func(func),
          flags(flags),
          map(nullptr)
    {}

    /**
     * Create a property definition of a property that is a string lookup in a table
     * @param key The key for the property.  The PropDef bimap will be used to retrieve the string name.
     * @param defvalue The default value for the property. This will be used if it is not specified by the end user.
     * @param map A bi-map between the property value (which is a string) and the integer value to store.
     * @param flags A collection of flags specifying how to handle this property.
     * @param trigger An optional trigger function to execute whenever this property changes value.
     */
    PropDef(K key, int defvalue, Bimap<int, std::string>& map, int flags=0)
        : key(key),
          names(bimap.all(key)),
          defvalue(defvalue),
          func(nullptr),
          flags(flags),
          map(&map)
    {}

    PropDef& operator=(const PropDef& rhs) = default;

    /**
     * Evaluate an assigned value by either converting it to the correct type or by using the string lookup table
     * to return the integer mapped value.
     * @param context The data-binding context (used in mapped lookup).
     * @param value The value to evaluate.
     * @return The calculated value.
     */
    Object calculate(const Context& context, Object value) const {
        return map ? asMapped(context, value, map, defvalue) : func(context, value);
    }

    /**
     * TODO: Move the binding function directly into the prop definition itself to avoid creating it each time.
     * @return A binding function that can be applied.  This strongly depends on the PropDef remaining fixed.
     */
    BindingFunction getBindingFunction() const
    {
        if (map)
            return [&](const Context& context, Object value) {
                return asMapped(context, value, map, defvalue);
            };

        return [&](const Context& context, Object value) {
            return func(context, value);
        };
    }

    K key;
    std::vector<std::string> names;
    Object defvalue;
    BindingFunction func;
    int flags;
    Bimap<int, std::string> *map;
};

/**
 * A property definition set is an array of properties defined for a component or command.
 *
 * @tparam K The type of the valid property keys.  This is usually an enumerated value.
 * @tparam bimap A bimap between the property keys and their string representation.
 */
template<class K, Bimap<int, std::string>&bimap, class PDef>
class PropDefSet {
public:
    // using PDef = PropDef<K, bimap>;
    using PVec = std::vector<PDef>;
    using PMap = std::map<K, PDef>;

    static inline void addToMap(std::map<K, PDef>& map, const PDef& m) {
        auto it = map.find(m.key);
        if (it != map.end())
            map.erase(it);
        map.emplace(m.key, m);
    }

    PropDefSet() = default;
    PropDefSet(const PropDefSet& other) = default;

    /**
     * Construct the set copying an existing property definition set and then merging in a list of property definitions.
     * @param other The existing PropDefSet.
     * @param list The list of property definitions to merge.
     */
    PropDefSet(const PropDefSet& other, const std::vector<PDef>& list)
        : PropDefSet(other)
    {
        add(list);
    }

    const typename std::map<K, PDef>::const_iterator begin() const { return mOrdered.begin(); }
    const typename std::map<K, PDef>::const_iterator end() const { return mOrdered.end(); }
    const typename std::map<K, PDef>::const_iterator find(K key) const { return mOrdered.find(key); }

protected:
    void addInternal(const PVec& list) {
        for (const PDef& m : list)
            addToMap(mOrdered, m);
    }
private:
    PMap mOrdered;
};


}

#endif //_APL_PROPERTY_DEF_H
