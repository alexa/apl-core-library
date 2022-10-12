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

#include <algorithm>
#include <set>

#include "apl/scenegraph/scenegraph.h"
#include "apl/scenegraph/layer.h"
#include "apl/scenegraph/node.h"

namespace apl {
namespace sg {

void
SceneGraphUpdates::clear()
{
    for (const auto& m : mChanged)
        m->clearFlags();
    mChanged.clear();

    for (const auto& m : mCreated)
        m->clearFlags();
    mCreated.clear();
}

void
SceneGraphUpdates::changed(const LayerPtr& layer)
{
    if (!layer->anyFlagSet())
        return;

    // Don't add it to the changed set if it is in the created set
    auto it = mCreated.find(layer);
    if (it == mCreated.end())
        mChanged.emplace(layer);
}

void
SceneGraphUpdates::changed(Layer *layer)
{
    assert(layer);
    changed(layer->shared_from_this());
}

void
SceneGraphUpdates::created(const LayerPtr& layer)
{
    // Remove it from the
    auto it = mChanged.find(layer);
    if (it != mChanged.end())
        mChanged.erase(it);

    mCreated.emplace(layer);
}

void
SceneGraphUpdates::created(Layer *layer)
{
    assert(layer);
    created(layer->shared_from_this());
}

void SceneGraphUpdates::mapChanged(std::function<void(const LayerPtr&)> func)
{
    std::for_each(mChanged.begin(), mChanged.end(), std::move(func));
}

void SceneGraphUpdates::mapCreated(std::function<void(const LayerPtr&)> func)
{
    std::for_each(mCreated.begin(), mCreated.end(), std::move(func));
}



} // namespace sg
} // namespace apl