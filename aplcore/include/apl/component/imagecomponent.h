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

#ifndef _APL_IMAGE_COMPONENT_H
#define _APL_IMAGE_COMPONENT_H

#include "mediacomponenttrait.h"

namespace apl {

class ImageComponent : public CoreComponent,
                       public MediaComponentTrait {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);

    ImageComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypeImage; }

    void postProcessLayoutChanges() override;

    void release() override;

protected:
    const EventPropertyMap& eventPropertyMap() const override;
    const ComponentPropDefSet& propDefSet() const override;

    std::string getVisualContextType() const override;

    /// Media component trait overrides
    std::vector<std::string> getSources() override;
    EventMediaType mediaType() const override { return kEventMediaTypeImage; }
    void onFail(const MediaObjectPtr&) override;
    void onLoad() override;

    // Component trait overrides
    CoreComponentPtr getComponent() override { return shared_from_corecomponent(); }

private:
    bool mOnLoadOnFailReported = false;
};

} // namespace apl

#endif //_APL_IMAGE_COMPONENT_H
