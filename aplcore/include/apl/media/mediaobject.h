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

#ifndef _APL_MEDIA_OBJECT_H
#define _APL_MEDIA_OBJECT_H

#include <functional>
#include <string>

#include "apl/common.h"
#include "apl/engine/event.h"
#include "apl/primitives/size.h"
#include "apl/utils/noncopyable.h"

namespace apl {

using MediaObjectCallback = std::function<void(const MediaObjectPtr&)>;

/**
 * An abstract base class for a media "blob" loaded from a view host which tracks the loading state
 * of the media.  The MediaManager returns a pointer to a subclass of MediaObject when media is
 * requested.  The media objects's internal state tracks if the load is pending, completed, or has failed.
 * Callbacks may be attached to pending media objects to be informed when the load completes or fails.
 *
 * The MediaManager normally caches media objects by URL; requesting the same URL may return the same
 * media object, but that behavior is not guaranteed.
 *
 * Releasing the shared pointer to the media object may also release the downloaded content.  Components
 * should hold onto the media object pointers as long as they are needed to render on-screen content.
 */
class MediaObject : NonCopyable,
                    public Counter<MediaObject> {
public:
    enum State {
        kPending,  /// The media object has not yet loaded.
        kReady,    /// The media object is loaded and may be displayed.
        kError     /// The media object failed to load and may not be displayed.
    };

    virtual ~MediaObject() = default;

    /**
     * @return The URL used to load the media object
     */
    virtual std::string url() const = 0;

    /**
     * @return The current state of the media object.
     */
    virtual State state() const = 0;

    /**
     * @return The type of the media object.
     */
    virtual EventMediaType type() const = 0;

    /**
     * @return The size of the media object.  Bitmap images and videos use pixels.  Vector graphics
     *         return the unit-less size of the vector graphic.
     */
    virtual Size size() const = 0;

    /**
     * Add a callback to be executed when the MediaObject changes state from kPending to
     * either kReady or kError.  Multiple callbacks may be added, but the order in which
     * the callbacks are executed is not guaranteed.  Callbacks will be invoked on the
     * main core engine thread.
     *
     * The callback will not be added if the MediaObject is not in the kPending state.
     *
     * @param callback The callback to be executed.
     * @return True if the callback has been added (the MediaObject is in the kPending state).
     */
    virtual bool addCallback(MediaObjectCallback callback) = 0;
};


} // namespace apl

#endif // _APL_MEDIA_OBJECT_H
