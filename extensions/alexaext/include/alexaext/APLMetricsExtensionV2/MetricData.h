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

#ifndef APL_METRIC_DATA_H
#define APL_METRIC_DATA_H

#include <map>
#include <string>

namespace alexaext {
namespace metricsExtensionV2 {
/**
 * This struct is used to store all the captured information about
 * a metric.
 */
typedef struct Metric {

    /// name will be used to store metric name. This name will be later used
    /// while publishing metric to destination.
    std::string name;

    ///  dimensions: This will be used if there are additional dimension specific to this metric.
    ///  If dimension key is already present with document setting, this dimension will override it.
    std::map<std::string, std::string> dimensions;

    ///  value: To store the value calculated for this mertic
    ///  For Timer Metric:
    ///  Value = (endTime - startTime) of metric
    ///  For Counter Metric
    ///  Value = Counter
    double value;
} Metric;

} // namespace metricsExtensionV2
} // namespace alexaext

#endif // APL_METRIC_DATA_H
