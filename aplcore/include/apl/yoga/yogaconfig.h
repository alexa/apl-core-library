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

#ifndef _APL_YOGA_CONFIG_H
#define _APL_YOGA_CONFIG_H

#include "apl/component/componentproperties.h"
#include "apl/component/textmeasurement.h"
#include "apl/content/metrics.h"
#include "apl/primitives/size.h"
#include "apl/utils/noncopyable.h"

namespace apl {

/**
 * Encapsulated representation of YGConfig. Generally a pass-through for Yoga calls, which does not
 * require any Yoga-specific types to be exposed outside of its implementation.
 */
class YogaConfig : public NonCopyable {
public:
    YogaConfig(const Metrics& metrics, bool debug);
    YogaConfig();
    ~YogaConfig();

private:
    friend class YogaNode;
    void* mConfig;
};

} // namespace apl


#endif // _APL_YOGA_PROPERTIES_H
