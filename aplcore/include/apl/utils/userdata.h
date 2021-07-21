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

#ifndef _APL_USERDATA_H
#define _APL_USERDATA_H

#include <functional>
#include "apl/apl_config.h"

namespace apl{

/**
 * Extend this class to allow a client to attach a chunk of data to it.
 * If you assign a release callback, it will be called from the object destructor.
 */
template<class Base>
class UserData {
public:
#ifdef USER_DATA_RELEASE_CALLBACKS
    using ReleaseCallback = std::function<void(void*)>;
#endif

    virtual ~UserData() {
#ifdef USER_DATA_RELEASE_CALLBACKS
        if (sReleaseCallback)
            sReleaseCallback(mUserData);
#endif
    }

#ifdef USER_DATA_RELEASE_CALLBACKS
    /**
     * Assign a class-specific callback to be executed in the destructor of an object of this class.
     * The user data pointer is passed in the release callback.
     * @param callback The release function to be executed at object destruction.
     */
    static void setUserDataReleaseCallback(ReleaseCallback callback) {
        sReleaseCallback = std::move(callback);
    }
#endif

    /**
     * Store user data with this object. It is the responsibility
     * of the entity assigning UserData to ensure it is deleted or
     * cleaned up in an appropriate fashion. The release callback may
     * be used to ensure that the user data is property deallocated when
     * the object destructor runs.
     * @param userData The data to store.
     */
    void setUserData(void* userData ) {
        mUserData = userData;
    }

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
    void* mUserData = nullptr;

#ifdef USER_DATA_RELEASE_CALLBACKS
    static ReleaseCallback sReleaseCallback;
#endif

};

#ifdef USER_DATA_RELEASE_CALLBACKS
    template<class Base>
    typename UserData<Base>::ReleaseCallback UserData<Base>::sReleaseCallback = nullptr;
#endif

}
#endif //APL_USERDATA_H
