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

#ifndef _APL_BINDING_H
#define _APL_BINDING_H

#include <string>
#include <functional>

#include "apl/utils/bimap.h"

namespace apl {

class Context;
class Object;

/**
 * Describe the type of an argument or bind parameter.  When specifying a parameter in a layout
 * or a binding value, the user can specify the type.  This enumeration contains the superset of
 * all valid binding types.
 */
enum BindingType {
    kBindingTypeAny,
    kBindingTypeArray,
    kBindingTypeBoolean,
    kBindingTypeColor,
    kBindingTypeComponent,
    kBindingTypeDimension,
    kBindingTypeInteger,
    kBindingTypeMap,
    kBindingTypeNumber,
    kBindingTypeObject,
    kBindingTypeString,
};

/**
 * Bimap connecting user-specified strings with the BindingType.
 */
extern const Bimap<BindingType , std::string> sBindingMap;

using BindingFunction = std::function<Object(const Context&, const Object&)>;

/**
 * Map from BindingType to a function that trys to convert an object into that type.
 */
extern const std::map<BindingType, BindingFunction> sBindingFunctions;

} // namespace apl

#endif //_APL_BINDING_H
