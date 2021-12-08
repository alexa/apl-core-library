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

#ifndef _APL_MAKE_UNIQUE_H
#define _APL_MAKE_UNIQUE_H

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

/**
 * This polyfill adds std::make_unique<>.  This is based on the implementation proposed
 * by Stephan T. Lavavej in N3656 (https://isocpp.org/files/papers/N3656.txt)
 *
 * We check the version of C++ implemented by the compiler and only add the polyfill
 * if the compiler is C++11.
 *
 * Three functions are defined:
 *
 * make_unique<T>(args...)
 * make_unique<T[]>(n)
 * make_unique<T[N]>(args...) = delete
 */

#if __cplusplus == 201103L

namespace std {

// Wrap helper templates in namespace to hide them from std
namespace make_unique_detail {

template <class T>
struct __Unique_if {
    typedef unique_ptr<T> _Single_object;
};

template <class T>
struct __Unique_if<T[]> {
    typedef unique_ptr<T[]> _Unknown_bound;
};

template <class T, size_t N>
struct __Unique_if<T[N]> {
    typedef void _Known_bound;
};

} // namespace make_unique_detail

/// make_unique<T>(args...)
template<typename T, typename... Args>
typename make_unique_detail::__Unique_if<T>::_Single_object
make_unique(Args&&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

/// make_unique<T[]>(n)
template<class T>
typename make_unique_detail::__Unique_if<T>::_Unknown_bound
make_unique(size_t n) {
    typedef typename remove_extent<T>::type U;
    return unique_ptr<T>(new U[n]());
}

/// make_unique<T[N]>(args...) = delete
template<typename T, typename... Args>
typename make_unique_detail::__Unique_if<T>::_Known_bound
make_unique(Args&&...) = delete;

} // namespace std

#endif // __cplusplus == 201103L

#endif // _APL_MAKE_UNIQUE_H
