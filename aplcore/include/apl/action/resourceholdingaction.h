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

#ifndef _APL_RESOURCE_HOLDING_ACTION_H
#define _APL_RESOURCE_HOLDING_ACTION_H

#include "apl/action/action.h"

namespace apl {

/**
 * Commodity class representing action that releases resources on finish.
 */
class ResourceHoldingAction : public Action {
public:
    ResourceHoldingAction(const TimersPtr& timers,
                          const ContextPtr& context);

protected:
    void onFinish() override;

    ContextPtr mContext;
};


} // namespace apl

#endif //_APL_ANIMATE_ITEM_ACTION_H
