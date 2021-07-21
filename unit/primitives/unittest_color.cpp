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

#include "apl/common.h"

using namespace apl;

class ColorTest : public MemoryWrapper {};

TEST_F(ColorTest, Grammar)
{
    ASSERT_EQ( 0xff0000ff, Color(session, "red") );
    ASSERT_EQ( 0x008000ff, Color(session, "green") );
    ASSERT_EQ( 0xeeddbbff, Color(session, "#edb"));
    ASSERT_EQ( 0x11223344, Color(session, "#1234"));
    ASSERT_EQ( 0x123456ff, Color(session, "#123456"));
    ASSERT_EQ( 0xfedcba98, Color(session, "#fedcba98"));

    ASSERT_EQ( 0x0000ff7f, Color(session, "rgba(blue, 50%)"));
    ASSERT_EQ( 0x0080003f, Color(session, "rgb(rgba(green, 50%), 50%)"));

    ASSERT_EQ( 0x8040c0ff, Color(session, "rgb(128, 64, 192)"));
    ASSERT_EQ( 0xff072040, Color(session, "rgba(255, 7, 32, 25%)"));
    ASSERT_EQ( 0xff072040, Color(session, "rgba(255, 7, 32, 0.25)"));
    ASSERT_EQ( 0xff072040, Color(session, "rgba(255, 7, 32, .25)"));

    ASSERT_EQ( 0xb8860bff, Color(session, "darkgoldenrod"));
}

TEST_F(ColorTest, HSL)
{
    ASSERT_EQ( Color::RED, Color(session, "hsl(0, 100%, 50%)"));
    ASSERT_EQ( 0xff8000ff, Color(session, "hsl(30, 100%, 50%)"));
    ASSERT_EQ( 0xffff00ff, Color(session, "hsl(60, 100%, 50%)"));
    ASSERT_EQ( 0x80ff00ff, Color(session, "hsl(90, 100%, 50%)"));
    ASSERT_EQ( 0x00ff00ff, Color(session, "hsl(120, 100%, 50%)"));
    ASSERT_EQ( 0x00FF80ff, Color(session, "hsl(150, 100%, 50%)"));
    ASSERT_EQ( 0x00FFFFff, Color(session, "hsl(180, 100%, 50%)"));
    ASSERT_EQ( 0x007fFFff, Color(session, "hsl(210, 100%, 50%)"));
    ASSERT_EQ( 0x0000FFff, Color(session, "hsl(240, 100%, 50%)"));
    ASSERT_EQ( 0x7f00FFff, Color(session, "hsl(270, 100%, 50%)"));
    ASSERT_EQ( 0xFF00FFff, Color(session, "hsl(300, 100%, 50%)"));
    ASSERT_EQ( 0xFF0080ff, Color(session, "hsl(330, 100%, 50%)"));
    ASSERT_EQ( Color::RED, Color(session, "hsl(360, 100%, 50%)"));

    ASSERT_EQ( 0x000000ff, Color(session, "hsl(120, 100%, 0%)"));
    ASSERT_EQ( 0x006600ff, Color(session, "hsl(120, 100%, 20%)"));
    ASSERT_EQ( 0x00cc00ff, Color(session, "hsl(120, 100%, 40%)"));
    ASSERT_EQ( 0x33FF33ff, Color(session, "hsl(120, 100%, 60%)"));
    ASSERT_EQ( 0x99FF99ff, Color(session, "hsl(120, 100%, 80%)"));
    ASSERT_EQ( 0xFFFFFFff, Color(session, "hsl(120, 100%, 100%)"));

    ASSERT_EQ( 0x00ff00ff, Color(session, "hsl(120, 100%, 50%)"));
    ASSERT_EQ( 0x19E619ff, Color(session, "hsl(120, 80%, 50%)"));
    ASSERT_EQ( 0x33CC33ff, Color(session, "hsl(120, 60%, 50%)"));
    ASSERT_EQ( 0x4dB34dff, Color(session, "hsl(120, 40%, 50%)"));
    ASSERT_EQ( 0x669966ff, Color(session, "hsl(120, 20%, 50%)"));
    ASSERT_EQ( 0x808080ff, Color(session, "hsl(120, 0%, 50%)"));

    ASSERT_EQ( 0x80808080, Color(session, "hsl(120, 0, 0.5, 0.5)"));
    ASSERT_EQ( 0x80808040, Color(session, "hsla(120, 0, 50%, 25%)"));
}

TEST_F(ColorTest, Basic)
{
    auto p = Color(session, "rgb(128, 64, 192, 0.125)");
    ASSERT_EQ(128, p.red());
    ASSERT_EQ(64, p.green());
    ASSERT_EQ(192, p.blue());
    ASSERT_EQ(32, p.alpha());

    p = Color(session, "#12345678");
    ASSERT_EQ(0x12, p.red());
    ASSERT_EQ(0x34, p.green());
    ASSERT_EQ(0x56, p.blue());
    ASSERT_EQ(0x78, p.alpha());
}

const static std::vector<std::string> sErrorTests = {
    "rgb(123 ",
    "bluz",
    "hsl(120, 0, 0, )"
};

TEST_F(ColorTest, Error)
{
    for (auto& m : sErrorTests) {
        ASSERT_EQ(Color::TRANSPARENT, Color(session, m)) << m;
        ASSERT_TRUE(ConsoleMessage()) << m;
    }
}

TEST_F(ColorTest, BadEnumConversionTest)
{
    // cast color to double
    double d_color = Color::RED;

    // since we force the enum to be stored in a uint32_t,
    // it should be able to convert back to a color
    ASSERT_EQ(Color::RED, Color(d_color));
}
