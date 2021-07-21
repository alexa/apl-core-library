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

#ifndef _APL_SEQUENCER_H
#define _APL_SEQUENCER_H

#include "apl/utils/counter.h"
#include "apl/action/action.h"
#include "apl/engine/context.h"
#include "apl/command/command.h"
#include "apl/time/executionresource.h"
#include "apl/time/executionresourceholder.h"

namespace apl {

class TimeManager;

static const std::string MAIN_SEQUENCER_NAME = "MAIN";

class Sequencer : public Counter<Sequencer> {
public:
    Sequencer(const std::shared_ptr<TimeManager>& timeManager, const std::string& documentVersion);

    /**
     * Execute a single command as the master action.  If an action is in progress on main sequencer, it will
     * be terminated and replace with this command sequence.
     *
     * @param commandPtr The command to execute
     * @param fastMode True if in fast mode
     * @return An action pointer that will resolve when the command finishes. A nullptr
     *         will be return in fast mode.
     */
    ActionPtr execute(const CommandPtr& commandPtr, bool fastMode);

    /**
     * Convenience routine that takes an array object of commands and a data-binding context,
     * inflates an ArrayCommand, and then executes it.
     *
     * @param commands An array of commands to execute.
     * @param context The data-binding context.
     * @param baseComponent The base component that these commands execute from.
     * @param fastMode True if the commands should run in fast mode.
     * @return The action pointer of the command or nullptr if there is nothing to execute.
     *         A nullptr will be returned in fast mode.
     */
    ActionPtr executeCommands(const Object& commands,
                              const ContextPtr& context,
                              const CoreComponentPtr& baseComponent,
                              bool fastMode);

    /**
     * Execute command on specific sequencer. If other action is in progress on requested sequencer,
     * it will be terminated.
     *
     * @param commandPtr The command to execute
     * @param sequencerName Name of the sequencer to use.
     * @return An action pointer that will resolve when the command finishes.
     */
    ActionPtr executeOnSequencer(const CommandPtr& commandPtr, const std::string& sequencerName);

    /**
     * Attach action to existing or new sequencer. Requests to attach to main sequencer will be ignored.
     * @param actionPtr action to attach.
     * @param sequencerName target sequencer name.
     */
    void attachToSequencer(const ActionPtr& actionPtr, const std::string& sequencerName);

    /**
     * Terminate any running sequencer. Requests to terminate main will be ignored, use Sequencer::terminate() instead.
     * @param sequencerName target sequencer name.
     */
    void terminateSequencer(const std::string& sequencerName);

    /**
     * Check if sequencer running. Main sequencer always considered running.
     * @param sequencerName target sequencer name.
     * @return true if something running on a sequences, false otherwise.
     */
    bool isRunning(const std::string& sequencerName);

    /**
     * Terminate and clear out the sequencer.  After calling this, no more commands will be accepted.
     */
    void terminate();

    /**
     * Reset the sequencer to clear out any currently executing master command.
     */
    void reset();

    /**
     * Check to see if a specific sequencer is empty.
     * @param sequencerName The sequencer to check.
     * @return true when the sequencer is empty or does not exist.
     */
    bool empty(const std::string& sequencerName) const;

    /**
     * Claim a system display resource (such as scrolling in a scroll view) and associate an action with that
     * resource.  If the resource is claimed by someone else the action is terminated.  Note that a single action
     * may be used to claim multiple resources.
     * @param resource The display resource to claim
     * @param action The action associated with that resource
     */
    void claimResource(const ExecutionResource& resource, const ActionPtr& action);

    /**
     * Claim a system display resource (such as scrolling in a scroll view) and associate a simple function callback.
     * If the resource is claimed by someone else, the callback is invoked.
     * @param resource The display resource to claim
     * @param holder A shared object that is holding the resource.
     */
    void claimResource(const ExecutionResource& resource, const ExecutionResourceHolderPtr& holder);

    /**
     * Release any claims on a system display resource.  Any existing holders of this resource will be
     * notified by action or callback.
     * @param resource The resource to release
     */
    void releaseResource(const ExecutionResource& resource);

    /**
     * Release all claimed resources associated with this action.
     * @param action The action
     */
    void releaseRelatedResources(const ActionPtr& action);

    /**
     * Release all claimed resources associated with this holder
     * @param holder
     */
    void releaseRelatedResources(const ExecutionResourceHolderPtr& holder);

private:
    void executeFast(const CommandPtr& commandPtr);

    bool mTerminated;
    const std::shared_ptr<TimeManager> mTimeManager;
    std::set<ActionPtr> mOneShotSet;
    std::set<std::string> mResetInExecute;
    std::map<std::string, ActionPtr> mSequencers;
    std::map<ExecutionResource, ActionPtr> mResourcesByAction;
    std::map<ExecutionResource, ExecutionResourceHolderPtr> mResourcesByHolder;
    bool mFeatureSupportResources = true;
    bool mFeatureSupportMultiSequencer = true;
};

} // namespace apl

#endif //_APL_SEQUENCER_H
