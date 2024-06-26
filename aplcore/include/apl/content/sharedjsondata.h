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

#ifndef _APL_SAFE_JSON_H
#define _APL_SAFE_JSON_H

#include <memory>
#include <string>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/pointer.h"
#include "rapidjson/stringbuffer.h"

namespace apl {

/**
 * Wrapper class for holding JSON data.
 *
 * There are a variety of ways of receiving JSON data including loading
 * directly from a string or character pointer, loading from a parsed file,
 * and loading from within a directive.  This wrapper class holds the parsed
 * JSON data with a consistent surface area. Unlike the JsonData class, the
 * life cycle of the JSON data is extended to this object. 
 */
class SharedJsonData {

public:
    /**
     * Initialize by moving an existing JSON document.
     * @param document A RapidJSON document
     */
    explicit SharedJsonData(rapidjson::Document&& document)
        : mDocument(std::make_shared<rapidjson::Document>(std::move(document)))
    {
        mValuePtr = mDocument.get();
    }

    /**
     * Initialize by sharing an existing JSON document.
     * @param document A RapidJSON document
     */
    explicit SharedJsonData(const std::shared_ptr<rapidjson::Document>& document)
        : mDocument(document)
    {
        assert(mDocument);
        mValuePtr = mDocument.get();
    }

    /**
     * Initialize by reference to a portion of an existing JSON document.
     * @param document A RapidJSON document
     * @param pointer Pointer to an object of the JSON document
     */
    SharedJsonData(const std::shared_ptr<rapidjson::Document>& document,
                   const rapidjson::Pointer& pointer)
        : mDocument(document)
    {
        assert(mDocument);

        if (!pointer.IsValid()) {
            mError = "Bad rapidjson::Pointer: Code " + std::to_string(pointer.GetParseErrorCode())
                    + " at " + std::to_string(pointer.GetParseErrorOffset());
            mDocument = nullptr;
            return;
        }

        mValuePtr = rapidjson::GetValueByPointer(*mDocument, pointer);
        if (!mValuePtr) {
            rapidjson::StringBuffer sb;
            pointer.Stringify(sb);
            mError = "Invalid pointer path: " + std::string(sb.GetString());
            mDocument = nullptr;
        }
    }

    /**
     * Initialize by parsing a std::string.
     * @param raw The string
     */
    explicit SharedJsonData(const std::string& raw)
        : mDocument(std::make_shared<rapidjson::Document>())
    {
        mDocument->Parse<rapidjson::kParseValidateEncodingFlag |
                         rapidjson::kParseStopWhenDoneFlag>(raw.c_str());
        mValuePtr = mDocument.get();
    }

    /**
     * Initialize by parsing a raw string. The string may be released
     * immediately.
     * @param raw The string
     */
    explicit SharedJsonData(const char *raw)
    {
        if (!raw) {
            mError = "Nullptr";
            return;
        }

        mDocument = std::make_shared<rapidjson::Document>();
        mDocument->Parse<rapidjson::kParseValidateEncodingFlag |
                         rapidjson::kParseStopWhenDoneFlag>(raw);
        mValuePtr = mDocument.get();
    }

    /**
     * @return True if this appears to be a valid JSON object.
     */
    operator bool() const {
        return mDocument && mError.empty() && !mDocument->HasParseError();
    }

    /**
     * @return The offset of the first parse error.
     */
    size_t offset() const {
        if (!mDocument) return 0;
        if (!mError.empty()) return 0;
        return mDocument->GetErrorOffset();
    }

    /**
     * @return The human-readable error state of the parser.
     */
    const char * error() const {
        if (!mError.empty()) return mError.c_str();
        if (!mDocument) return "Nullptr";
        return rapidjson::GetParseError_En(mDocument->GetParseError());
    }

    /**
     * @return The shared_ptr of Document that ensures the life cycle.
     */
    const std::shared_ptr<rapidjson::Document>& getSharedDoc() const { return mDocument; }

    /**
     * @return A reference to the top-level rapidjson Value.
     */
    const rapidjson::Value& get() const {
        assert(mValuePtr);
        return *mValuePtr;
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
    friend class JsonData;
    SharedJsonData() = default;

private:
    // mDocument is used to hold the strong reference of the JSON data 
    std::shared_ptr<rapidjson::Document> mDocument;
    const rapidjson::Value* mValuePtr = nullptr;
    std::string mError;
};

} // namespace APL

#endif //_APL_SAFE_JSON_H
