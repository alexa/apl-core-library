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

using namespace apl;

class DimensionTest : public MemoryWrapper {
public:
    void SetUp() override
    {
        auto m = Metrics().size(2048,1000)
                      .dpi(320)
                      .theme("green")
                      .shape(apl::RECTANGLE);
        c = Context::createTestContext(m, session);
    }

    void TearDown() override
    {
        c = nullptr;
        MemoryWrapper::TearDown();
    }

    ContextPtr c;
};

::testing::AssertionResult IsAbsolute(double value, const Dimension& dimen)
{
    streamer s;
    if (!dimen.isAbsolute()) {
        s << dimen << " is not absolute";
        return ::testing::AssertionFailure() << s.str();
    }
    if (value != dimen.getValue()) {
        s << dimen << " is not equal to " << value;
        return ::testing::AssertionFailure() << s.str();
    }

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult IsRelative(double value, const Dimension& dimen)
{
    streamer s;
    if (!dimen.isRelative()) {
        s << dimen << " is not relative";
        return ::testing::AssertionFailure() << s.str();
    }
    if (value != dimen.getValue()) {
        s << dimen << " is not equal to " << value;
        return ::testing::AssertionFailure() << s.str();
    }

    return ::testing::AssertionSuccess();
}

TEST_F(DimensionTest, Basic)
{
    EXPECT_TRUE(Dimension(*c, "auto").isAuto());
    EXPECT_FALSE(Dimension(*c, "auto").isRelative());
    EXPECT_FALSE(Dimension(*c, "auto").isAbsolute());

    EXPECT_TRUE(Dimension(*c, "10px").isAbsolute());
    EXPECT_FALSE(Dimension(*c, "10px").isRelative());
    EXPECT_FALSE(Dimension(*c, "10px").isAuto());
    EXPECT_EQ(5, Dimension(*c, "10px").getValue());

    EXPECT_TRUE(Dimension(*c, "50%").isRelative());
    EXPECT_FALSE(Dimension(*c, "50%").isAbsolute());
    EXPECT_FALSE(Dimension(*c, "50%").isAuto());
    EXPECT_EQ(50, Dimension(*c, "50%").getValue());

    EXPECT_TRUE(Dimension(*c, "     auto  ").isAuto());

    EXPECT_TRUE(IsAbsolute(1024, Dimension(*c, "  100 vw ")));
    EXPECT_TRUE(IsAbsolute(250, Dimension(*c, "50vh")));
    EXPECT_TRUE(IsAbsolute(125, Dimension(*c, "125  dp")));
    EXPECT_TRUE(IsAbsolute(150, Dimension(*c, "150")));
    EXPECT_TRUE(IsAbsolute(150, Dimension(*c, "   300px ")));

    EXPECT_TRUE(IsRelative(30, Dimension(*c, "30%")));

    EXPECT_TRUE(IsAbsolute(0, Dimension(*c, "")));
    EXPECT_TRUE(IsAbsolute(0, Dimension(*c, "pixel")));

    EXPECT_TRUE(IsRelative(-30, Dimension(*c, "-30%")));
    EXPECT_TRUE(IsRelative(-124, Dimension(*c, "  -124%  ")));
}

TEST_F(DimensionTest, PreferRelative)
{
    EXPECT_TRUE(Dimension(*c, "auto", true).isAuto());
    EXPECT_FALSE(Dimension(*c, "auto", true).isRelative());
    EXPECT_FALSE(Dimension(*c, "auto", true).isAbsolute());

    EXPECT_TRUE(Dimension(*c, "10px", true).isAbsolute());
    EXPECT_FALSE(Dimension(*c, "10px", true).isRelative());
    EXPECT_FALSE(Dimension(*c, "10px", true).isAuto());
    EXPECT_EQ(5, Dimension(*c, "10px", true).getValue());

    EXPECT_TRUE(Dimension(*c, "50%", true).isRelative());
    EXPECT_FALSE(Dimension(*c, "50%", true).isAbsolute());
    EXPECT_FALSE(Dimension(*c, "50%", true).isAuto());
    EXPECT_EQ(50, Dimension(*c, "50%", true).getValue());

    EXPECT_TRUE(Dimension(*c, "     auto  ", true).isAuto());

    EXPECT_TRUE(IsAbsolute(1024, Dimension(*c, "  100 vw ", true)));
    EXPECT_TRUE(IsAbsolute(250, Dimension(*c, "50vh", true)));
    EXPECT_TRUE(IsAbsolute(125, Dimension(*c, "125  dp", true)));
    EXPECT_TRUE(IsRelative(150, Dimension(*c, "1.5", true)));
    EXPECT_TRUE(IsAbsolute(150, Dimension(*c, "   300px ", true)));

    EXPECT_TRUE(IsRelative(30, Dimension(*c, "30%", true)));

    EXPECT_TRUE(IsAbsolute(0, Dimension(*c, "", true)));
    EXPECT_TRUE(IsAbsolute(0, Dimension(*c, "pixel", true)));

    EXPECT_TRUE(IsRelative(-30, Dimension(*c, "-30%", true)));
    EXPECT_TRUE(IsRelative(-124, Dimension(*c, "  -124%  ", true)));
}
