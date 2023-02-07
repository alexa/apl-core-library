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

#ifndef _APL_SCENE_GRAPH_UPDATES_H
#define _APL_SCENE_GRAPH_UPDATES_H

#include <functional>
#include <memory>
#include <set>

#include "apl/scenegraph/common.h"

namespace apl {
namespace sg {

/**
 * TODO: In the future I'd like to remove the "created" tracker.
 * This is a challenge because when you are updating a dirty component hierarchy you
 * can end up creating some new children and later updating them based on the dirty component
 * hierarchy (for example, a pager will create the new layer being paged in, then call update
 * on that newly created page because the _component_ hierarchy had the new page marked as dirty).
 */
class SceneGraphUpdates {
public:
    SceneGraphUpdates() = default;
    ~SceneGraphUpdates() = default;

    void clear();
    bool empty() { return mChanged.empty() && mCreated.empty(); }

    void changed(const LayerPtr& layer);
    void created(const LayerPtr& layer);
    void resize(const LayerPtr& layer);

    void mapChanged(const std::function<void(const LayerPtr&)>& func);

    void fixCreatedFlags();
    void processResize();

private:
    std::set<LayerPtr> mChanged;
    std::set<LayerPtr> mCreated;
    std::set<LayerPtr> mResize;
};

} // namespace sg
} // namespace apl

#endif // _APL_SCENE_GRAPH_UPDATES_H
