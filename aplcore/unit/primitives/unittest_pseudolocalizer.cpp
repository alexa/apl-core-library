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
#include "gtest/gtest.h"

#include "apl/primitives/pseudolocalizer.h"

using namespace apl;

class PseudoLocalizeTest : public ::testing::Test {
public:
    PseudoLocalizeTest() {
        textTransformer = std::make_shared<PseudoLocalizationTextTransformer>();
    }

    std::shared_ptr<TextTransformer> textTransformer;
};

static const std::vector<std::string> PSEUDO_TEST_CASES = {"Hello World", "Testing", "Random",
                                                           "String", ""};
static const std::vector<std::string> PSEUDO_TEST_CASES_RESPONSE_70_EXPANSION = {
    "[--Ħḗḗŀŀǿǿ Ẇǿǿřŀḓ--]", "[-Ŧḗḗşŧīīƞɠ-]", "[-Řȧȧƞḓǿǿḿ-]", "[-Şŧřīīƞɠ--]", "[]"};
static const std::vector<std::string> PSEUDO_TEST_CASES_RESPONSE_DEFAULT_EXPANSION = {
    "[Ħḗḗŀŀǿǿ Ẇǿǿřŀḓ]", "[Ŧḗḗşŧīīƞɠ]", "[Řȧȧƞḓǿḿ]", "[Şŧřīīƞɠ]", "[]"};

TEST_F(PseudoLocalizeTest, transform_HappyCase) {

    auto cnt = 0;
    for (const std::string& input : PSEUDO_TEST_CASES) {
        auto props =
            std::make_shared<ObjectMap>(ObjectMap{{"enabled", true}, {"expansionPercentage", 70}});
        std::string result = textTransformer->transform(input, props);

        // Assert that the string has been transformed.
        EXPECT_EQ(result, PSEUDO_TEST_CASES_RESPONSE_70_EXPANSION.at(cnt++));
    }
}

TEST_F(PseudoLocalizeTest, transform_SuppledExpansionPercentagelessThan0_DefaultExpansionFactor) {
    auto cnt = 0;
    for (const std::string& input : PSEUDO_TEST_CASES) {
        auto props =
            std::make_shared<ObjectMap>(ObjectMap{{"enabled", true}, {"expansionPercentage", -1}});
        std::string result = textTransformer->transform(input, props);

        // Assert that the length of the result is greater than input
        EXPECT_EQ(result, PSEUDO_TEST_CASES_RESPONSE_DEFAULT_EXPANSION.at(cnt++));
    }
}

TEST_F(PseudoLocalizeTest, transform_SuppledExpansionPercentage0_NoExpansion) {
    auto props =
        std::make_shared<ObjectMap>(ObjectMap{{"enabled", true}, {"expansionPercentage", 0}});
    std::string result = textTransformer->transform("input", props);

    // Assert that the length of the result is greater than input
    EXPECT_EQ(result, "[īƞƥŭŧ]");
}

TEST_F(PseudoLocalizeTest, transform_SuppledExpansionPercentageMoreThan100_DefaultExpansionFactor) {
    auto cnt = 0;
    for (const std::string& input : PSEUDO_TEST_CASES) {
        auto props =
            std::make_shared<ObjectMap>(ObjectMap{{"enabled", true}, {"expansionPercentage", 101}});
        std::string result = textTransformer->transform(input, props);

        // Assert that the string is expanded by default expansion percentage
        EXPECT_EQ(result, PSEUDO_TEST_CASES_RESPONSE_DEFAULT_EXPANSION.at(cnt++));
    }
}

TEST_F(PseudoLocalizeTest, transform_SuppledExpansionPercentageNull_DefaultExpansionFactor) {
    auto cnt = 0;
    for (const std::string& input : PSEUDO_TEST_CASES) {
        auto props = std::make_shared<ObjectMap>(ObjectMap{{"enabled", true}});
        std::string result = textTransformer->transform(input, props);

        // Assert that the string is expanded by default expansion percentage
        EXPECT_EQ(result, PSEUDO_TEST_CASES_RESPONSE_DEFAULT_EXPANSION.at(cnt++));
    }
}

TEST_F(PseudoLocalizeTest, transform_NullSettings) {
    EXPECT_EQ(textTransformer->transform("Hello World", Object::NULL_OBJECT()), "Hello World");
    EXPECT_EQ(textTransformer->transform("", Object::NULL_OBJECT()), "");
}

TEST_F(PseudoLocalizeTest, getPseudoLocalString_Disabled) {
    auto props =
        std::make_shared<ObjectMap>(ObjectMap{{"enabled", false}, {"expansionPercentage", 40}});

    std::string result;
    result = textTransformer->transform("Hello World", props);

    EXPECT_EQ(result, "Hello World");
}

TEST_F(PseudoLocalizeTest, expandString_OddSettingsSuppliedExpansionPercentageNotNumber) {
    auto cnt = 0;
    for (const std::string& input : PSEUDO_TEST_CASES) {
        auto props = std::make_shared<ObjectMap>(
            ObjectMap{{"enabled", true}, {"expansionPercentage", "abc"}});
        std::string result = textTransformer->transform(input, props);
        // Assert that the string is expanded by default expansion percentage
        EXPECT_EQ(result, PSEUDO_TEST_CASES_RESPONSE_DEFAULT_EXPANSION.at(cnt++));
    }
}

TEST_F(PseudoLocalizeTest, getPseudoLocalString_OddSettingsInvalidEnabledValue) {
    auto props =
        std::make_shared<ObjectMap>(ObjectMap{{"enabled", "gh"}, {"expansionPercentage", 40}});
    std::string result = textTransformer->transform("Hello World", props);

    EXPECT_EQ(result, "[Ħḗḗŀŀǿǿ Ẇǿǿřŀḓ-]");
}

TEST_F(PseudoLocalizeTest, getPseudoLocalString_OddSettingsEnabledFlagAbsent) {
    auto props = std::make_shared<ObjectMap>(ObjectMap{{"expansionPercentage", 70}});
    std::string result = textTransformer->transform("Hello World", props);
    EXPECT_EQ(result, "Hello World");
}
