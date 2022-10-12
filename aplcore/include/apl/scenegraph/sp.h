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

#ifndef _APL_SP_H
#define _APL_SP_H

namespace apl {
namespace sg {

/**
 * Base class for objects that can be referenced by the non-thread-safe "sp" template
 * class.
 */
class RefCounted {
public:
    virtual ~RefCounted() = default;

    // Block stack copy-constructor and assignment
    RefCounted(const RefCounted&) = delete;
    RefCounted& operator=(const RefCounted&) = delete;
    RefCounted(RefCounted&&) = delete;
    RefCounted& operator=(RefCounted&&) = delete;

    void incrementRef() {
        mCounter++;
    }

    void decrementRef() {
        assert(mCounter > 0);
        mCounter--;
        if (!mCounter)
            delete this;
    }

protected:
    RefCounted() = default;

private:
    int mCounter = 0;
};

/**
 * Templated smart pointer class for RefCounted items.
 * @tparam T Subclass of RefCounted.
 */
template<typename T>
class sp {
public:
    // Constructors
    constexpr sp() noexcept : mPtr(nullptr) {}
    sp(const sp& r) noexcept : mPtr(r.mPtr) { if (mPtr) mPtr->incrementRef(); }
    sp(sp&& r) noexcept : mPtr(r.mPtr) { r.mPtr = nullptr; }

    template<class Y>
    sp(Y* p) noexcept
        : mPtr(p->mPtr)
    {
        if (mPtr)
            mPtr->incrementRef();
    }

    template<class Y> explicit
    sp(const sp<Y>& r) noexcept
        : mPtr(r.mPtr)
    {
        if (mPtr)
            mPtr->incrementRef();
    }

    template<class Y> explicit
    sp(sp<Y>&& r) noexcept
        : mPtr(r.mPtr)
    {
        r.mPtr = nullptr;
    }

    // Destructor
    ~sp() {
        if (mPtr)
            mPtr->decrementRef();
    }

    // Assignment
    sp& operator=(const sp& r) noexcept {
        if (this == r) return *this;
        if (mPtr) mPtr->decrementRef();
        mPtr = r.mPtr;
        if (mPtr) mPtr->incrementRef();
        return *this;
    }

    sp& operator=(sp&& r) noexcept {
        if (this == r) return *this;
        if (mPtr) mPtr->decrementRef();
        mPtr = r.mPtr;
        r.mPtr = nullptr;
        return *this;
    }

    template<class Y>
    sp& operator=(const sp<Y>& r) noexcept {
        if (this == r) return *this;
        if (mPtr) mPtr->decrementRef();
        mPtr = r.mPtr;
        if (mPtr) mPtr->incrementRef();
        return *this;
    }

    template<class Y>
    sp& operator=(sp<Y>&& r) noexcept {
        if (this == r) return *this;
        if (mPtr) mPtr->decrementRef();
        mPtr = r.mPtr;
        r.mPtr = nullptr;
        return *this;
    }

    // Modifiers
    void reset() noexcept {
        if (mPtr) mPtr->decrementRef();
        mPtr = nullptr;
    }

    // Observers
    T* get() const noexcept { return mPtr; }
    T& operator*() const noexcept { return *mPtr; }
    T& operator->() const noexcept { return *mPtr; }
    explicit operator bool() const noexcept { return mPtr; }

    // Comparisons
    bool operator==(const sp& r) { return mPtr == r.mPtr; }
    bool operator!=(const sp& r) { return mPtr != r.mPtr; }

private:
    T* mPtr;
};


} // namespace sg
} // namespace apl

#endif // _APL_SP_H
