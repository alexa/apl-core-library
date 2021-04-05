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

#ifndef _APL_EXECUTION_RESOURCE_HOLDER_H
#define _APL_EXECUTION_RESOURCE_HOLDER_H

#include "apl/time/executionresource.h"
#include "apl/utils/noncopyable.h"

namespace apl {

class Sequencer;

/**
 * An execution resource holder is a convenience object for claiming a resource from the sequencer.  For example,
 * a gesture may use a holder to claim a position resource from the sequencer when the gesture is triggered and
 * to release the resource when the gesture has finished.  If the claimed resource is lost, a custom callback
 * function is invoked to warn that the resource has been taken away.
 */
class ExecutionResourceHolder : public NonCopyable,
                                public std::enable_shared_from_this<ExecutionResourceHolder> {
public:
    /**
     * Create a shared pointer to an execution resource handler
     * @param resourceKey The resource key
     * @param callback The function to invoke if the resource is lost.
     * @param component The component to attach to this resource.
     * @param propKey The property key to attach to this resource, if needed.
     * @return A shared pointer to an execution resource holder
     */
    static std::shared_ptr<ExecutionResourceHolder> create( ExecutionResourceKey resourceKey,
                                                            const CoreComponentPtr& component,
                                                            std::function<void()>&& callback,
                                                            PropertyKey propKey = kPropertyId );

    // Use the ::create method instead
    explicit ExecutionResourceHolder(ExecutionResourceKey resourceKey,
                                     const CoreComponentPtr& component,
                                     std::function<void()>&& callback,
                                     PropertyKey propKey);

    ~ExecutionResourceHolder();

    /**
     * Release this resource holder; the component is no longer valid
     */
    void release();

    /**
     * Take control of the resource, terminating any other users of the resource
     */
    void takeResource();

    /**
     * Release the resource; it is no longer needed.
     */
    void releaseResource();

private:
    void onResourceLoss();
    Sequencer *getSequencer();

private:
    ExecutionResource mResource;
    std::function<void()> mCallback;
    std::weak_ptr<Context> mContext;  // The context is needed to access the sequencer
    bool mHoldingResources = false;

    friend class Sequencer;
};

using ExecutionResourceHolderPtr = std::shared_ptr<ExecutionResourceHolder>;

} // namespace apl

#endif // _APL_EXECUTION_RESOURCE_HOLDER_H
