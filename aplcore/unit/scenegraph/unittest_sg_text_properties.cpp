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
#include "test_sg.h"

#include "apl/scenegraph/textpropertiescache.h"

using namespace apl;

class SGTextPropertiesTest : public ::testing::Test {
};

TEST_F(SGTextPropertiesTest, Basic)
{
    sg::TextPropertiesCache cache;

    auto tp = sg::TextProperties::create(cache,
                                      {"Arial", "Helvetica"},
                                        22.0f,
                                        FontStyle::kFontStyleNormal,
                                        "en-US",
                                        900);

    rapidjson::Document doc;
    auto value = tp->serialize(doc.GetAllocator());

    ASSERT_TRUE(IsEqual(value, StringToMapObject(R"apl(
        {
            "fontFamily": ["Arial", "Helvetica"],
            "fontSize": 22,
            "fontStyle": "normal",
            "lang": "en-US",
            "fontWeight": 900,
            "letterSpacing": 0,
            "lineHeight": 1.25,
            "maxLines": 0,
            "textAlign": "auto",
            "textAlignVertical": "auto"
        }
    )apl")));

    ASSERT_EQ(1, cache.size());
}

TEST_F(SGTextPropertiesTest, Duplicate)
{
    sg::TextPropertiesCache cache;

    auto tp = sg::TextProperties::create(cache,
                                         {"Arial", "Helvetica"},
                                         22.0f,
                                         FontStyle::kFontStyleNormal,
                                         "en-US",
                                         900);

    ASSERT_EQ(1, cache.size());

    auto tp2 = sg::TextProperties::create(cache,
                                          {"Arial", "Helvetica"},
                                          22.0f,
                                          FontStyle::kFontStyleNormal,
                                          "en-US",
                                          900,
                                          0, // Letter spacing
                                          1.25f // Line height
    );

    ASSERT_EQ(1, cache.size());
    ASSERT_EQ(*tp, *tp2);
    ASSERT_EQ(tp.get(), tp2.get());
}

TEST_F(SGTextPropertiesTest, NotDuplicate)
{
    sg::TextPropertiesCache cache;

    auto tp = sg::TextProperties::create(cache,
                                         {"Arial"},
                                         22.0f,
                                         FontStyle::kFontStyleNormal,
                                         "en-US",
                                         900);

    ASSERT_EQ(1, cache.size());

    auto tp2 = sg::TextProperties::create(cache,
                                          {"Arial", "Helvetica"},
                                          22.0f,
                                          FontStyle::kFontStyleNormal,
                                          "en-US",
                                          900,
                                          0, // Letter spacing
                                          1.25f // Line height
    );

    ASSERT_EQ(2, cache.size());
    ASSERT_NE(*tp, *tp2);
    ASSERT_NE(tp.get(), tp2.get());
}