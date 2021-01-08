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

#include "apl/content/configurationchange.h"
#include "apl/content/metrics.h"

namespace apl {

/**
 * Merge this configuration change into a metrics object.
 * @param oldMetrics The old metrics to be updated
 * @return The updated metrics
 */
Metrics
ConfigurationChange::mergeMetrics(const Metrics& oldMetrics) const
{
    auto metrics = oldMetrics;
    if ((mFlags & kConfigurationChangeSize) != 0)
        metrics.size(mPixelWidth, mPixelHeight);

    if ((mFlags & kConfigurationChangeTheme) != 0)
        metrics.theme(mTheme.c_str());

    if ((mFlags & kConfigurationChangeViewportMode) != 0)
        metrics.mode(mViewportMode);

    return metrics;
}

/**
 * Merge this configuration change into a root config object
 * @param oldRootConfig The RootConfig object to be modifie
 * @return The updated RootConfig
 */
RootConfig
ConfigurationChange::mergeRootConfig(const RootConfig& oldRootConfig) const
{
    auto rootConfig = oldRootConfig;
    if ((mFlags & kConfigurationChangeScreenMode) != 0)
        rootConfig.screenMode(mScreenMode);

    if ((mFlags & kConfigurationChangeFontScale) != 0)
        rootConfig.fontScale(mFontScale);

    if ((mFlags & kConfigurationChangeScreenReader) != 0)
        rootConfig.screenReader(mScreenReaderEnabled);

    return rootConfig;
}


} // namespace apl