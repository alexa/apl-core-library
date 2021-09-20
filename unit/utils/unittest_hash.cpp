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

#include "../testeventloop.h"
#include <apl/utils/lrucache.h>

using namespace apl;

class HashTest : public MemoryWrapper {};

TEST_F(HashTest, ObjectTypes)
{
    auto context = Context::createTestContext(Metrics(), RootConfig());
    std::hash<Object> hasher;
    ASSERT_EQ(0, hasher(Object::NULL_OBJECT()));
    ASSERT_NE(0, hasher(Object(true)));
    ASSERT_NE(0, hasher(Object("string")));
    ASSERT_NE(0, hasher(Object(1)));
    ASSERT_NE(0, hasher(Object(Dimension(DimensionType::Absolute, 20))));
    ASSERT_NE(0, hasher(Object(Dimension(DimensionType::Auto, 0))));
    ASSERT_NE(0, hasher(Object(Dimension(DimensionType::Relative, 20))));
    ASSERT_NE(0, hasher(Object(Color(0xFFFFFFFF))));
    ASSERT_NE(0, hasher(Object(StyledText::create(*context, Object("Styled text")))));
    ASSERT_EQ(0, hasher(Object::EMPTY_ARRAY()));
    ASSERT_EQ(0, hasher(Object::EMPTY_MAP()));
}

TEST_F(HashTest, CombineOrder)
{
    auto context = Context::createTestContext(Metrics(), RootConfig());
    std::hash<Object> hasher;

    auto hash12 = hasher(Object(1));
    auto hash21 = hasher(Object(2));

    hashCombine(hash12, Object(2));
    hashCombine(hash21, Object(1));

    ASSERT_NE(hash12, hash21);

    auto hash1 = hasher(Object(1));

    hashCombine(hash1, Object(2));

    ASSERT_EQ(hash12, hash1);
}