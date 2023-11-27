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

#include "apl/component/mediacomponenttrait.h"
#include "apl/media/mediaplayerfactory.h"
#include "apl/primitives/mediastate.h"

namespace apl {

class VideoComponent : public CoreComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);
    VideoComponent(const ContextPtr& context, Properties&& properties, const Path& path);
    virtual ~VideoComponent() noexcept;

    ComponentType getType() const override { return kComponentTypeVideo; }

    void releaseSelf() override;
    bool remove() override;

    void updateMediaState(const MediaState& state, bool fromEvent) override;
    bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) override;

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override;

    std::string getCurrentUrl() const;

    MediaPlayerPtr getMediaPlayer() const override { return mMediaPlayer; }

    /**
     * Detach media player from the component.
     */
    void detachPlayer();

    /**
     * Attach media player tho the component. Replaces any existing one.
     * @param player MediaPlayer.
     */
    void attachPlayer(const MediaPlayerPtr& player);

    /**
     * @param component Pointer to cast.
     * @return Casted pointer to this type, nullptr if not possible.
     */
    static std::shared_ptr<VideoComponent> cast(const ComponentPtr& component);

protected:
    const EventPropertyMap & eventPropertyMap() const override;
    const ComponentPropDefSet& propDefSet() const override;
    std::string getVisualContextType() const override;
    void assignProperties(const ComponentPropDefSet &propDefSet) override;

#ifdef SCENEGRAPH
    // Common scene graph handling
    sg::LayerPtr constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph) override;
    bool updateSceneGraphInternal(sg::SceneGraphUpdates& sceneGraph) override;
#endif // SCENEGRAPH

private:
    void saveMediaState(const MediaState& state);
    std::shared_ptr<ObjectMap> createDefaultEventProperties();
    std::shared_ptr<ObjectMap> createErrorEventProperties(int errorCode);
    std::shared_ptr<ObjectMap> createReadyEventProperties();
    void playerCallback(MediaPlayerEventType eventType, const MediaState& mediaState);

    MediaPlayerPtr mMediaPlayer;
    const std::string mMediaSequencer;  // Internal sequencer used for onEnd/onPause/onPlay
};


} // namespace apl

#endif //_APL_VIDEO_COMPONENT_H
