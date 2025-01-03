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

#include "apl/action/speakitemaction.h"

#include "apl/action/scrolltoaction.h"
#include "apl/audio/audioplayerfactory.h"
#include "apl/command/commandproperties.h"
#include "apl/command/corecommand.h"
#include "apl/component/textcomponent.h"
#include "apl/primitives/styledtext.h"
#include "apl/scenegraph/textlayout.h"
#include "apl/scenegraph/textmeasurement.h"
#include "apl/time/sequencer.h"
#include "apl/utils/actiondata.h"
#include "apl/utils/make_unique.h"
#include "apl/utils/principal_ptr.h"
#include "apl/utils/session.h"
#include "apl/media/mediatrack.h"

static const bool DEBUG_SPEAK_ITEM = false;

namespace apl {

// TODO: This is a copy from rootcontext.cpp, where it is used by view hosts to cause elements
//  to scroll into view.  We can remove this funtionality from the viewhost when it's migrated.
static const std::string SCROLL_TO_RECT_SEQUENCER = "__SCROLL_TO_RECT_SEQUENCE";

/**
 * This class is used when an AudioPlayerFactory has been installed in RootConfig.
 * It handles scrolling, displaying highlighted lines, and playing of the audio.
 *
 * Calling the start() method on this class kicks off the series of actions to handle all playback.
 * No methods from the original class are used, but the instance variables from the original class
 * are used, which requires us to pass a SpeakItemAction reference to each method.
 *
 */
class SpeakItemActionPrivate {
public:
    void terminate() {
        if (mScrollAction) {
            mScrollAction->terminate();
            mScrollAction = nullptr;
        }

        if (mSpeakAction) {
            mSpeakAction->terminate();
            mSpeakAction = nullptr;
        }

        if (mDwellAction) {
            mDwellAction->terminate();
            mDwellAction = nullptr;
        }

        mAudioPlayer = nullptr;
    }

    static void offsetBounds(TextComponent* textComponent, Rect& bounds) {
        auto innerBounds = textComponent->getCalculated(kPropertyInnerBounds).get<Rect>();
        bounds.offset(innerBounds.getX(), innerBounds.getY());
    }

    void start(SpeakItemAction& action) {
        preroll(action);

        // Calculate the bounds to scroll into view.
        Rect bounds; // Empty bounds by default

        // Line-by-line highlighting only occurs when we've stored the current text
        auto target = TextComponent::cast(action.mTarget);
        if (target && !mText.empty()) {
            // Check for a text layout
            auto layout = target->getTextLayout();
            if (layout && layout->getLineCount() > 0)
                bounds = layout->getBoundingBoxForLines(Range{0, 0});
            offsetBounds(target.get(), bounds);
        }
        initialScroll(action, bounds);
    }

    void preroll(SpeakItemAction& action) {
        auto context = action.mCommand->context();

        // Create a MediaTrack from Speech Property
        MediaTrack track = createMediaTrack(action.mTarget->getCalculated(kPropertySpeech), context);
        if(!track.valid()){
            CONSOLE(context).log("Audio source missing in playback");
            return;
        }
        action.mSource = track.url;
        LOG_IF(DEBUG_SPEAK_ITEM) << "source: " << action.mSource
                                 << ", lineMode: " << !mText.empty();

        // If we are doing line highlighting, grab a copy of the text in the component
        if (action.mTarget->getType() == kComponentTypeText &&
            action.mCommand->getValue(kCommandPropertyHighlightMode) == kCommandHighlightModeLine) {
            // Stash the raw text in lower-case for word comparisons
            mText = action.mTarget->getRootConfig().getLocaleMethods()->toLowerCase(
                action.mTarget->getCalculated(kPropertyText).get<StyledText>().getText(), "");
        }

        // Create an audio player and queue up the TTS as the track
        const auto& factory = context->getRootConfig().getAudioPlayerFactory();
        std::weak_ptr<SpeakItemAction> weak_ptr(
            std::static_pointer_cast<SpeakItemAction>(action.shared_from_this()));

        if (!action.mSource.empty() && factory && !mSpeakAction) {
            auto audioCallback = [weak_ptr](AudioPlayerEventType eventType,
                                            const AudioState& state) {
                LOG_IF(DEBUG_SPEAK_ITEM) << "eventType: " << eventType
                                         << ", state: " << state.toDebugString();

                if (eventType == kAudioPlayerEventPlay ||
                    eventType == kAudioPlayerEventTimeUpdate) {
                    auto self = weak_ptr.lock();
                    if (!self)
                        return;

                    auto& p = *self->mPrivate;
                    p.updateAudioState(*self, state);
                }
            };

            SpeechMarkCallback speechMarkCallback = nullptr;

            mOnSpeechMark = action.mTarget->getCalculated(kPropertyOnSpeechMark);
            if (!mText.empty() || !mOnSpeechMark.empty())
                speechMarkCallback = [weak_ptr](const std::vector<SpeechMark>& speechMarks) {
                    auto self = weak_ptr.lock();
                    if (!self)
                        return;

                    if (DEBUG_SPEAK_ITEM) {
                        for (auto& sm : speechMarks) {
                            LOG_IF(DEBUG_SPEAK_ITEM) << "SpeechMark("
                                                     << " value=" << sm.value
                                                     << " end=" << sm.end
                                                     << " start=" << sm.start
                                                     << " type=" << sm.type
                                                     << " time=" << sm.time
                                                     << ")";
                        }
                    }

                    auto& p = *self->mPrivate;
                    p.mSpeechMarks.insert(std::end(p.mSpeechMarks), std::begin(speechMarks),
                                          std::end(speechMarks));
                };

            mAudioPlayer = factory->createPlayer(audioCallback, speechMarkCallback);
            context->sequencer().claimResource(kExecutionResourceForegroundAudio,
                                               action.shared_from_this());

            if (mAudioPlayer) {
                mAudioPlayer->setTrack(track);
            }
        }
    }

