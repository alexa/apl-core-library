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

#ifndef _APL_EXTENSION_PROPERTY_H
#define _APL_EXTENSION_PROPERTY_H

#include "apl/primitives/object.h"
#include "apl/engine/binding.h"

namespace apl {

/**
 * Extension property definition.
 */
struct ExtensionProperty {
    /// Binding type
    BindingType btype;
    /// Default value
    Object defvalue;
    /// Required flag
    bool required;
};

} // namespace apl

#endif // _APL_EXTENSION_PROPERTY_H