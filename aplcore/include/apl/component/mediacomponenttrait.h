/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "componenttrait.h"

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

    /**
     * Notify component about media loaded.
     * @param source source that was loaded.
     */
    virtual void pendingMediaLoaded(const std::string& source, int stillPending);

    /**
     * Notify component about media that failed to be loaded.
     * @param source source that failed to load.
     */
    virtual void pendingMediaFailed(const std::string& source);

    /// Internal media fetching utilities
    void resetMediaFetchState();
    void ensureMediaRequested();

    /**
     * @return The list of media properties to add to the component.
     */
    static const std::vector<ComponentPropDef>& propDefList();

protected:


    /**
     * @return set of source URI's required by component.
     */
    virtual std::set<std::string> getSources() = 0;
};

} // namespace apl

#endif //_APL_MEDIA_COMPONENT_TRAIT_H
