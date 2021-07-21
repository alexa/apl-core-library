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

#ifndef _APL_LIVE_MAP_CHANGE_H
#define _APL_LIVE_MAP_CHANGE_H

namespace apl {

/**
 * Represents a single change to a LiveMap.
 */
class LiveMapChange {
public:
    enum Command {
        SET,
        REMOVE,
        REPLACE
    };

    static LiveMapChange set(const std::string& key) { return {SET, key}; }
    static LiveMapChange remove(const std::string& key) { return {REMOVE, key}; }
    static LiveMapChange replace() { return {REPLACE, ""}; }

    Command command() const { return mCommand; }
    std::string key() const { return mKey; }

private:
    LiveMapChange(Command command, const std::string& key)
        : mCommand(command), mKey(key) {}

private:
    Command mCommand;
    std::string mKey;
};


} // namespace apl

#endif // _APL_LIVE_MAP_CHANGE_H
