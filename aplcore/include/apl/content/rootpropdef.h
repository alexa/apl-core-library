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

#ifndef _APL_ROOT_PROP_DEF_H
#define _APL_ROOT_PROP_DEF_H

#include "apl/engine/propdef.h"
#include "apl/content/rootproperties.h"

namespace apl {

using RootPropDef = PropDef<RootProperty, sRootPropertyBimap>;

class RootPropDefSet : public PropDefSet<RootProperty, sRootPropertyBimap, RootPropDef>
{
public:
    RootPropDefSet() = default;
    RootPropDefSet(const RootPropDefSet& rhs) = default;
    RootPropDefSet(const RootPropDefSet& other, const std::vector<RootPropDef>& list)
        : RootPropDefSet(other)
    {
        add(list);
    }

    RootPropDefSet& add(const std::vector<RootPropDef>& list) {
        addInternal(list);
        return *this;
    }
};

} // namespace apl

#endif //_APL_ROOT_PROP_DEF_H
