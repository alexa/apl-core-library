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

/**
 * Verify that the bimap contains one and only one entry for each of the expected strings.  The bimap may
 * also contain alternate spellings for some entries; these should be passed in the alternates argument.
 * @param bimap The bimap to verify
 * @param expected A list of the unique strings we expect to find
 * @param alternates A list of the duplicate string we expect to find.
 * @return success if all tests pass.
 */
::testing::AssertionResult
CheckBimap(const Bimap<int, std::string>& bimap,
           const std::vector<std::string>& expected,
           const std::vector<std::string>& alternates = {})
{
    std::set<int> found;

    if (expected.size() != bimap.size())
        return ::testing::AssertionFailure() << "Expected=" << expected.size() << " Bimap=" << bimap.size();

    // Check the expected strings to ensure that each one occurs exactly once
    for (const auto& m : expected) {
        if (!bimap.has(m))
            return ::testing::AssertionFailure() << "Missing key '" << m << "'";

        auto value = bimap.at(m);
        if (found.find(value) != found.end())
            return ::testing::AssertionFailure() << "Duplicate key '" << m << "'";

        found.emplace(value);
    }

    if (expected.size() != found.size())
        return ::testing::AssertionFailure() << "Expected=" << expected.size() << " Found=" << found.size();

    // Check the alternate strings.  They should already be in the found set.  We also need to check for
    // duplicate alternate strings passed in.
    std::set<std::string> alternatesFound;
    for (const auto& m : alternates) {
        if (!bimap.has(m))
            return ::testing::AssertionFailure() << "Missing alternate '" << m << "'";

        auto value = bimap.at(m);
        if (found.find(value) == found.end())
            return ::testing::AssertionFailure() << "Alternate was not found in original list '" << m << "'";

        if (alternatesFound.find(m) != alternatesFound.end())
            return ::testing::AssertionFailure() << "Duplicate alternate '" << m << "'";

        alternatesFound.emplace(m);
    }

    // The total number of expected and alternates should match the size of the reverse bimap
    auto reverseSize = std::distance(bimap.beginBtoA(), bimap.endBtoA());
    auto expectedAlternateCount = reverseSize - expected.size();
    if (expectedAlternateCount != alternates.size())
        return ::testing::AssertionFailure() << "Unexpected number of alternates actual=" << alternates.size()
                                             << " expected=" << expectedAlternateCount;

    return ::testing::AssertionSuccess();
}

class PropertyTest : public ::testing::Test {};

/**
 * There are two image/vector graphic alignment maps.  They should contain the same values.
 */
TEST_F(PropertyTest, VectorGraphicAlign)
{
    auto expected = std::vector<std::string> {"bottom", "bottom-left", "bottom-right", "top", "top-left",
                                               "top-right", "center", "left", "right" };
    auto alternates = std::vector<std::string> { "bottomLeft", "bottomRight", "topLeft", "topRight" };

    ASSERT_TRUE(CheckBimap(sAlignMap, expected, alternates));
    ASSERT_TRUE(CheckBimap(sVectorGraphicAlignMap, expected, alternates));
}
/**
 * There are many role values and it is easy to type a string incorrectly.
 * This test compares a manually typed in list from the specification to the
 * roles defined in the bimap 'sRoleMap'.
 */
TEST_F(PropertyTest, Roles)
{
    ASSERT_TRUE(CheckBimap(sRoleMap, {
        "none", "adjustable", "alert", "button", "checkbox", "combobox", "header", "image", "imagebutton", "keyboardkey",
        "link", "list", "listitem", "menu", "menubar", "menuitem", "progressbar", "radio", "radiogroup", "scrollbar",
        "search", "spinbutton", "summary", "switch", "tab", "tablist", "text", "timer", "toolbar"
    }));
}

/**
 * Verify all of the blending modes
 */
TEST_F(PropertyTest, BlendMode) {
    ASSERT_TRUE(CheckBimap(sBlendModeBimap,
                           {"normal", "multiply", "screen", "overlay", "darken", "lighten", "color-dodge",
                            "color-burn", "hard-light", "soft-light", "difference", "exclusion", "hue",
                            "saturation", "color", "luminosity"},
                           {"colorDodge", "colorBurn", "hardLight", "softLight"})
    );
}
