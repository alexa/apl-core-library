/*
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

#include "alexaext/APLWebflowExtension/AplWebflowBase.h"

using namespace alexaext::webflow;

AplWebflowBase::AplWebflowBase(std::string token, std::string url, std::string flowId)
    : mToken(std::move(token)),
      mUrl(std::move(url)),
      mFlowId(std::move(flowId)) {}

const std::string&
AplWebflowBase::getUrl() const
{
    return mUrl;
}

const std::string&
AplWebflowBase::getFlowId() const
{
    return mFlowId;
}

const std::string&
AplWebflowBase::getToken() const
{
    return mToken;
}