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

#ifndef _APL_UNIQUEIDMANAGER_H
#define _APL_UNIQUEIDMANAGER_H

#include <string>
#include <map>

#include "apl/common.h"
#include "apl/engine/uidgenerator.h"
#include "apl/utils/log.h"

namespace apl {

/**
 * Keep track of all components, graphics, etc. that have a unique ID assigned.
 * Components that inherit from UniqueId automatically register and receive
 * a unique ID from the UniqueIdManager that is associated with the root data context.
 *
 * When the component is released, it is removed from this map.
 */
class UIDManager {
public:
    UIDManager(UIDGenerator& generator, const SessionPtr& session) :
          mSession(session), mGenerator(generator) {}

    std::string create(UIDObject* element);
    UIDObject* find(const std::string& id);

    void terminate() { mTerminated = true; }

private:
    SessionPtr mSession;
    UIDGenerator& mGenerator;
    std::map<std::string, UIDObject*> mMap;
    bool mTerminated = false;

private:
    friend class UIDObject;

    // Only used in UIDObject destructor
    void remove(const std::string& id, UIDObject* element);
};

} // namespace apl

#endif // _APL_UNIQUEIDMANAGER_H
