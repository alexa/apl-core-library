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

#ifndef APL_STREAMER_H
#define APL_STREAMER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace apl {
class streamer {
public:
    template <typename T>
    streamer& operator<<(T* __p) {
        if (__p)
            return *this << reinterpret_cast<std::uintptr_t>(__p);
        return *this << "null";
    }

    template <typename T>
    streamer& operator<<(const std::shared_ptr<T>& __p) {
        if (__p.get())
            return *this << *__p.get();
        return *this << "null";
    }

    template <typename T>
    streamer& operator<<(std::shared_ptr<T>& __p) {
        if (__p.get())
            return *this << *__p.get();
        return *this << "null";
    }

    streamer& operator<<(const char* __c) {
        mString += __c;
        return *this;
    }

    streamer& operator<<(char* __c) {
        mString += __c;
        return *this;
    }

    streamer& operator<<(char __c) {
        mString += __c;
        return *this;
    }

    streamer& operator<<(std::string& __s) {
        mString += __s;
        return *this;
    }

    streamer& operator<<(const std::string& __s) {
        mString += __s;
        return *this;
    }

    streamer& operator<<(bool __n) {
        mString += std::to_string(__n);
        return *this;
    }

    streamer& operator<<(short __n) {
        mString += std::to_string(__n);
        return *this;
    }

    streamer& operator<<(unsigned short __n) {
        mString += std::to_string(__n);
        return *this;
    }

    streamer& operator<<(int __n) {
        mString += std::to_string(__n);
        return *this;
    }

    streamer& operator<<(unsigned int __n) {
        mString += std::to_string(__n);
        return *this;
    }

    streamer& operator<<(long __n) {
        mString += std::to_string(__n);
        return *this;
    }

    streamer& operator<<(unsigned long __n) {
        mString += std::to_string(__n);
        return *this;
    }

    streamer& operator<<(long long __n) {
        mString += std::to_string(__n);
        return *this;
    }

    streamer& operator<<(unsigned long long __n) {
        mString += std::to_string(__n);
        return *this;
    }

    streamer& operator<<(float __f) {
        mString += std::to_string(__f);
        return *this;
    }

    streamer& operator<<(double __f) {
        mString += std::to_string(__f);
        return *this;
    }

    streamer& operator<<(long double __f) {
        mString += std::to_string(__f);
        return *this;
    }

    streamer& operator<<(void *__f) {
        char hex[30];
        snprintf(hex, sizeof(hex), "%p", __f);
        mString += hex;
        return *this;
    }

    void reset() {
        mString = "";
    }

    std::string str() { return mString; }

private:
    std::string mString;
};
} // namespace apl

#endif // APL_STREAMER_H
