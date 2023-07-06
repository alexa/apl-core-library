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
#ifndef APL_APLWEBFLOW_H
#define APL_APLWEBFLOW_H

#include <string>
#include <memory>

namespace alexaext {
namespace webflow {

/**
 * Webflow base class. It handles the launch and event processing.
 *
 * This is a helper base class to encourage best practices.
 */
class AplWebflowBase {
public:
    /**
     * Constructor of the webflow
     *
     * @param token Meta information about the webflow
     * @param url URL we want to open
     * @param flowId flow identier of this object
     */
    AplWebflowBase(std::string token, std::string url, std::string flowId);

    /**
     * Destructor
     */
    virtual ~AplWebflowBase() = default;

    /**
     * Starts a webflow
     */
    virtual bool launch() = 0;

    /**
     * Gets the Url of the webflow
     *
     * @return string with the url of the webflow
     */
    const std::string& getUrl() const;


    /**
     * Gets the flow of the webflow
     *
     * @return string with the flow of the webflow
     */
    const std::string& getFlowId() const;


    /**
     * Gets the token of the webflow
     *
     * @return string with the token used to start this webflow
     */
    const std::string& getToken() const;
protected:
    std::string mToken;
    std::string mUrl;
    std::string mFlowId;
};

using AplWebflowBasePtr = std::shared_ptr<AplWebflowBase>;

}  // namespace webflow
}  // namespace alexaext

// TODO Remove this: https://tiny.amazon.com/asvt1s36
namespace Webflow = alexaext::webflow;

#endif // APL_APLWEBFLOW_H
