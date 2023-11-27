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

#include "apl/command/logcommand.h"
#include "apl/utils/session.h"

namespace apl {

const static bool DEBUG_LOG_COMMAND = false;

const CommandPropDefSet&
LogCommand::propDefSet() const
{
    static CommandPropDefSet sLogCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyLevel,     kCommandLogLevelInfo,  asAny},
            {kCommandPropertyMessage,   "",                    asString},
            {kCommandPropertyArguments, Object::EMPTY_ARRAY(), asArray},
    });
    return sLogCommandProperties;
}

ActionPtr
LogCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!mContext || !calculateProperties())
        return nullptr;

    // Level can be specified as a number or a string.
    int levelValue = kCommandLogLevelInfo;
    auto level = getValue(kCommandPropertyLevel);
    if (level.isNumber()) {
        auto levelAsInt = level.asInt();
        levelValue = sCommandLogLevelMap.has(levelAsInt) ? levelAsInt : levelValue;
    } else {
        levelValue = sCommandLogLevelMap.get(level.asString(), levelValue);
    }

    // Translate command property to logger log level
    LogLevel logLevel = LogLevel::kInfo;
    switch (levelValue) {
        case kCommandLogLevelDebug:
            logLevel = LogLevel::kDebug;
            break;
        case kCommandLogLevelInfo:
            logLevel = LogLevel::kInfo;
            break;
        case kCommandLogLevelWarn:
            logLevel = LogLevel::kWarn;
            break;
        case kCommandLogLevelError:
            logLevel = LogLevel::kError;
            break;
        case kCommandLogLevelCritical:
            logLevel = LogLevel::kCritical;
            break;
        default:
            // If we add a command level property and forget to add a case above, we end up here
            assert(false);
    }

    // Freeze the arguments array as a JSON object
    mArguments = mValues.at(kCommandPropertyArguments).serialize(mDocument.GetAllocator());
    rapidjson::Document argumentsDoc;
    argumentsDoc.CopyFrom(mArguments, argumentsDoc.GetAllocator());

    // Freeze the "event.source" property as a JSON object
    auto event = mContext->opt("event");
    if (event.empty()) {
        LOG(LogLevel::kError)
            << "Event field not available in context. Should not happen during normal operation.";
        return nullptr;
    }
    mSource = event.get("source").serialize(mDocument.GetAllocator());
    rapidjson::Document sourceDoc;
    sourceDoc.CopyFrom(mSource, sourceDoc.GetAllocator());

    LogCommandMessage message{
        getValue(kCommandPropertyMessage).asString(),
        logLevel,
        Object(std::move(argumentsDoc)),
        Object(std::move(sourceDoc))
    };

    if (DEBUG_LOG_COMMAND) {
        LOG(LogLevel::kDebug).session(mContext) << "Log command: " << message.text
            << ", level=" << sCommandLogLevelMap.at(levelValue)
            << ", arguments=" << message.arguments.toDebugString()
            << ", origin=" << message.origin.toDebugString();
    }

    mContext->session()->write(std::move(message));

    return nullptr;
}

} // namespace apl
