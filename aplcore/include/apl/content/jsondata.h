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

#include "apl/content/sharedjsondata.h"

namespace apl {

class Object;

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
    enum Type { kShared, kValue, kNullPtr };

public:
    /**
     * Initialize by moving an existing JSON document.
     * @param document A RapidJSON document
     */
    JsonData(rapidjson::Document&& document)
        : mSharedJson(SharedJsonData(std::move(document))),
          mType(kShared)
    {}

    /**
     * Initialize by moving an existing SharedJsonData.
     * @param sharedJson A SharedJsonData to move in
     */
    JsonData(SharedJsonData&& sharedJson)
        : mSharedJson(std::move(sharedJson)),
          mType(kShared)
    {}

    /**
     * Initialize by copying an existing SharedJsonData.
     * @param sharedJson A SharedJsonData to copy from
     */
    JsonData(const SharedJsonData& sharedJson)
        : mSharedJson(sharedJson),
          mType(kShared)
    {}

    /**
     * Initialize by reference to an existing JSON document.  The document
     * is not copied, so another agent must keep it alive during the
     * lifespan of this object.
     * @param value A RapidJSON value
     * @deprecated Use SharedJsonData(const std::shared_ptr<rapidjson::Document>&,
                   const rapidjson::Pointer& pointer) instead
     */
    JsonData(const rapidjson::Value& value)
        : mValuePtr(&value),
          mType(kValue)
    {}

    /**
     * Initialize by parsing a std::string.
     * @param raw The string
     */
    JsonData(const std::string& raw)
        : mSharedJson(raw),
          mType(kShared)
    {}

    /**
     * Initialize by parsing a raw string.  The string may be released
     * immediately.
     * @param raw The string
     */
    JsonData(const char *raw)
        : mSharedJson(raw),
          mType(kShared)
    {}

    /**
     * @return True if this appears to be a valid JSON object.
     */
    operator bool() const {
        switch (mType) {
            case kShared:
                return mSharedJson;
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
    size_t offset() const { return mType == kShared ? mSharedJson.offset() : 0; }

    /**
     * @return The human-readable error state of the parser.
     */
    const char * error() const {
        switch (mType) {
            case kShared:
                return mSharedJson.error();
            case kValue:
                return "Value-constructed; no error";
            case kNullPtr:
            default:
                return "Nullptr";
        }
    }

    /**
     * Move the rapidjson state of this to a new Object and return that Object.
     * @return The Object
     */
    Object moveToObject();

    /**
     * @return A reference to the top-level rapidjson Value.
     */
    const rapidjson::Value& get() const { return mType == kValue ? *mValuePtr : mSharedJson.get(); }

    /**
     * @return JSON serialized to a string.
     */
    std::string toString() const;

    /**
     * @return Readable string representation of data for debug.
     */
    std::string toDebugString() const;

private:
    SharedJsonData mSharedJson;
    const rapidjson::Value *mValuePtr = nullptr;
    Type mType;
};

} // namespace APL

#endif //_APL_JSON_H