    void initialScroll(SpeakItemAction& action, const Rect& bounds) {
        LOG_IF(DEBUG_SPEAK_ITEM) << "bounds: " << bounds.toDebugString();
        std::weak_ptr<SpeakItemAction> weak_ptr(
            std::static_pointer_cast<SpeakItemAction>(action.shared_from_this()));

        // Create a scroll action.  We'll start Karaoke and playback AFTER this scroll action terminates
        if (bounds.empty())
            mScrollAction = ScrollToAction::make(action.timers(), action.mCommand, action.mTarget);
        else
            mScrollAction = ScrollToAction::make(action.timers(), action.mCommand, bounds, action.mTarget);

        if (mScrollAction) {
            mScrollAction->then([weak_ptr](const ActionPtr& actionPtr) {
                auto self = weak_ptr.lock();
                if (!self)
                    return;

                // TODO: Scroll takes 1s fixed. Which is unreasonable for SpeakItem.
                //  Need to change that.
                auto& p = *self->mPrivate;
                p.advance(*self);
            });

            // If scroll was killed by conflicting operation - kill whole SpeakItem.
            mScrollAction->addTerminateCallback([weak_ptr](const TimersPtr&) {
                auto self = weak_ptr.lock();
                if (self)
                    self->terminate();
            });
        }
        else {
            advance(action);
        }
    }

    void advance(SpeakItemAction& action)
    {
        // The scrolling action is no longer useful.  It will be reused if we do line-by-line scrolling
        mScrollAction = nullptr;

        std::weak_ptr<SpeakItemAction> weak_ptr(std::static_pointer_cast<SpeakItemAction>(action.shared_from_this()));

        // If you have an audio player, start playback
        if (mAudioPlayer) {
            // Note that we don't need a weak pointer because the start function runs immediately
            mSpeakAction = Action::make(action.timers(), [this](ActionRef ref) { mAudioPlayer->play(ref); });

            if (mSpeakAction) {
                mSpeakAction->then([weak_ptr](const ActionPtr& actionPtr) {
                    auto self = weak_ptr.lock();
                    if (!self)
                        return;

                    auto& p = *self->mPrivate;

                    // Release the audio player; it is no longer needed.
                    p.mAudioPlayer = nullptr;
                    p.mSpeakAction = nullptr;

                    // If we are not waiting on the dwell, we are done
                    if (!p.mDwellAction) {
                        p.clearKaraoke(*self);
                        self->resolve();
                    }
                });

                mSpeakAction->addTerminateCallback([weak_ptr](const TimersPtr&) {
                    auto self = weak_ptr.lock();
                    if (self)
                        self->terminate();  // Kill the entire SpeakItemAction
                });
            }
        }

        // Construct the dwell action
        auto minDwell = action.mCommand->getValue(kCommandPropertyMinimumDwellTime).asInt();
        if (minDwell > 0) {
            mDwellAction = Action::makeDelayed(action.timers(), minDwell);
            mDwellAction->then([weak_ptr](const ActionPtr& actionPtr) {
                auto self = weak_ptr.lock();
                if (!self)
                    return;

                auto& p = *self->mPrivate;
                p.mDwellAction = nullptr;

                // If we are not waiting on the speech, we are done
                if (!p.mSpeakAction) {
                    p.clearKaraoke(*self);
                    self->resolve();
                }
            });
            // The dwell does not need a terminate action
        }

        // If we have either a dwell OR speech, we set Karaoke state.  Otherwise, resolve immediately.
        if (mDwellAction || mSpeakAction)
            action.mTarget->setState(kStateKaraoke, true);
        else
            action.resolve();
    }

