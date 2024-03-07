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
 */

#ifndef APL_SCREENLOCKHOLDER_H
#define APL_SCREENLOCKHOLDER_H

#include "apl/engine/context.h"

namespace apl {

/**
 * This is a helper class that holds a screen lock for a component and ensures
 * that the screen lock is released if the component is destroyed.
 * It's designed to be used as a member variable inside a class.  For example::
 *
 *     class FooComponent {
 *       FooComponent() : mScreenLock(mContext) {}
 *       ScreenLockHolder mScreenLock;
 *     }
 */
class ScreenLockHolder {
public:
    explicit ScreenLockHolder(ContextPtr context)
        : mContext(std::move(context)) {
        assert(mContext);
    }

    ~ScreenLockHolder() {
        if (mHasScreenLock)
            mContext->releaseScreenLock();
    }

    /**
     * Acquire the screen lock if it is not already held.
     * This method may be safely called multiple times.
     */
    void take() {
        if (!mHasScreenLock) {
            mHasScreenLock = true;
            mContext->takeScreenLock();
        }
    }

    /**
     * Release the screen lock if it is currently held.
     * This method may be safely called multiple times.
     */
    void release() {
        if (mHasScreenLock) {
            mHasScreenLock = false;
            mContext->releaseScreenLock();
        }
    }

    /**
     * Ensure that the screen lock is held or released based on the argument.
     * This method may be safely called multiple times with the same argument.
     * @param takeScreenLock If true, take the screen lock. Otherwise release it.
     */
    void ensure(bool takeScreenLock) {
        if (takeScreenLock)
            take();
        else
            release();
    }

private:
    ContextPtr mContext;
    bool mHasScreenLock = false;
};

} // namespace apl

#endif // APL_SCREENLOCKHOLDER_H
