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

#ifndef _APL_COMPONENT_PROP_DEF_H
#define _APL_COMPONENT_PROP_DEF_H

#include "component.h"
#include "apl/engine/propdef.h"

namespace apl {

using Trigger = void (*)(Component&);
using LayoutFunc = void (*)(YGNodeRef nodeRef, const Object& object, const Context& context);
using DefaultFunc = Object (*)(Component&, const RootConfig&);
using GetterFunc = Object (*)(const CoreComponent&);
using SetterFunc = void (*)(CoreComponent&, const Object&);

/**
 * A component property definition is precompiled set of information on how to handle a
 * single component property.  Each property has a human-readable name, an enumerated key,
 * and a default value.  Properties may have either a known type (such as "color") or may be
 * an enumerated value looked up from a table.
 *
 * This class extends the PropDef template to add a trigger function to execute when the property
 * changes value and a layout function to execute to update the YogaNode based on the properties value.
 *
 * For convenience, we review how the flags and triggers are used:
 *
 * Flags:
 * <pre>
 *    kPropIn        The property is specified by the user.  The name of the property should
 *                   match the APL specification.
 *
 *    kPropOut       A change in this property will set the dirty flag.  The property is used
 *                   by the view host for drawing.
 *
 *    kPropRequired  This property must be present or the component will not be usable.
 *
 *    kPropStyled    This property may be set by a style.
 *
 *    kPropDynamic   This property may be changed dynamically with the SetValue command.
 *
 *    kPropLayout    A change in this property will trigger a layout pass
 *
 *    kPropNone      This property is not specified by the user; it is assigned by the system (neither kPropIn or kPropOut)
 * </pre>
 *
 * A "trigger" is a function to execute whenever the property changes value (from a style change or
 * a SetValue command).  Triggers do not run when the component is first being inflated.
 *
 * A "layoutFunc" is a function that updates the YogaNode.  It will be called as necessary when the
 * node is attached or when a property changes value and needs to update the node.
 */
class ComponentPropDef : public PropDef<PropertyKey, sComponentPropertyBimap> {
public:
    ComponentPropDef(PropertyKey key, const Object& defvalue, BindingFunction func, int flags)
        : ComponentPropDef(key, defvalue, func, flags, nullptr, nullptr, nullptr) {}

    ComponentPropDef(PropertyKey key, const Object& defvalue, BindingFunction func, int flags, LayoutFunc layoutFunc)
        : ComponentPropDef(key, defvalue, func, flags, layoutFunc, nullptr, nullptr) {}

    ComponentPropDef(PropertyKey key, const Object& defvalue, BindingFunction func, int flags, Trigger trigger)
        : ComponentPropDef(key, defvalue, func, flags, nullptr, trigger, nullptr) {}

    ComponentPropDef(PropertyKey key, const Object& defvalue, BindingFunction func, int flags, DefaultFunc defaultFunc)
        : ComponentPropDef(key, defvalue, func, flags, nullptr, nullptr, defaultFunc) {}

    ComponentPropDef(PropertyKey key, const Object& defvalue, BindingFunction func, int flags, LayoutFunc layoutFunc, Trigger trigger)
        : ComponentPropDef(key, defvalue, func, flags, layoutFunc, trigger, nullptr) {}

    ComponentPropDef(PropertyKey key, const Object& defvalue, BindingFunction func, int flags, Trigger trigger, DefaultFunc defaultFunc)
        : ComponentPropDef(key, defvalue, func, flags, nullptr, trigger, defaultFunc) {}

    ComponentPropDef(PropertyKey key, const Object& defvalue, BindingFunction func, int flags, LayoutFunc layoutFunc, DefaultFunc defaultFunc)
        : ComponentPropDef(key, defvalue, func, flags, layoutFunc, nullptr, defaultFunc) {}