    /**
     * Note: This algorithm is not exactly the same as used by Android.  It needs to be sanity-checked
     * to make sure we don't introduce something that performs worse than the current implementation.
     *
     * This gets called on any time update from the audio player
     *
     * The current algorithm looks at word marks and searches forward in the text (lower-cased)
     * to find the word.  If the word has hyphens, we split on the hyphens.
     *
     * Some word marks contain tags - such as "<break time=\"250ms\"/>"
     * To avoid matching these, we skip word marks that start with the '<' character.
     */
    void updateMarks(SpeakItemAction& action, unsigned long currentTime)
    {
        if (mSpeechMarks.empty())
            return;

        const auto markLen = mSpeechMarks.size();
        const auto textLen = mText.empty() ? SIZE_MAX : mText.size();

        LOG_IF(DEBUG_SPEAK_ITEM) << "currentTime=" << currentTime
                                 << " markLen=" << markLen
                                 << " textLen=" << textLen
                                 << " mNextMark=" << mNextMark;

        for (; mNextMark < markLen; mNextMark++) {
            const auto& mark = mSpeechMarks.at(mNextMark);

            LOG_IF(DEBUG_SPEAK_ITEM) << " mNextMark=" << mNextMark
                                     << " value=" << mark.value
                                     << " end=" << mark.end
                                     << " start=" << mark.start
                                     << " type=" << mark.type
                                     << " time=" << mark.time;

            if (mark.time > currentTime)
                return;

            if (!mOnSpeechMark.empty()) {
                auto speechMarkOpt = std::make_shared<std::map<std::string, Object>>();
                speechMarkOpt->emplace("markType", sSpeechMarkTypeMap.at(mark.type));
                speechMarkOpt->emplace("markTime", mark.time);
                speechMarkOpt->emplace("markValue", mark.value);

                action.mContext->sequencer().executeCommands(
                    mOnSpeechMark,
                    action.mTarget->createEventContext("SpeechMark", speechMarkOpt),
                    action.mTarget, true);
            }

            if (mark.type != kSpeechMarkWord || mark.value.empty() || mark.value.at(0) == '<')
                continue;

            if (mText.empty() || mTextPosition >= textLen)
                continue;

            // Lowercase the word and split hyphenated words into separate elements
            auto value = action.mTarget->getRootConfig().getLocaleMethods()->toLowerCase(mark.value, "");
            std::string::size_type pos = 0;
            for ( std::string::size_type offset = 0 ; offset != std::string::npos ; pos = offset + 1) {
                offset = value.find('-', pos);
                auto count = offset == std::string::npos ? value.size() - pos : offset - pos;
                if (count == 0)
                    continue;

                auto sourceOffset = mText.find( value.c_str(), mTextPosition, count);

                if (sourceOffset != std::string::npos) {
                    mTextPosition = sourceOffset + count;
                    highlight(action,
                        Range{static_cast<int>(sourceOffset), static_cast<int>(mTextPosition - 1)});
                    break; // This mark has matched, so continue to the next speech mark
                }
            }
        }
    }

    void freeze() {
        mScrollAction = nullptr;
        mDwellAction = nullptr;
    }

    void rehydrate(SpeakItemAction& action) {
        // Don't need it for line by line - time update will scroll appropriately
        if (!mText.empty()) return;

        if (!mSpeakAction) {
            // Restart
            start(action);
        } else {
            auto align = static_cast<CommandScrollAlign>(action.mCommand->getValue(kCommandPropertyAlign).getInteger());
            mScrollAction = ScrollToAction::make(
                action.timers(),
                align,
                Rect(),
                action.mCommand->context(),
                false,
                action.mTarget,
                0,
                false);
            action.mContext->sequencer().claimResource(kExecutionResourceForegroundAudio,
                                                       action.shared_from_this());
        }
    }

    void clearKaraoke(SpeakItemAction& action) {
        action.mTarget->setState(kStateKaraoke, false);
        auto target = TextComponent::cast(action.mTarget);
        if (target && !mText.empty()) {
            target->clearKaraokeLine();

            // Just send highlight event for non-SG impls. It's fire and forget.
            EventBag bag;
            bag.emplace(kEventPropertyRangeStart, -1);
            bag.emplace(kEventPropertyRangeEnd, -1);
            action.mContext->pushEvent(Event(kEventTypeLineHighlight, std::move(bag), action.mTarget));
        }
    }

