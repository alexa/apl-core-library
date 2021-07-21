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

#ifndef _APL_LIVE_OBJECT_H
#define _APL_LIVE_OBJECT_H

#include "apl/common.h"
#include "apl/primitives/object.h"

namespace apl {

/**
 * Base class for LiveArray and LiveMap
 */
class LiveObject {
public:
    virtual Object::ObjectType getType() const = 0;
    virtual ~LiveObject() = default;
};

} // namespace apl

#endif // _APL_LIVE_OBJECT_H
