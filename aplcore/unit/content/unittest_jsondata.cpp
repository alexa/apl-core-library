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
#include "apl/apl.h"

using namespace apl;

struct JsonDataTestCase {
    const char *original;
    const char *expected;
    bool isvalid;
    int offset;
};

static auto sTestCases = std::vector<JsonDataTestCase>{
    { "", "", false, 0},
    { "123", "123", true, 0 },
    { "{  }", "{}", true, 0 },
    { "[2,3", "", false, 4 },
};

/*
 * Constructing by value reference is a special case because the original document
 * might have an error when parsing.  But that's okay; the value reference is just
 * to whatever the document was last set to.  When you create a new document it gets
 * set to "null", so that's what we expect to see.
 */
TEST(JsonData, ValueReference)
{
    for (const auto& m : sTestCases) {
        rapidjson::Document doc;
        auto data = JsonData(doc.Parse(m.original));

        ASSERT_TRUE(data);     // Always valid, even if the parse failed
        ASSERT_STREQ(m.isvalid ? m.expected : "null", data.toString().c_str());
        ASSERT_STREQ("Value-constructed; no error", data.error());
        ASSERT_EQ(0, data.offset());
    }
}

TEST(JsonData, MoveDocument)
{
    for (const auto& m : sTestCases) {
        rapidjson::Document doc;
        doc.Parse(m.original);
        auto data = JsonData(std::move(doc));
        ASSERT_EQ(m.isvalid, (bool) data);
        if (m.isvalid) {
            ASSERT_STREQ(m.expected, data.toString().c_str());
        }
        else {
            ASSERT_GT(strlen(data.error()), 0);
            ASSERT_EQ(m.offset, data.offset());
            ASSERT_STREQ("INVALID", data.toString().c_str());
        }
    }
}

TEST(JsonData, MoveFromSharedJson)
{
    for (const auto& m : sTestCases) {
        auto doc = std::make_shared<rapidjson::Document>();
        doc->Parse(m.original);
        auto data = JsonData(SharedJsonData(doc));
        ASSERT_EQ(m.isvalid, (bool) data);
        if (m.isvalid) {
            ASSERT_STREQ(m.expected, data.toString().c_str());
        }
        else {
            ASSERT_EQ(m.offset, data.offset());
            ASSERT_STREQ("INVALID", data.toString().c_str());
        }
    }
}

TEST(JsonData, CopyFromSharedJson)
{
    for (const auto& m : sTestCases) {
        auto doc = std::make_shared<rapidjson::Document>();
        doc->Parse(m.original);
        SharedJsonData sjson(doc); 
        auto data = JsonData(sjson);
        ASSERT_EQ(m.isvalid, (bool) data);
        if (m.isvalid) {
            ASSERT_STREQ(m.expected, data.toString().c_str());
        }
        else {
            ASSERT_EQ(m.offset, data.offset());
            ASSERT_STREQ("INVALID", data.toString().c_str());
        }
    }
}

TEST(JsonData, StdString)
{
    for (const auto& m : sTestCases) {
        auto data = JsonData(std::string(m.original));
        ASSERT_EQ(m.isvalid, (bool)data);
        if (m.isvalid) {
            ASSERT_STREQ(m.expected, data.toString().c_str());
        }
        else {
            ASSERT_EQ(m.offset, data.offset());
            ASSERT_STREQ("INVALID", data.toString().c_str());
        }
    }
}

TEST(JsonData, ConstChar)
{
    for (const auto& m : sTestCases) {
        auto data = JsonData(m.original);
        ASSERT_EQ(m.isvalid, (bool)data);
        if (m.isvalid) {
            ASSERT_STREQ(m.expected, data.toString().c_str());
        }
        else {
            ASSERT_EQ(m.offset, data.offset());
            ASSERT_STREQ("INVALID", data.toString().c_str());
        }
    }
}

TEST(JsonData, Char)
{
    for (const auto& m : sTestCases) {
        char tmp[100];
        strncpy(tmp, m.original, 100);

        auto data = JsonData(tmp);
        ASSERT_EQ(m.isvalid, (bool)data);
        if (m.isvalid) {
            ASSERT_STREQ(m.expected, data.toString().c_str());
        }
        else {
            ASSERT_GT(strlen(data.error()), 0);
            ASSERT_EQ(m.offset, data.offset());
            ASSERT_STREQ("INVALID", data.toString().c_str());
        }
    }
}

TEST(JsonData, NullPointer)
{
    auto data = JsonData((char *) nullptr);
    ASSERT_FALSE(data);
    ASSERT_STREQ("INVALID", data.toString().c_str());
    ASSERT_STREQ("Nullptr", data.error());

    data = JsonData((const char *)nullptr);
    ASSERT_FALSE(data);
    ASSERT_STREQ("INVALID", data.toString().c_str());
    ASSERT_STREQ("Nullptr", data.error());

    data = JsonData("{}");
    auto _ = data.moveToObject();
    ASSERT_FALSE(data);
    ASSERT_STREQ("INVALID", data.toString().c_str());
    ASSERT_STREQ("Nullptr", data.error());
    ASSERT_EQ(0, data.offset());

    auto nullObj = data.moveToObject();
    ASSERT_TRUE(nullObj.isNull());
}