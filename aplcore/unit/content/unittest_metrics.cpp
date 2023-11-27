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

#include "../testeventloop.h"

using namespace apl;

class MetricsTest : public MemoryWrapper {};

TEST_F(MetricsTest, Basic) {
    auto m = Metrics()
                 .theme("floppy")
                 .size(300, 400)
                 .minAndMaxWidth(200, 500)
                 .dpi(320)
                 .shape(ScreenShape::ROUND)
                 .mode(ViewportMode::kViewportModePC);

    ASSERT_EQ("floppy", m.getTheme());
    ASSERT_EQ(200, m.getHeight()); // Scaling factor of 160/320
    ASSERT_EQ(150, m.getWidth());  // Scaling factor of 160/320
    ASSERT_TRUE(m.getAutoWidth());
    ASSERT_FALSE(m.getAutoHeight());
    ASSERT_EQ(100, m.getMinWidth());  // Scaling factor of 160/320
    ASSERT_EQ(250, m.getMaxWidth());  // Scaling factor of 160/320
    ASSERT_EQ(200, m.getMinHeight());  // Scaling factor of 160/320
    ASSERT_EQ(200, m.getMaxHeight());  // Scaling factor of 160/320
    ASSERT_EQ(320, m.getDpi());
    ASSERT_EQ(ScreenShape::ROUND, m.getScreenShape());
    ASSERT_EQ(ViewportMode::kViewportModePC, m.getViewportMode());

    ASSERT_EQ(200, m.dpToPx(100));
    ASSERT_EQ(100, m.pxToDp(200));
    ASSERT_EQ("round", m.getShape());
    ASSERT_EQ("pc", m.getMode());

    // Check the debug format
    auto s = m.toDebugString();
    const std::regex check_regex("Metrics<.*>$");
    std::smatch match;
    ASSERT_TRUE(std::regex_match(s, match, check_regex));

    for (const auto& test : { "theme=floppy", "size=300x400", "autoSizeWidth=true",
                          "autoSizeHeight=false", "dpi=320", "shape=round", "mode=pc"}) {
        ASSERT_TRUE(s.find(test) != std::string::npos) << test;
    }
}