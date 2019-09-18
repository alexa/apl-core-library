/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef _APL_USERDATA_H
#define _APL_USERDATA_H

namespace apl{

/**
 * Extend this class to allow a client to attach a chunk of data to it.
 */
class UserData {
public:
    UserData(): mUserData(nullptr) {}

    /**
     * Store user data with this object. It is the responsibility
     * of the entity assigning UserData to ensure it is deleted or
     * cleaned up in an appropriate fashion.
     * @param userData The data to store.
     */
    void setUserData(void* userData) { mUserData = userData; }

    /**
     * @return User raw void data stored with this object.
     */
    void* getUserData() const { return mUserData; }

    /**
     * Convenience function for casting user data.
     * @return User typed data stored with this object
     */
    template<class T>
    T* getUserData() const { return static_cast<T*>(mUserData); }
private:
    void* mUserData;
};
}
#endif //APL_USERDATA_H
