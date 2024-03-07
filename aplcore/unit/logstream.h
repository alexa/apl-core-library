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
 *
 */

#ifndef APL_LOGSTREAM_H
#define APL_LOGSTREAM_H

#include "apl/apl.h"
#include "rapidjson/prettywriter.h"

namespace apl {

/**
 * A convenience class to write a rapidjson object to the debug log.
 * You can use this with the rapidjson pretty printer class.
 */
class LogOSStream {
public:
    typedef char Ch;

    Ch Peek() const { assert(false); return '\0'; }
    Ch Take() { assert(false); return '\0'; }
    size_t Tell() const { return 0; }

    Ch* PutBegin() { assert(false); return 0; }
    void Put(Ch c) { buf.push_back(c); }
    void Flush() {
        LOG(LogLevel::kDebug) << buf;
        buf.clear();
    }
    size_t PutEnd(Ch*) { assert(false); return 0; }

private:
    std::string buf;
};

/**
 * Write a rapidjson Value to the debug log
 * @param value The value to write.
 */
void dumpRapidJSONValue( rapidjson::Value& value )
{
    LogOSStream log;
    rapidjson::PrettyWriter<LogOSStream> writer(log);
    value.Accept(writer);
}

} // namespace apl

#endif // APL_LOGSTREAM_H
