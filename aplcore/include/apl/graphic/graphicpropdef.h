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

#ifndef _APL_GRAPHIC_PROP_DEF_H
#define _APL_GRAPHIC_PROP_DEF_H

#include "apl/engine/propdef.h"

namespace apl {

using Trigger = void (*)(GraphicElement &);
using DefaultFunc = Object (*)(GraphicElement&, const RootConfig&);

class GraphicPropDef : public PropDef<GraphicPropertyKey, sGraphicPropertyBimap> {
public:
    GraphicPropDef(GraphicPropertyKey key, int defvalue, Bimap<int, std::string> &map, int flags)
            : PropDef(key, defvalue, map, flags) {}

    GraphicPropDef(GraphicPropertyKey key, const Object &defvalue, BindingFunction func, int flags)
            : GraphicPropDef(key, defvalue, std::move(func), flags, nullptr, nullptr) {}

    GraphicPropDef(GraphicPropertyKey key, const Object &defvalue, BindingFunction func, int flags, Trigger trigger)
            : GraphicPropDef(key, defvalue, std::move(func), flags, trigger, nullptr) {}

    GraphicPropDef(GraphicPropertyKey key, const Object &defvalue, BindingFunction func, int flags, DefaultFunc defaultFunc)
            : GraphicPropDef(key, defvalue, std::move(func), flags, nullptr, defaultFunc) {}

    GraphicPropDef(
            GraphicPropertyKey key,
            const Object &defvalue,
            BindingFunction func,
            int flags,
            Trigger trigger,
            DefaultFunc defaultFunc)
            : PropDef(key, defvalue, std::move(func), flags),
              trigger(trigger),
              defaultFunc(defaultFunc) {}

    Trigger trigger = nullptr;
    DefaultFunc defaultFunc = nullptr;
};

class GraphicPropDefSet : public PropDefSet<GraphicPropertyKey, sGraphicPropertyBimap, GraphicPropDef> {
public:
    GraphicPropDefSet &add(const std::vector <GraphicPropDef> &list) {
        addInternal(list);
        return *this;
    }
};

} // namespace apl

#endif //_APL_GRAPHIC_PROP_DEF_H
