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

#ifndef _APL_UNIQUEIDOBJECT_H
#define _APL_UNIQUEIDOBJECT_H

#include "rapidjson/document.h"

#include <string>

#include "apl/common.h"
#include "apl/utils/noncopyable.h"

namespace apl {

/**
 * A base class for an object with a unique ID.  The context holds a reference to the
 * unique ID manager.  When the object is created, it is registered with the unique ID
 * manager and when it is destroyed it is unregistered.  This provides a fast-path for
 * finding the object.
 *
 * The object also contains a unique type field which is used for run-time type identification
 * handled by the UIDObjectWrapper template.
 */
class UIDObject : public NonCopyable {
public:
    // Not ideal, but required to distinguish extending class without RTTI.
    enum class UIDObjectType {
        COMPONENT,
        GRAPHIC,
        GRAPHIC_ELEMENT,
        GRAPHIC_PATTERN
    };

    UIDObject(const ContextPtr& context, apl::UIDObject::UIDObjectType type);
    virtual ~UIDObject();

    /**
     * @return The unique ID assigned to this component by the system.
     */
    std::string getUniqueId() const { return mUniqueId; }

    /**
     * @return Type of the object.
     */
    UIDObject::UIDObjectType objectType() const { return mType; }

    /**
     * Equality operator override
     */
    bool operator==(const UIDObject& other) const { return mUniqueId == other.mUniqueId; }

    /**
     * Serialize the data-binding context.
     * @param depth Number of contexts to serialize.  0 -> all but root.
     * @param allocator RapidJSON memory allocator
     * @return An array of contexts
     */
    rapidjson::Value serializeContext(int depth, rapidjson::Document::AllocatorType& allocator);

protected:
    std::string mUniqueId;
    ContextPtr  mContext;

private:
    UIDObject::UIDObjectType mType;
};

} // namespace apl

#endif // _APL_UNIQUEIDOBJECT_H
