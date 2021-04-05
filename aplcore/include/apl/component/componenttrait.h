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

#ifndef APL_COMPONENTTRAIT_H
#define APL_COMPONENTTRAIT_H

#include "corecomponent.h"

namespace apl {

/**
 * The base class for all component traits (mixins).
 *
 * Note that in order to avoid complications with multiple inheritance, this class should remain
 * stateless.
 */
class ComponentTrait {
public:

    /**
     * @return the component that implements the current trait.
     */
    virtual CoreComponentPtr getComponent() = 0;
};

} // namespace apl

#endif // APL_COMPONENTTRAIT_H
