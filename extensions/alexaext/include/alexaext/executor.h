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

#ifndef _ALEXAEXT_EXECUTOR_H
#define _ALEXAEXT_EXECUTOR_H

#include <functional>
#include <memory>

namespace alexaext {

/**
 * Defines the contract for extension task executors. Executors typically execute enqueued tasks in parallel, e.g.
 * by using a thread pool.
 */
class Executor {
public:
    using Task = std::function<void(void)>;

    virtual ~Executor() = default;

    /**
     * Enqueues a task for execution. The task may be executed asynchronously, after this method returns.
     *
     * @param task The task to execute
     * @return @c true if the task was successfully enqueued (or executed), @c false otherwise (i.e. the task will never
     *         be executed)
     */
    virtual bool enqueueTask(Task task) = 0;

    /**
     * @return A shared instance of a synchronous executor.
     */
    static std::shared_ptr<Executor> getSynchronousExecutor();
};

using ExecutorPtr = std::shared_ptr<Executor>;



} // namespace alexaext

#endif //_ALEXAEXT_EXECUTOR_H
