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

#ifndef _APL_VIDEO_COMPONENT_H
#define _APL_VIDEO_COMPONENT_H

#include "apl/primitives/mediastate.h"
#include "mediacomponenttrait.h"

namespace apl {


class VideoComponent : public CoreComponent,
                       public MediaComponentTrait {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);
    VideoComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypeVideo; };
    void updateMediaState(const MediaState& state, bool fromEvent) override;

    bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) override;

    std::string getCurrentUrl() const;

    void postProcessLayoutChanges() override;

protected:
    const EventPropertyMap & eventPropertyMap() const override;
    const ComponentPropDefSet& propDefSet() const override;
    void addEventProperties(ObjectMap &event) const override;
    std::string getVisualContextType() const override;
    void assignProperties(const ComponentPropDefSet &propDefSet) override;

    /// Media component trait overrides
    std::vector<std::string> getSources() override;
    EventMediaType mediaType() const override { return kEventMediaTypeVideo; }
    void pendingMediaReturned(const MediaObjectPtr& source) override;

    // Component trait overrides
    CoreComponentPtr getComponent() override { return shared_from_corecomponent(); }

private:
    void saveMediaState(const MediaState& state);
};


} // namespace apl

#endif //_APL_VIDEO_COMPONENT_H
