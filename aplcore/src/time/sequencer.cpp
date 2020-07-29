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

#include "apl/time/sequencer.h"
#include "apl/utils/log.h"
#include "apl/command/arraycommand.h"
#include "apl/action/delayaction.h"
#include "apl/time/timemanager.h"

namespace apl {

static const bool DEBUG_SEQUENCER = false;

Sequencer::Sequencer(const std::shared_ptr<TimeManager>& timeManager,
                     const std::string& documentVersion)
    : mTerminated(false), mTimeManager(timeManager) {
    mFeatureSupportResources = (documentVersion.compare("1.4") >= 0);
    mFeatureSupportMultiSequencer = mFeatureSupportResources;
}

ActionPtr
Sequencer::executeOnSequencer(const CommandPtr& commandPtr, const std::string& sequencerName)
{
    if (mTerminated || !commandPtr)
        return nullptr;

    // ignore multi sequencer request for early version documents
    const std::string& targetSequencer = mFeatureSupportMultiSequencer
                                             ? sequencerName : MAIN_SEQUENCER_NAME;

    bool isMain = (targetSequencer == MAIN_SEQUENCER_NAME);

    auto it = mSequencers.find(targetSequencer);
    if (it != mSequencers.end() && it->second != nullptr) {
        it->second->terminate();
        mSequencers.erase(targetSequencer);
    }

    ActionPtr action;

    mResetInExecute.erase(targetSequencer);
    action = DelayAction::make(mTimeManager, commandPtr, false);

    if (mResetInExecute.count(targetSequencer)) {
        action->terminate();
        action = nullptr;
    }

    if (action && action->isPending()) {
        mSequencers.emplace(targetSequencer, action);
    }

    if (!isMain) {
        action->then([this, targetSequencer](const ActionPtr &) {
            mSequencers.erase(targetSequencer);
        });
    } else {
        return action;
    }

    return nullptr;
}

void
Sequencer::executeFast(const CommandPtr& commandPtr)
{
    ActionPtr ptr = commandPtr->execute(mTimeManager, true);
    if (ptr && ptr->isPending()) {
        mOneShotSet.emplace(ptr);
        ptr->then([this](const ActionPtr& ptr) {
            mOneShotSet.erase(ptr);
        });
    }
}

ActionPtr
Sequencer::execute(const CommandPtr& commandPtr, bool fastMode)
{
    if (mTerminated || !commandPtr)
        return nullptr;

    auto sequencerName = commandPtr->sequencer();

    if (sequencerName.empty())
        sequencerName = MAIN_SEQUENCER_NAME;
    else
        fastMode = false;

    if (fastMode) {
        executeFast(commandPtr);
        return nullptr;
    }
    else
        return executeOnSequencer(commandPtr, sequencerName);
}

ActionPtr
Sequencer::executeCommands(const Object& commands,
                           const ContextPtr& context,
                           const CoreComponentPtr& baseComponent,
                           bool fastMode)
{
    if (!commands.isArray()) {
        LOG(LogLevel::ERROR) << "executeCommands: invalid command list";
        return nullptr;
    }

    if (commands.empty())
        return nullptr;

    if (!context->has("event") && !fastMode)
        LOG(LogLevel::WARN) << "missing event in context";

    Properties props;
    auto commandPtr = ArrayCommand::create(context, commands, baseComponent, props, "");
    return execute(commandPtr, fastMode);
}

void
Sequencer::terminate()
{
    if (DEBUG_SEQUENCER) {
        LOG(LogLevel::DEBUG) << "Sequencer terminate";

        for (auto t : mSequencers)
            LOG(LogLevel::DEBUG) << "Thread: " << t.first;

        LOG(LogLevel::DEBUG) << "OneShots: " << mOneShotSet.size();
    }

    for (auto& sequencer : mSequencers) {
        mResetInExecute.emplace(sequencer.first);
        sequencer.second->terminate();
    }

    mSequencers.clear();
    mOneShotSet.clear();
    mResources.clear();
    mResetInExecute.clear();
    mTerminated = true;
}

void
Sequencer::reset()
{
    LOG_IF(DEBUG_SEQUENCER) << "reset start";
    mResetInExecute.emplace(MAIN_SEQUENCER_NAME);
    auto sequencerIt = mSequencers.find(MAIN_SEQUENCER_NAME);
    if (sequencerIt != mSequencers.end()) {
        sequencerIt->second->terminate();
        mSequencers.erase(MAIN_SEQUENCER_NAME);
        mResetInExecute.erase(MAIN_SEQUENCER_NAME);
    }
}

void
Sequencer::claimResource(const ExecutionResource& resource, const ActionPtr& action)
{
    releaseResource(resource);
    mResources.emplace(resource, action);
}

void
Sequencer::releaseResource(const ExecutionResource& resource)
{
    auto rIt = mResources.find(resource);
    if (rIt != mResources.end()) {
        auto holdingAction = rIt->second;

        if (mFeatureSupportResources) {
            holdingAction->terminate();
        }

        releaseRelatedResources(holdingAction);
    }
}

void
Sequencer::releaseRelatedResources(const ActionPtr& action)
{
    for (auto it = mResources.cbegin(); it != mResources.cend(); )
    {
        if (it->second == action)
            it = mResources.erase(it);
        else
            ++it;
    }
}

int
Sequencer::isEmpty(const std::string& sequencerName) const {

    auto it = mSequencers.find(sequencerName);
    return !(it != mSequencers.end() && it->second != nullptr);
}

} // namespace apl
