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

#ifndef _APL_PRINCIPAL_PTR_H
#define _APL_PRINCIPAL_PTR_H

namespace apl {

/**
 * A principal_ptr is a special type of shared_ptr that calls a method on the object
 * when the pointer goes out of scope.
 *
 * @tparam T The underlying object type.
 * @tparam Method The method to call when the pointer is released
 */
template<class T, void (T::*Method)()>
class principal_ptr {
public:
    constexpr principal_ptr() noexcept {}

    principal_ptr(const std::shared_ptr<T>& ptr) noexcept : mPtr(ptr) {}
    principal_ptr(std::shared_ptr<T>&& ptr) noexcept : mPtr(std::move(ptr)) {}

    principal_ptr& operator=(const std::shared_ptr<T>& ptr) {
        if (mPtr)
            (mPtr.get()->*Method)();
        mPtr = ptr;
        return *this;
    }

    principal_ptr& operator=(std::shared_ptr<T>&& ptr) {
        if (mPtr)
            (mPtr.get()->*Method)();
        mPtr = std::move(ptr);
        return *this;
    }

    ~principal_ptr() {
        if (mPtr)
            (mPtr.get()->*Method)();
    }

    std::shared_ptr<T> getPtr() const noexcept { return mPtr; }

    void reset() { if (mPtr) (mPtr.get()->*Method)(); mPtr = nullptr; }
    T* get() const noexcept { return mPtr.get(); }
    T& operator*() const noexcept { return *mPtr; }
    T* operator->() const noexcept { return mPtr.get(); }
    explicit operator bool() const noexcept { return (bool) mPtr; }

    bool operator==(const std::shared_ptr<T>& other) noexcept { return mPtr == other; }
    bool operator!=(const std::shared_ptr<T>& other) noexcept { return mPtr != other; }

private:
    std::shared_ptr<T> mPtr;
};

} // namespace apl

#endif // _APL_PRINCIPAL_PTR_H
