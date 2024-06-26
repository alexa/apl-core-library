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

#ifndef APL_DESTINATIONINTERFACE_H
#define APL_DESTINATIONINTERFACE_H

#include "MetricData.h"
#include <vector>

namespace alexaext {
namespace metricsExtensionV2 {
/**
 * The destination interface is used as interface to all the available destinations
 * to publish metrics.
 */
class DestinationInterface {
public:
    /**
     * Destructor
     */
    virtual ~DestinationInterface() = default;

    /**
     * Publish the current recorded metrics
     */
    virtual void publish(Metric metric) = 0;
    /**
     * Publish list of recorded metrics
     */
    virtual void publish(std::vector<Metric> metric) = 0;
};
} // namespace metricsExtensionV2
} // namespace alexaext

#endif // APL_DESTINATIONINTERFACE_H
