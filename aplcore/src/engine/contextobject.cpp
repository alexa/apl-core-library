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

#include "apl/engine/contextobject.h"

namespace apl {

std::string
ContextObject::toDebugString() const
{
    if (mProvenance.empty())
        return mValue.toDebugString();

    return mValue.toDebugString() + " [" + mProvenance.toString() + "]";
}

streamer&
operator<<(streamer& os, const ContextObject& object)
{
    os << object.toDebugString();
    return os;
}

} // namespace apl;