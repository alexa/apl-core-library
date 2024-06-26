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
#include "apl/content/sharedjsondata.h"

using namespace apl;

static const char * TEST_JSON_DATA = R"apl({
  "A": {
    "B": "c"
  }
})apl";

TEST(SharedJsonData, CreateFromDocument)
{
    std::shared_ptr<SharedJsonData> data;
    {
        rapidjson::Document doc;
        doc.Parse(TEST_JSON_DATA);
        data = std::make_shared<SharedJsonData>(std::move(doc));
    }

    ASSERT_TRUE(*data);
    ASSERT_STREQ(R"({"A":{"B":"c"}})", data->toString().c_str());
}

TEST(SharedJsonData, CreateFromSharedDocument)
{
    std::shared_ptr<SharedJsonData> data;
    {
        auto doc = std::make_shared<rapidjson::Document>();
        doc->Parse(TEST_JSON_DATA);
        data = std::make_shared<SharedJsonData>(doc);
    }

    ASSERT_TRUE(*data);
    ASSERT_STREQ(R"({"A":{"B":"c"}})", data->toString().c_str());
}

TEST(SharedJsonData, CreateFromDocumentWithPath)
{
    std::shared_ptr<SharedJsonData> data;
    {
        auto doc = std::make_shared<rapidjson::Document>();
        doc->Parse(TEST_JSON_DATA);
        rapidjson::Pointer ptr("/A");
        data = std::make_shared<SharedJsonData>(doc, ptr);
    }

    ASSERT_TRUE(*data);
    ASSERT_STREQ(R"({"B":"c"})", data->toString().c_str());
}

TEST(SharedJsonData, CreateFromDocumentWithInvalidPointer)
{
    std::shared_ptr<SharedJsonData> data;
    {
        auto doc = std::make_shared<rapidjson::Document>();
        doc->Parse(TEST_JSON_DATA);
        rapidjson::Pointer ptr("X/");
        data = std::make_shared<SharedJsonData>(doc, ptr);
    }

    ASSERT_FALSE(*data);
    ASSERT_STREQ("INVALID", data->toString().c_str());
    ASSERT_STREQ("Bad rapidjson::Pointer: Code 1 at 0", data->error());
}

TEST(SharedJsonData, CreateFromDocumentWithInvalidPath)
{
    std::shared_ptr<SharedJsonData> data;
    {
        auto doc = std::make_shared<rapidjson::Document>();
        doc->Parse(TEST_JSON_DATA);
        rapidjson::Pointer ptr("/X");
        data = std::make_shared<SharedJsonData>(doc, ptr);
    }

    ASSERT_FALSE(*data);
    ASSERT_STREQ("INVALID", data->toString().c_str());
    ASSERT_STREQ("Invalid pointer path: /X", data->error());
}

TEST(SharedJsonData, CreateFromString)
{
    std::shared_ptr<SharedJsonData> data;
    {
        std::string s = TEST_JSON_DATA;
        data = std::make_shared<SharedJsonData>(s);
    }

    ASSERT_TRUE(*data);
    ASSERT_STREQ(R"({"A":{"B":"c"}})", data->toString().c_str());
}

TEST(SharedJsonData, CreateFromCString)
{
    std::shared_ptr<SharedJsonData> data;
    {
        data = std::make_shared<SharedJsonData>(TEST_JSON_DATA);
    }

    ASSERT_TRUE(*data);
    ASSERT_STREQ(R"({"A":{"B":"c"}})", data->toString().c_str());
}
