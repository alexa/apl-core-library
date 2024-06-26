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

#ifndef APL_DESTINATIONFACTORYINTERFACE_H
#define APL_DESTINATIONFACTORYINTERFACE_H

#include "DestinationInterface.h"
#include "MetricData.h"
#include <memory>
#include <rapidjson/document.h>

namespace alexaext {
namespace metricsExtensionV2 {
/**
 * The destination factory interface is responsible to create destination
 * object as per provided settings.
 */
class DestinationFactoryInterface {
public:
    /**
     * Destructor
     */
    virtual ~DestinationFactoryInterface() = default;

    /**
     * Provides an instance of \c DestinationInterface based on the destination type and options
     * present inside the Json
     * @param settings contains a json object as below:
     * {
     *   "destination": {
     *       "type": "DESTINATION_TYPE", // String
     *       "destinationConfig1": "MY_CONFIG_1",
     *       "destinationConfig2": "MY_CONFIG_2",
     *       "predefinedKeys": [
     *       "DEVICE_LANGUAGE",
     *       "TIME_ZONE"
     *       ]
     *   },
     *   "dimensions": {
     *       "applicationId": "<applicationId>",
     *       "experienceId": "<experienceId>"
     *      }
     *   }
     *
     * @returns a valid instance of the destnation interface which will honor the provided options
     */
    virtual std::shared_ptr<DestinationInterface>
    createDestination(const rapidjson::Value& settings) = 0;
};

using DestinationFactoryInterfacePtr = std::shared_ptr<DestinationFactoryInterface>;

} // namespace metricsExtensionV2
} // namespace alexaext

#endif // APL_DESTINATIONFACTORYINTERFACE_H
