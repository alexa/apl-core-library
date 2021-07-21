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

#ifndef _APL_CONTEXT_OBJECT_H
#define _APL_CONTEXT_OBJECT_H

#include <string>

#include "apl/primitives/object.h"
#include "apl/utils/path.h"

namespace apl {

/**
 * A ContextObject stores information about a single key-value pair in a data-binding
 * context.  This includes the current value, the provenance (where the key-value pair
 * was defined), and whether or not the user may write a changed value.
 */
class ContextObject {
public:
    explicit ContextObject(const Object& value)
        : mValue(value)
    {}

    /**
     * Assign a provenance.
     * @param path The path data to associate with this object.
     * @return This object, for chaining.
     */
    ContextObject& provenance(const Path& path) { mProvenance = path; return *this; }

    /**
     * Mark this object as system-writeable.
     * @return This object, for chaining.
     */
    ContextObject& systemWriteable() { mMutable = true; return *this; }

    /**
     * Mark this object as user-writeable.
     * @return This object, for chaining.
     */
    ContextObject& userWriteable() { mUserWriteable = true; mMutable = true; return *this; }

    /**
     * @return The value of the object.
     */
    const Object& value() const { return mValue; }

    /**
     * @return The path data associated with this object
     */
    const Path& provenance() const { return mProvenance; }

    /**
     * @return True if this value may be changed and hence dependency relationships should be tracked.
     */
    bool isMutable() const { return mMutable; }

    /**
     * @return True if this value may be changed by the user with a SetValue command
     */
    bool isUserWriteable() const { return mUserWriteable; }

    /**
     * Change the value stored.  Non-mutable objects will never change their value.
     * @param value The new value to store.
     * @return True if the value was changed.
     */
    bool set(const Object& value) {
        bool result = (mMutable && mValue != value);
        if (result)
            mValue = value;
        return result;
    }

    std::string toDebugString() const;

    friend streamer& operator<<(streamer&, const ContextObject&);

private:
    Object mValue;
    Path mProvenance;
    bool mMutable = false;
    bool mUserWriteable = false;
};

} // namespace apl

#endif // _APL_CONTEXT_OBJECT_H
