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
Sequencer::attachToSequencer(const ActionPtr& actionPtr, const std::string& sequencerName)
{
    if (sequencerName == MAIN_SEQUENCER_NAME) return;

    auto it = mSequencers.find(sequencerName);
    if (it != mSequencers.end() && it->second != nullptr) {
        it->second->terminate();
        mSequencers.erase(sequencerName);
    }

    if (actionPtr && actionPtr->isPending()) {
        mSequencers.emplace(sequencerName, actionPtr);
    }

    actionPtr->then([this, sequencerName](const ActionPtr &) {
      mSequencers.erase(sequencerName);
    });
}

void
Sequencer::terminateSequencer(const std::string& sequencerName)
{
    if (sequencerName == MAIN_SEQUENCER_NAME) return;

    auto it = mSequencers.find(sequencerName);
    if (it != mSequencers.end() && it->second != nullptr) {
        it->second->terminate();
        mSequencers.erase(sequencerName);
    }
}

bool
Sequencer::isRunning(const std::string& sequencerName)
{
    return (sequencerName == MAIN_SEQUENCER_NAME) || mSequencers.count(sequencerName);
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
    if (mTerminated)
        return nullptr;

    if (!commands.isArray()) {
        LOG(LogLevel::kError) << "executeCommands: invalid command list";
        return nullptr;
    }

    if (commands.empty())
        return nullptr;

    if (!context->has("event") && !fastMode)
        LOG(LogLevel::kWarn) << "missing event in context";

    Properties props;
    auto commandPtr = ArrayCommand::create(context, commands, baseComponent, props, "");
    return execute(commandPtr, fastMode);
}


ActionPtr
Sequencer::executeCommandsOnSequencer(const Object& commands,
                                      const ContextPtr& context,
                                      const CoreComponentPtr& baseComponent,
                                      const std::string& sequencer)
{
    if (mTerminated)
        return nullptr;

    if (!commands.isArray()) {
        LOG(LogLevel::kError) << "executeCommands: invalid command list";
        return nullptr;
    }

    if (commands.empty())
        return nullptr;

    if (!context->has("event"))
        LOG(LogLevel::kWarn) << "missing event in context";

    Properties props;
    auto commandPtr = ArrayCommand::create(context, commands, baseComponent, props, "");
    return executeOnSequencer(commandPtr, sequencer);
}

void
Sequencer::terminate()
{
    if (DEBUG_SEQUENCER) {
        LOG(LogLevel::kDebug) << "Sequencer terminate";

        for (auto t : mSequencers)
            LOG(LogLevel::kDebug) << "Thread: " << t.first;

        LOG(LogLevel::kDebug) << "OneShots: " << mOneShotSet.size();
    }

    for (auto& sequencer : mSequencers) {
        mResetInExecute.emplace(sequencer.first);
        sequencer.second->terminate();
    }

    mSequencers.clear();
    mOneShotSet.clear();
    mResourcesByAction.clear();
    mResourcesByHolder.clear();
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
    if (mTerminated)
        return;

    releaseResource(resource);
    mResourcesByAction.emplace(resource, action);
}

void
Sequencer::claimResource(const ExecutionResource& resource, const ExecutionResourceHolderPtr& holder)
{
    if (mTerminated)
        return;

    releaseResource(resource);
    mResourcesByHolder.emplace(resource, holder);
}

void
Sequencer::releaseResource(const ExecutionResource& resource)
{
    if (mTerminated)
        return;

    auto it = mResourcesByAction.begin();
    while (it != mResourcesByAction.end()) {
        if (!it->second.lock())
            it = mResourcesByAction.erase(it);
        else
            it++;
    }

    it = mResourcesByAction.find(resource);
    if (it != mResourcesByAction.end()) {
        auto actionPtr = it->second.lock();
        if (mFeatureSupportResources)
            actionPtr->terminate();
        releaseRelatedResources(actionPtr);
    }

    auto it2 = mResourcesByHolder.find(resource);
    if (it2 != mResourcesByHolder.end()) {
        if (mFeatureSupportResources)
            it2->second->onResourceLoss();
        releaseRelatedResources(it2->second);
    }
}

void
Sequencer::releaseRelatedResources(const ActionPtr& action)
{
    if (mTerminated)
        return;

    auto it = mResourcesByAction.begin();
    while (it != mResourcesByAction.end()) {
        auto actionPtr = it->second.lock();
        if (!actionPtr || actionPtr == action)
            it = mResourcesByAction.erase(it);
        else
            it++;
    }
}

void
Sequencer::releaseRelatedResources(const ExecutionResourceHolderPtr& holder)
{
    if (mTerminated)
        return;

    auto it = mResourcesByHolder.begin();
    while (it != mResourcesByHolder.end()) {
        if (it->second == holder)
            it = mResourcesByHolder.erase(it);
        else
            it++;
    }
}

bool
Sequencer::empty(const std::string& sequencerName) const
{
    if (mTerminated)
        return true;

    auto it = mSequencers.find(sequencerName);
    return !(it != mSequencers.end() && it->second != nullptr);
}

} // namespace apl