    void highlight(SpeakItemAction& action, Range byteRange) {
        if (mText.empty()) return;

        auto *target = TextComponent::cast(action.mTarget).get();
        auto changed = target->setKaraokeLine(byteRange);
        if (!changed)
            return;

        EventBag bag;
        bag.emplace(kEventPropertyRangeStart, byteRange.lowerBound());
        bag.emplace(kEventPropertyRangeEnd, byteRange.upperBound());
        action.mContext->pushEvent(Event(kEventTypeLineHighlight, std::move(bag), action.mTarget));

        LOG_IF(DEBUG_SPEAK_ITEM) << "highlight: " << byteRange.toDebugString();

        auto bounds = target->getKaraokeBounds();
        if (bounds.empty())
            return;
        offsetBounds(target, bounds);
        LOG_IF(DEBUG_SPEAK_ITEM) << "scroll: " << bounds.toDebugString();

        mLastLineBounds = bounds;

        // Turns out that there is a SCROLL_TO_RECT_SEQUENCER (see rootcontext.cpp)
        mScrollAction = ScrollToAction::make(action.timers(), action.mCommand, bounds, action.mTarget);
        if (mScrollAction && mScrollAction->isPending())
            action.mContext->sequencer().attachToSequencer(mScrollAction, SCROLL_TO_RECT_SEQUENCER);
    }

    void updateAudioState(SpeakItemAction& action, const AudioState& state) {
        if (mLastProcessedTime != state.getCurrentTime()) {
            if (mLastLineBounds.empty() && mSpeechMarks.empty()) {
                // Explicitly highlight first line. Speech marks may arrive after play
                // started, and we will miss out otherwise.
                highlight(action, {0, 0});
            }
            else {
                updateMarks(action, state.getCurrentTime());
            }
        }
        mLastProcessedTime = state.getCurrentTime();
    }

protected:
    ActionPtr mScrollAction;
    ActionPtr mSpeakAction;
    ActionPtr mDwellAction;

    principal_ptr<AudioPlayer, &AudioPlayer::release> mAudioPlayer;
    std::vector<SpeechMark> mSpeechMarks;
    std::string mText;
    std::string::size_type mTextPosition = 0;
    std::vector<SpeechMark>::size_type mNextMark = 0;
    Object mOnSpeechMark;

    Rect mLastLineBounds; // Memoize currently highlighted bounds, can't dedupe in other way for now.
    int mLastProcessedTime = -1;
};

/*********************** SpeakItemAction Implementation *********************/

SpeakItemAction::SpeakItemAction(const TimersPtr& timers, const std::shared_ptr<CoreCommand>& command,
        const CoreComponentPtr& target)
        : ResourceHoldingAction(timers, command->context()),
          mCommand(command),
          mTarget(target)
{
    const auto& rootConfig = mCommand->context()->getRootConfig();
    assert(rootConfig.getAudioPlayerFactory());

    mPrivate = std::make_unique<SpeakItemActionPrivate>();
    addTerminateCallback([this](const TimersPtr&) {
        mPrivate->terminate();
        mPrivate->clearKaraoke(*this);
    });
}

std::shared_ptr<SpeakItemAction>
SpeakItemAction::make(const TimersPtr& timers,
                      const std::shared_ptr<CoreCommand>& command,
                      const CoreComponentPtr& target)
{
    auto t = target ? target : command->target();
    if (!t)
        return nullptr;

    auto ptr = std::make_shared<SpeakItemAction>(timers, command, t);
    assert(ptr->mPrivate);
    ptr->mPrivate->start(*ptr);
    return ptr;
}

void
SpeakItemAction::freeze()
{
    if (!mPrivate) return;

    mTarget = nullptr;

    if (mCurrentAction) {
        mCurrentAction->freeze();
    }

    if (mCommand) {
        mCommand->freeze();
    }

    mPrivate->freeze();

    ResourceHoldingAction::freeze();
}

bool
SpeakItemAction::rehydrate(const CoreDocumentContext& context)
{
    if (!mPrivate) return false;

    if (!ResourceHoldingAction::rehydrate(context)) return false;

    if (mCommand) {
        if (!mCommand->rehydrate(context)) return false;
    }

    if (mCommand && !mTarget) {
        mTarget = mCommand->target();
    }

    if (!mTarget) return false;

    if (mCurrentAction && !mCurrentAction->rehydrate(context)) return false;

    mTarget->setState(kStateKaraoke, true);

    mPrivate->rehydrate(*this);

    return true;
}

ActionData
SpeakItemAction::getActionData() {
    return ActionData().target(mTarget).actionHint("Speaking");
}

} // namespace apl
