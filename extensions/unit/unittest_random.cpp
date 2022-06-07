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

#include <functional>

#include <alexaext/alexaext.h>
#include <gtest/gtest.h>

TEST(Random, DefaultParameters) {
    auto value = alexaext::generateBase36Token();

    ASSERT_EQ(8, value.length());
    ASSERT_NE(value, alexaext::generateBase36Token());
}

TEST(Random, Size) {
    auto value = alexaext::generateBase36Token("", 10);

    ASSERT_EQ(10, value.length());
}

TEST(Random, Prefix) {
    auto value = alexaext::generateBase36Token("unit-");

    ASSERT_EQ("unit-", value.substr(0, 5));
}