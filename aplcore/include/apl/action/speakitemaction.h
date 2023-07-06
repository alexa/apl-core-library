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

#ifndef _APL_SPEAK_ITEM_ACTION_H
#define _APL_SPEAK_ITEM_ACTION_H

#include "apl/apl_config.h"
#include "apl/action/resourceholdingaction.h"
#include "apl/audio/speechmark.h"
#include "apl/primitives/range.h"

namespace apl {

class CoreCommand;
class CoreComponent;
class ScrollToAction;
class SpeakItemActionPrivate;

/**
 * Lightweight action that handles speaking.
 *
 * When no AudioPlayerFactory is installed in RootConfig, this action will send out an
 * EventType::kEventTypePreroll, move the appropriate component into view, and send an
 * EventType::kEventTypeSpeak to speak the item.  Line-by-line karaoke highlighting is
 * handled completely by the view host.
 *
 * When an AudioPlayerFactory is installed the action creates a local AudioPlayer, sets
 * the track, moves the appropriate component into view, and plays audio through the
 * AudioPlayer. Line-by-line karaoke highlighting is handled by new set of events:
 * EventType::kEventTypeRequestLineBounds and EventType::kEventTypeLineHighlight or
 * by the scene graph directly
 */
class SpeakItemAction : public ResourceHoldingAction {
public:
    static std::shared_ptr<SpeakItemAction> make(const TimersPtr& timers,
                                                 const std::shared_ptr<CoreCommand>& command,
                                                 const CoreComponentPtr& target=nullptr);

    SpeakItemAction(const TimersPtr& timers,
                    const std::shared_ptr<CoreCommand>& command,
                    const CoreComponentPtr& target);

    void freeze() override;
    bool rehydrate(const CoreDocumentContext& context) override;

private:
    void start();
    void scroll(const std::shared_ptr<ScrollToAction>& action);
    void advance();

private:
    friend class SpeakListAction;

    std::shared_ptr<CoreCommand> mCommand;
    CoreComponentPtr mTarget;
    ActionPtr mCurrentAction;
    std::string mSource;  // The URL of the audio file to play

    friend class SpeakItemActionPrivate;
#ifdef SCENEGRAPH
    friend class SpeakItemActionPrivateSceneGraph;
#endif // SCENEGRAPH
    friend class SpeakItemActionPrivateEvents;
    std::unique_ptr<SpeakItemActionPrivate> mPrivate;
};

}  // namespace apl

#endif //_APL_SPEAK_ITEM_ACTION_H
