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

#include <iostream>

#include "gtest/gtest.h"

#include "apl/engine/evaluate.h"
#include "apl/engine/state.h"
#include "apl/engine/context.h"
#include "apl/content/metrics.h"
#include "apl/utils/session.h"

using namespace apl;

TEST(StateTest, Basic)
{
    State a(kStateDisabled);
    State b(kStateDisabled);

    ASSERT_FALSE( a < b );
    ASSERT_FALSE( b < a );

    State c;

    ASSERT_TRUE( c < a );
    ASSERT_FALSE( a < c );
}

TEST(StateTest, Karaoke)
{
    State a(kStateDisabled);
    State b(kStateDisabled);

    b.toggle(kStateKaraoke);

    ASSERT_TRUE( a < b );
    ASSERT_FALSE( b < a );
}

TEST(StateTest, Extend)
{
    State a(kStateDisabled);
    a.set(kStateKaraoke, true);
    a.set(kStateKaraokeTarget, true);
    auto context = a.extend(Context::createTestContext(Metrics(), makeDefaultSession()));

    ASSERT_FALSE(evaluate(*context, "${state.pressed}").asBoolean());
    ASSERT_TRUE(evaluate(*context, "${state.disabled}").asBoolean());
    ASSERT_FALSE(evaluate(*context, "${state.focused}").asBoolean());
    ASSERT_FALSE(evaluate(*context, "${state.checked}").asBoolean());
    ASSERT_TRUE(evaluate(*context, "${state.karaoke}").asBoolean());
    ASSERT_TRUE(evaluate(*context, "${state.karaokeTarget}").asBoolean());
}

TEST(StateTest, StringToState)
{
    ASSERT_EQ( kStatePressed, State::stringToState("pressed"));
    ASSERT_EQ( kStateDisabled, State::stringToState("disabled"));
    ASSERT_EQ( kStateFocused, State::stringToState("focused"));
    ASSERT_EQ( kStateChecked, State::stringToState("checked"));
    ASSERT_EQ( kStateKaraoke, State::stringToState("karaoke"));
    ASSERT_EQ( kStateKaraokeTarget, State::stringToState("karaokeTarget"));
    ASSERT_EQ( static_cast<StateProperty>(-1), State::stringToState("confusion"));
}