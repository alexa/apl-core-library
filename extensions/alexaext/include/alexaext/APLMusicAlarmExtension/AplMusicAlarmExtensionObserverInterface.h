/*
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
#ifndef APL_APLMUSICALARMEXTENSIONOBSERVERINTERFACE_H
#define APL_APLMUSICALARMEXTENSIONOBSERVERINTERFACE_H

#include <memory>
#include <string>

namespace alexaext {
namespace musicalarm {

class AplMusicAlarmExtensionObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~AplMusicAlarmExtensionObserverInterface() = default;

    /**
     * The DismissAlarm command is used to dismiss the current ringing alarm.
     */
    virtual void dismissAlarm() = 0;

    /**
     * The SnoozeAlarm command is used to snooze the current ringing alarm.
     */
    virtual void snoozeAlarm() = 0;
};

using AplMusicAlarmExtensionObserverInterfacePtr = std::shared_ptr<AplMusicAlarmExtensionObserverInterface>;

}  // namespace musicalarm
}  // namespace alexaext

// TODO Remove this: https://tiny.amazon.com/asvt1s36
namespace MusicAlarm = alexaext::musicalarm;

#endif // APL_APLMUSICALARMEXTENSIONOBSERVERINTERFACE_H
