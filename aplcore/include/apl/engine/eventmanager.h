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

#ifndef APL_EVENT_MANAGER_H
#define APL_EVENT_MANAGER_H

#include <memory>

#include "apl/utils/scopeddequeue.h"

#include "apl/engine/event.h"

namespace apl
{

class EventManager : public ScopedDequeue<DocumentContextDataPtr, Event> {};

} // namespace apl

#endif // APL_EVENT_MANAGER_H