    /**
     * Create a property definition of a typed property.
     * @param key The key for the property.  The PropDef bimap will be used to retrieve the string name.
     * @param defvalue The default value for the property. This will be used if it is not specified by the end user.
     * @param func A conversion function that takes an Object and converts it into the correct type for this property.
     * @param flags A collection of flags specifying how to handle this property.
     * @param layoutFunc A function that updates the YogaNode when the property changes value.
     * @param trigger An optional trigger function to execute whenever this property changes value.
     * @param defaultFunc A function that returns the property default value based on the root configuration
     */
    ComponentPropDef(PropertyKey key, const Object& defvalue, BindingFunction func, int flags, LayoutFunc layoutFunc,
                     Trigger trigger, DefaultFunc defaultFunc)
        : PropDef(key, defvalue, func, flags),
          trigger(trigger),
          layoutFunc(layoutFunc),
          defaultFunc(defaultFunc)
    {}


    ComponentPropDef(PropertyKey key, int defvalue, Bimap<int, std::string>& map, int flags)
        : ComponentPropDef(key, defvalue, map, flags, nullptr, nullptr) {}

    ComponentPropDef(PropertyKey key, int defvalue, Bimap<int, std::string>& map, int flags, Trigger trigger)
        : ComponentPropDef(key, defvalue, map, flags, nullptr, trigger) {}

    /**
     * Create a property definition of a property that is a string lookup in a table
     * @param key The key for the property.  The PropDef bimap will be used to retrieve the string name.
     * @param defvalue The default value for the property. This will be used if it is not specified by the end user.
     * @param map A bi-map between the property value (which is a string) and the integer value to store.
     * @param flags A collection of flags specifying how to handle this property.
     * @param layoutFunc A function that updates the YogaNode when the property changes value.
     * @param trigger An optional trigger function to execute whenever this property changes value.
     */
    ComponentPropDef(PropertyKey key, int defvalue, Bimap<int, std::string>& map, int flags, LayoutFunc layoutFunc,
                     Trigger trigger = nullptr)
        : PropDef(key, defvalue, map, flags),
          trigger(trigger),
          layoutFunc(layoutFunc),
          defaultFunc(nullptr) {}

    /**
     * Create a property definition of a virtual property that has a getter and a setter (optional).
     * @param key The key for the property.  The PropDef bimap will be used to retrieve the string name
     * @param getter Getter function for retrieving the property
     * @param setter Setter function for setting the property
     * @param flags A collection of flags specifying how to handle this property
     */
    ComponentPropDef(PropertyKey key, GetterFunc getter, SetterFunc setter, int flags)
        : PropDef(key, Object::NULL_OBJECT(), asAny, flags),
          getterFunc(getter),
          setterFunc(setter)
    {
    }

    ComponentPropDef& operator=(const ComponentPropDef& rhs) = default;

    Trigger trigger = nullptr;
    LayoutFunc layoutFunc = nullptr;
    DefaultFunc defaultFunc = nullptr;
    GetterFunc getterFunc = nullptr;
    SetterFunc setterFunc = nullptr;
};

/**
 * A collection of component property definitions.
 */
class ComponentPropDefSet : public PropDefSet<PropertyKey, sComponentPropertyBimap, ComponentPropDef>
{
public:
    ComponentPropDefSet() = default;
    ComponentPropDefSet& operator=(const ComponentPropDefSet& rhs) = default;
    ComponentPropDefSet(const ComponentPropDefSet& other, const std::vector<ComponentPropDef>& list)
        : ComponentPropDefSet(other)
    {
        add(list);
    }

    /**
    * Merge a list of property definitions into this set.
    * @param list The property definitions to merge.
    * @return A reference to this PropDefSet.  This allows chaining.
    */
    ComponentPropDefSet& add(const std::vector<ComponentPropDef>& list) {
        addInternal(list);

        for (const ComponentPropDef& m : list) {
            if ((m.flags & kPropStyled) != 0)
                addToMap(mStyled, m);

            if ((m.flags & kPropDynamic) != 0)
                addToMap(mDynamic, m);

            if ((m.flags & kPropNeedsNode) != 0)
                addToMap(mNeedsNode, m);
        }

        return *this;
    }

    /**
     * @return The styled properties
     */
    const PMap& styled() const { return mStyled; }

    /**
     * @return The dynamic properties
     */
    const PMap& dynamic() const { return mDynamic; }

    /**
     * @return The properties that only work when we're attached to a parent yoga node
     */
    const PMap& needsNode() const { return mNeedsNode; }

private:
    PMap mStyled;
    PMap mDynamic;
    PMap mNeedsNode;
};

}  // namespace apl

#endif //_APL_COMPONENT_PROP_DEF_H
