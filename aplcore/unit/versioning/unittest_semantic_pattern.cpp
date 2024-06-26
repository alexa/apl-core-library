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

#include "apl/versioning/semanticpattern.h"
#include "apl/versioning/semanticversion.h"

using namespace apl;

class SemanticPatternTest : public MemoryWrapper {
};

static std::vector<std::pair<std::string, std::string>> GOOD_PATTERNS = {
    { "1", "=1.0.0" },
    { "    1   ", "=1.0.0"},
    { "12.1", "=12.1.0"},
    { "1.3-alpha.v2.12+beta.4444", "=1.3.0.'alpha'.'v2'.12" },
    { ">2.12.4", ">2.12.4"},
    { "<6", "<6.0.0"},
    { "=1.4.2", "=1.4.2"},
    { ">=2+testing", ">=2.0.0"},
    { "<=13-0", "<=13.0.0.0"},
    { "1   ||  2", "=1.0.0 || =2.0.0"},
    { ">=2.3||<3", ">=2.3.0 || <3.0.0"},
    { "1.0.4-beta || 1.0.6-beta || <1.3.2-alpha >1", "=1.0.4.'beta' || =1.0.6.'beta' || <1.3.2.'alpha' >1.0.0"}
};

TEST_F(SemanticPatternTest, Good)
{
    for (const auto& m : GOOD_PATTERNS) {
        auto p = SemanticPattern::create(session, m.first);
        ASSERT_TRUE(p) << m.first;
        ASSERT_EQ(m.second, p->toDebugString()) << m.first;
    }
}

static std::vector<std::string> BAD_PATTERNS = {
        "",
        " b ",
        "> 1.2",
        "1.b.2",
        "!=3.0.5",
        "(>1.2 <2)",
        ">1.2 && < 2",
        ">=1.3.5-alpha.@fuzzy",
};

TEST_F(SemanticPatternTest, Bad)
{
    for (const auto& m : BAD_PATTERNS) {
        auto p = SemanticPattern::create(session, m);
        ASSERT_FALSE(p) << m;
        ASSERT_TRUE(ConsoleMessage()) << m;
    }
}

TEST_F(SemanticPatternTest, PatternBasic)
{
    auto p = SemanticPattern::create(session, ">1.0 <2.0.4");
    ASSERT_TRUE(p);
    ASSERT_TRUE(p->match(SemanticVersion::create(session, "1.1.3")));
    ASSERT_TRUE(p->match(SemanticVersion::create(session, "2.0.3")));
    ASSERT_FALSE(p->match(SemanticVersion::create(session, "1.0.0")));
    ASSERT_FALSE(p->match(SemanticVersion::create(session, "2.0.4")));
    ASSERT_FALSE(p->match(nullptr));
}

TEST_F(SemanticPatternTest, PatternBasic2)
{
    auto p = SemanticPattern::create(session, ">1.0-alpha");
    ASSERT_TRUE(p);
    ASSERT_TRUE(p->match(SemanticVersion::create(session, "1.0-alpha.2")));
    ASSERT_TRUE(p->match(SemanticVersion::create(session, "1.1+testbuild")));
    ASSERT_TRUE(p->match(SemanticVersion::create(session, "1.1")));
}

TEST_F(SemanticPatternTest, PatternBasic3)
{
    auto p = SemanticPattern::create(session, "1 || 1.1");
    ASSERT_TRUE(p);
    ASSERT_TRUE(p->match(SemanticVersion::create(session, "1.0.0")));
    ASSERT_TRUE(p->match(SemanticVersion::create(session, "1.1.0")));
    ASSERT_TRUE(p->match(SemanticVersion::create(session, "1.1.0+testbuild")));
    ASSERT_FALSE(p->match(SemanticVersion::create(session, "1.1.0-b2")));
    ASSERT_FALSE(p->match(nullptr));
}

struct PatternTestCase {
    std::string pattern;
    std::vector<std::string> good;
    std::vector<std::string> bad;
};

static const std::vector<PatternTestCase> SV_PATTERN_TEST = {
        {">1.0",
                {"1.2",   "1.0.1", "32.1"},
                {"1.0", "0.9",         "1.0.3-alpha.1"}},
        {">1.0-alpha",  // Prerelease matters
                {"1.2",   "1.0.1", "32.1",  "1.0", "1.0-alpha.2"},
                {"0.9", "1.0.0-alpha", "1.0.3-alpha.1"}},
        {">1.0+alpha",  // Build should be ignored
                {"1.2",   "1.0.1", "32.1"},
                {"1.0", "0.9",         "1.0.3-alpha.1"}},
        {">=1.0.0 <2.0.0",
                {"1.2",   "1.0.0", "1.0.1", "1.9999.999"},
                {"0.9", "1.0.0-alpha", "1.0.3-alpha.1", "2.0.0", "234.23.222"}},
        {">2.2.3 || 1.2.3",
                {"1.2.3", "2.2.4", "3.1.0"},
                {"1.0", "0.9",         "1.2.3-alpha",   "1.0.3-alpha.1"}},
        {"1.3.2 || 2.0 || 2.0.1",
                {"1.3.2", "2.0.0", "2.0.1"},
                {"1.3", "2.0.2", "2-a"}},
        {"<=2.0.0-alpha",
                {"1.3.2", "0.0.0", "2.0-a", "2-02"},
                {"2", "2.0.0-alpha.2", "2-alpha2"}},
};

TEST_F(SemanticPatternTest, PatternTest)
{
    for (const auto& m: SV_PATTERN_TEST) {
        auto pattern = SemanticPattern::create(session, m.pattern);
        ASSERT_TRUE(pattern) << m.pattern;

        for (const auto& good: m.good) {
            auto v = SemanticVersion::create(session, good);
            ASSERT_TRUE(v) << m.pattern << " " << good;
            ASSERT_TRUE(pattern->match(v)) << m.pattern << " " << good;
        }

        for (const auto& bad: m.bad) {
            auto v = SemanticVersion::create(session, bad);
            ASSERT_TRUE(v) << m.pattern << " " << bad;
            ASSERT_FALSE(pattern->match(v)) << m.pattern << " " << bad;
        }
    }
}