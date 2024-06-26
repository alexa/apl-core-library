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
#include "apl/versioning/semanticversion.h"

using namespace apl;

class SemanticVersionTest : public MemoryWrapper {};

TEST_F(SemanticVersionTest, Basic) {
    auto test = SemanticVersion::create(session, "1.3.0");
    ASSERT_TRUE(test);
    ASSERT_EQ("1.3.0", test->toDebugString());
}

static const char * SV_GOOD[] = {
        "1",
        "1.3.12",
        "0.0.4",
        "    1.1    ",
        "1.3.12  ",
        "23.124.0",
        "1.2.3-alpha",
        "1.2.3-alpha.2+32423",
        "0.0.1-alpha-beta-gamma.-.02",
        "1.2.3-2147483647",  // 2^31 - 1
        "1.0.0-1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0",
        // The last entry has 255 characters, which just fits
        "1.0.0"   // 5 characters
        "-1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0"  // 80 characters
        ".1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0"  // 80 characters
        ".1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0"  // 80 characters
        ".1.2.3.4.5"  // 10 characters
};

TEST_F(SemanticVersionTest, Good) {
    for (const auto& m : SV_GOOD) {
        auto a = SemanticVersion::create(session, m);
        ASSERT_TRUE(a) << m;
    }
}

static const char * SV_BAD[] = {
        "",
        "v2.2",
        "+hello",
        "1.2.1+hello?",
        "-23.124.0",
        "1.2.3-alpha%",  // Trailing invalid character '%'
        "1.2.3-alpha.2+32423-..234",  // The ".." is invalid; there should be something in between
        "1-2147483648",  // 2^32 doesn't fit.
        // The last entry has 256 characters, which is too long
        "1.0.10"   // 6 characters
        "-1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0"  // 80 characters
        ".1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0"  // 80 characters
        ".1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0"  // 80 characters
        ".1.2.3.4.5"  // 10 characters
};

TEST_F(SemanticVersionTest, Bad) {
    for (const auto& m : SV_BAD) {
        auto a = SemanticVersion::create(session, m);
        ASSERT_FALSE(a) << m;
        ASSERT_TRUE(ConsoleMessage()) << m;
    }
}

static const std::vector<std::string> SV_ORDERED = {
        "1.0.0-alpha",
        "1.0.0-alpha.1",
        "1.0-alpha.2",
        "1.0.0-alpha.beta",
        "1.0.0-beta",
        "1.0.0-beta.2",
        "1.0.0-beta.11",
        "1.0.0-rc.1",
        "1.0.0",
        "2.0.0-alpha",
        "2.0.0",
        "2.1.0",
        "2.1.1",
        "2.2.0",
        "2.12.3",
        "3-beta",
        "3",
        "3.1-ALPHA.1",
        "3.1.0-ALPHA.2",
        "3.1-ALPHA.BETA",
        "3.1",
        "3.1.1-ALPHA",
        "3.1.1",
        "4-0",
        "4-4",
        "4-1235",
        "4-00000",   // Numerics are less than strings
        "11"
};

TEST_F(SemanticVersionTest, Ordered) {
    for (auto i = 0 ; i < SV_ORDERED.size() - 1 ; i++) {
        const auto& sa = SV_ORDERED.at(i);
        const auto& sb = SV_ORDERED.at(i+1);
        auto a = SemanticVersion::create(session, sa);
        auto b = SemanticVersion::create(session, sb);

        ASSERT_TRUE(a) << sa;
        ASSERT_TRUE(b) << sb;
        ASSERT_TRUE(*a < *b) << sa << " < " << sb;
        ASSERT_TRUE(*b > *a) << sb << " > " << sa;
        ASSERT_TRUE(*a != *b) << sa << " != " << sb;
        ASSERT_FALSE(*a == *b) << sa << " == " << sb;
        ASSERT_TRUE(*a <= *b) << sa << " <= " << sb;
        ASSERT_TRUE(*b >= *a) << sb << " >= " << sa;

        ASSERT_FALSE(*a < *a) << sa;
        ASSERT_FALSE(*a > *a) << sa;
        ASSERT_FALSE(*a != *a) << sa;
        ASSERT_TRUE(*a == *a) << sa;
        ASSERT_TRUE(*a <= *a) << sa;
        ASSERT_TRUE(*a >= *a) << sa;
    }
}

static const std::vector<std::pair<std::string, std::string>> SV_DEBUG_STRING_TEST = {
        { "1", "1.0.0" },
        { "2.12", "2.12.0" },
        { "13.0.33", "13.0.33" },
        { "1-a-2", "1.0.0.'a-2'" },
        { "2-a.b-3.234.0.02", "2.0.0.'a'.'b-3'.234.0.'02'" },
        { "0+423.a", "0.0.0" },
};

TEST_F(SemanticVersionTest, DebugString)
{
    for (const auto& m : SV_DEBUG_STRING_TEST) {
        auto v = SemanticVersion::create(session, m.first);
        ASSERT_TRUE(v) << m.first;
        ASSERT_EQ(m.second, v->toDebugString()) << m.first;
    }
}
