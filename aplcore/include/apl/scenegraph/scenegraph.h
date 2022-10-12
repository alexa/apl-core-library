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

#ifndef _APL_SG_SCENE_GRAPH_H
#define _APL_SG_SCENE_GRAPH_H

#include <rapidjson/document.h>

#include "apl/common.h"
#include "apl/scenegraph/scenegraphupdates.h"
#include "apl/utils/noncopyable.h"

namespace apl {
namespace sg {

class SceneGraphUpdates;

class SceneGraph : public NonCopyable {
public:
    static SceneGraphPtr create() { return std::make_shared<SceneGraph>(); }

    SceneGraph() = default;
    virtual ~SceneGraph() {}

    void setLayer(const LayerPtr& layer) { mTopLayer = layer; }
    LayerPtr getLayer() const { return mTopLayer; }

    SceneGraphUpdates& updates() { return mUpdates; }

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

private:
    LayerPtr mTopLayer;
    SceneGraphUpdates mUpdates;
};

} // namespace sg
} // namespace apl

#endif // _APL_SG_SCENE_GRAPH_H
