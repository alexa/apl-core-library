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
 *
 */

/**
 * This file contains some common tests for comparing primitive objects
 */

#ifndef _APL_TEST_COMPARISONS_H
#define _APL_TEST_COMPARISONS_H

#include "gtest/gtest.h"

#include "apl/primitives/transform2d.h"
#include "apl/primitives/point.h"
#include "apl/primitives/rect.h"
#include "apl/primitives/object.h"

namespace apl {

inline ::testing::AssertionResult
IsEqual(float lhs, float rhs, float epsilon = 0.0001) {
    if (std::abs(lhs - rhs) > epsilon)
        return ::testing::AssertionFailure() << lhs << " != " << rhs;

    return ::testing::AssertionSuccess();
}

inline ::testing::AssertionResult
IsEqual(const Transform2D& lhs, const Transform2D& rhs, float epsilon = 0.0001) {
    for (int i = 0; i < 6; i++)
        if (std::abs(lhs.get()[i] - rhs.get()[i]) > epsilon) {
            streamer s;
            s << "[" << lhs << "] != [" << rhs << "]";
            return ::testing::AssertionFailure() << s.str();
        }

    return ::testing::AssertionSuccess();
}

inline ::testing::AssertionResult
IsEqual(const Point& lhs, const Point& rhs, float epsilon = 0.0001) {
    if (std::abs(lhs.getX() - rhs.getX()) > epsilon || std::abs(lhs.getY() - rhs.getY()) > epsilon)
        return ::testing::AssertionFailure()
               << lhs.toDebugString() << " != " << rhs.toDebugString();

    return ::testing::AssertionSuccess();
}

inline ::testing::AssertionResult
IsEqual(const Rect& lhs, const Rect& rhs, float epsilon = 0.0001) {
    if (std::abs(lhs.getX() - rhs.getX()) > epsilon ||
        std::abs(lhs.getY() - rhs.getY()) > epsilon ||
        std::abs(lhs.getWidth() - rhs.getWidth()) > epsilon ||
        std::abs(lhs.getHeight() - rhs.getHeight()) > epsilon)
        return ::testing::AssertionFailure()
               << lhs.toDebugString() << " != " << rhs.toDebugString();

    return ::testing::AssertionSuccess();
}

// This template compares two vectors of floating point numbers (doubles, floats, etc)
template <typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true>
::testing::AssertionResult
IsEqual(const std::vector<T>& a, const std::vector<T>& b, float epsilon = 1e-6) {
    if (a.size() != b.size())
        return ::testing::AssertionFailure() << "Size mismatch a=" << a.size() << " b=" << b.size();

    for (int i = 0; i < a.size(); i++)
        if (std::abs(a.at(i) - b.at(i)) > epsilon)
            return ::testing::AssertionFailure()
                   << "Element mismatch index=" << i << " a=" << a.at(i) << " b=" << b.at(i);

    return ::testing::AssertionSuccess();
}

// This template compares two vectors of items that support the '!=' operator
template <class T, typename std::enable_if<!std::is_floating_point<T>::value, bool>::type = true>
::testing::AssertionResult
IsEqual(const std::vector<T>& a, const std::vector<T>& b) {
    if (a.size() != b.size())
        return ::testing::AssertionFailure() << "Size mismatch a=" << a.size() << " b=" << b.size();

    for (int i = 0; i < a.size(); i++)
        if (a.at(i) != b.at(i))
            return ::testing::AssertionFailure()
                   << "Element mismatch index=" << i << " a=" << a.at(i) << " b=" << b.at(i);

    return ::testing::AssertionSuccess();
}

inline ::testing::AssertionResult
IsEqual(const Object& lhs, const Object& rhs) {
    if (lhs != rhs)
        return ::testing::AssertionFailure()
               << lhs.toDebugString() << " != " << rhs.toDebugString();

    return ::testing::AssertionSuccess();
}

} // namespace apl


#endif //_APL_TEST_COMPARISONS_H
