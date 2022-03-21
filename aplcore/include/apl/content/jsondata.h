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
private:
    enum Type { kDocument, kValue, kNullPtr };

public:
    /**
     * Initialize by moving an existing JSON document.
     * @param document
     */
    JsonData(rapidjson::Document&& document)
        : mDocument(std::move(document)),
          mType(kDocument)
    {}

    /**
     * Initialize by reference to an existing JSON document.  The document
     * is not copied, so another agent must keep it alive during the
     * lifespan of this object.
     * @param value
     */
    JsonData(const rapidjson::Value& value)
        : mValuePtr(&value),
          mType(kValue)
    {}

    /**
     * Initialize by parsing a std::string.
     * @param raw
     */
    JsonData(const std::string& raw)
        : mType(kDocument)
    {
        mDocument.Parse<rapidjson::kParseValidateEncodingFlag |
                        rapidjson::kParseStopWhenDoneFlag>(raw.c_str());
    }

    /**
     * Initialize by parsing a raw string.  The string may be released
     * immediately.
     * @param raw
     */
    JsonData(const char *raw)
        : mType(raw ? kDocument : kNullPtr)
    {
        if (raw != nullptr)
            mDocument.Parse<rapidjson::kParseValidateEncodingFlag |
                            rapidjson::kParseStopWhenDoneFlag>(raw);
    }

    /**
     * Initialize by parsing a raw string in situ.  The string may be
     * modified. Another agent must keep the raw string in memory until
     * this object is destroyed.
     * @param raw
     */
    JsonData(char *raw)
        : mType(raw ? kDocument : kNullPtr)
    {
        if (raw != nullptr)
            mDocument.ParseInsitu<rapidjson::kParseValidateEncodingFlag |
                                  rapidjson::kParseStopWhenDoneFlag>(raw);
    }

    /**
     * @return True if this appears to be a valid JSON object.
     */
    operator bool() const {
        switch (mType) {
            case kDocument:
                return !mDocument.HasParseError();
            case kValue:
                return true;
            case kNullPtr:
            default:
                return false;
        }
    }

    /**
     * @return The offset of the first parse error.
     */
    unsigned int offset() const {
        return mType == kDocument ? mDocument.GetErrorOffset() : 0;
    }

    /**
     * @return The human-readable error state of the parser.
     */
    const char * error() const {
        switch (mType) {
            case kDocument:
                return rapidjson::GetParseError_En(mDocument.GetParseError());
            case kValue:
                return "Value-constructed; no error";
            case kNullPtr:
            default:
                return "Nullptr";
        }
    }

    /**
     * @return A reference to the top-level rapidjson Value.
     */
    const rapidjson::Value& get() const { return mType == kValue ? *mValuePtr : mDocument; }

    /**
     * @return JSON serialized to a string.
     */
    std::string toString() const;

    /**
     * @return Readable string representation of data for debug.
     */
    std::string toDebugString() const;

private:
    rapidjson::Document mDocument;
    const rapidjson::Value *mValuePtr = nullptr;
    Type mType;
};

} // namespace APL

#endif //_APL_JSON_H
