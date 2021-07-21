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

#ifndef _APL_EXTENSION_EVENT_ACTION_H
#define _APL_EXTENSION_EVENT_ACTION_H

#include "apl/action/action.h"

namespace apl {

class ExtensionEventCommand;
class Component;

/**
 * The action executed by the ExtensionEventCommand.  This action sends an Event of type kEventTypeExtension
 * to the view host.  It may or may not require resolution from the view host.
 */
class ExtensionEventAction : public Action {
public:
    static std::shared_ptr<ExtensionEventAction> make(const TimersPtr& timers,
                                                      const std::shared_ptr<ExtensionEventCommand>& command,
                                                      bool requireResolution);

    ExtensionEventAction(const TimersPtr& timers,
                         const std::shared_ptr<ExtensionEventCommand>& command)
        : Action(timers),
          mCommand(command)
    {}

private:
    void start(bool requireResolution);

private:
    std::shared_ptr<ExtensionEventCommand> mCommand;
};

} // namespace apl

#endif // _APL_EXTENSION_EVENT_ACTION_H
