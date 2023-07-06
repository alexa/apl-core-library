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

#include "apl/engine/corerootcontext.h"

namespace apl {

RootContextPtr
RootContext::create(const Metrics& metrics, const ContentPtr& content)
{
    return create(metrics, content, RootConfig(), nullptr);
}

RootContextPtr
RootContext::create(const Metrics& metrics, const ContentPtr& content, const RootConfig& config)
{
    return create(metrics, content, config, nullptr);
}

RootContextPtr
RootContext::create(const Metrics& metrics,
                    const ContentPtr& content,
                    const RootConfig& config,
                    std::function<void(const RootContextPtr&)> callback)
{
    return CoreRootContext::create(metrics, content, config, callback);
}

streamer&
operator<<(streamer& os, const RootContext& root)
{
    os << "RootContext: " << root.context();
    return os;
}

} // namespace apl
