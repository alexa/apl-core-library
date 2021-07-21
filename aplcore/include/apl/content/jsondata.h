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

#ifndef _APL_JSON_H
#define _APL_JSON_H

#include <string>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

namespace apl {

/**
 * Wrapper class for holding JSON data.
 *
 * There are a variety of ways of receiving JSON data including loading
 * directly from a string or character pointer, loading from a parsed file,
 * and loading from within a directive.  This wrapper class holds the parsed
 * JSON data with a consistent surface area.
 */
class JsonData {
public:
    /**
     * Initialize by moving an existing JSON document.
     * @param document
     */
    JsonData(rapidjson::Document&& document)
        : mHasDocument(true),
          mDocument(std::move(document)),
          mValue(mDocument)
    {
    }

    /**
     * Initialize by reference to an existing JSON document.  The document
     * is not copied, so another agent must keep it alive during the
     * lifespan of this object.
     * @param value
     */
    JsonData(const rapidjson::Value& value)
        : mHasDocument(false),
          mValue(value)
    {}

    /**
     * Initialize by parsing a std::string.
     * @param raw
     */
    JsonData(const std::string& raw)
        : mHasDocument(true),
          mDocument(),
          mOk(mDocument.Parse<rapidjson::kParseValidateEncodingFlag | rapidjson::kParseStopWhenDoneFlag>(raw.c_str())),
          mValue(mDocument)
    {}

    /**
     * Initialize by parsing a raw string.  The string may be released
     * immediately.
     * @param raw
     */
    JsonData(const char *raw)
        : mHasDocument(true),
          mDocument(),
          mOk(mDocument.Parse<rapidjson::kParseValidateEncodingFlag | rapidjson::kParseStopWhenDoneFlag>(raw)),
          mValue(mDocument)
    {}

    /**
     * Initialize by parsing a raw string in situ.  The string may be
     * modified. Another agent must keep the raw string in memory until
     * this object is destroyed.
     * @param raw
     */
    JsonData(char *raw)
        : mHasDocument(true),
          mDocument(),
          mOk(mDocument.Parse<rapidjson::kParseValidateEncodingFlag | rapidjson::kParseStopWhenDoneFlag>(raw)),
          mValue(mDocument)
    {}

    /**
     * @return True if this appears to be a valid JSON object.
     */
    operator bool() const {
        return !mOk.IsError();
    }

    /**
     * @return The offset of the first parse error.
     */
    unsigned int offset() const {
        return mOk.Offset();
    }

    /**
     * @return The human-readable error state of the parser.
     */
    const char * error() const {
        return rapidjson::GetParseError_En(mOk.Code());
    }

    /**
     * @return A reference to the top-level rapidjson Value.
     */
    const rapidjson::Value& get() const {
        return mHasDocument ? mDocument : mValue;
    }

    /**
     * @return JSON serialized to a string.
     */
    std::string toString() const;

    /**
     * @return Readable string representation of data for debug.
     */
    std::string toDebugString() const;

private:
    bool mHasDocument;
    rapidjson::Document mDocument;
    rapidjson::ParseResult mOk;
    const rapidjson::Value& mValue;
};

} // namespace APL

#endif //_APL_JSON_H
