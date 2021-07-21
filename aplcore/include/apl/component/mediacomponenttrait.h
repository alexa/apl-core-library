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

#ifndef _APL_MEDIA_COMPONENT_TRAIT_H
#define _APL_MEDIA_COMPONENT_TRAIT_H

#include "apl/component/componenttrait.h"
#include "apl/engine/event.h"

namespace apl {

/**
 * Trait for a component that contains any media sources that should be loaded.
 */
class MediaComponentTrait : public ComponentTrait {
public:
    /**
     * Should be called from the media components own postProcessLayoutChanges.
     */
    void postProcessLayoutChanges();

    /// Internal media fetching utilities
    void resetMediaFetchState();
    void ensureMediaRequested();

    /**
     * @return The list of media properties to add to the component.
     */
    static const std::vector<ComponentPropDef>& propDefList();

protected:
    /**
     * @return vector of source URI's required by component.  Note that order matters.
     */
    virtual std::vector<std::string> getSources() = 0;

    /**
     * Override this method in your subclass if you need a callback when a pending media object is returned.
     * This will not be called if the media object was not pending.  The override must call the superclass method.
     * @param object The ready or failed media object
     */
    virtual void pendingMediaReturned(const MediaObjectPtr& object);

    /**
     * Override this method in your subclass.
     * @return The type of media used by this component.
     */
    virtual EventMediaType mediaType() const = 0;

private:
    void updateMediaState();

protected:
    std::vector<MediaObjectPtr> mMediaObjects;
};

} // namespace apl

#endif //_APL_MEDIA_COMPONENT_TRAIT_H
