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
#include "apl/content/importrequest.h"

using namespace apl;

class PathTest : public DocumentWrapper {
public:
    void checkPaths(const char *filename, const std::map<std::string, std::string>& map) {
        loadDocument(filename);
        checkPaths(map);
    }

    void checkPaths(const std::map<std::string, std::string>& map) {
        for (auto& kv : map) {
            auto component = context->findComponentById(kv.first);
            if (kv.second.empty()) {
                ASSERT_FALSE(component) << kv.first;
            } else {
                ASSERT_TRUE(component) << kv.first;
                ASSERT_EQ(kv.second, component->getPath()) << kv.first;
            }
        }
    }
};

static const char * BASIC_USING_ITEMS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Text\","
    "          \"id\": \"text1\""
    "        },"
    "        {"
    "          \"type\": \"Text\","
    "          \"id\": \"text2\""
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(PathTest, BasicUsingItems)
{
    loadDocument(BASIC_USING_ITEMS);

    auto text1 = context->findComponentById("text1");
    ASSERT_TRUE(text1);

    auto text2 = context->findComponentById("text2");
    ASSERT_TRUE(text2);

    ASSERT_STREQ("_main/mainTemplate/items", component->getPath().c_str());
    ASSERT_STREQ("_main/mainTemplate/items/items/0", text1->getPath().c_str());
    ASSERT_STREQ("_main/mainTemplate/items/items/1", text2->getPath().c_str());

    // Sanity check that path actually matches rapidjson Pointer implementation
    ASSERT_STREQ(followPath(text1->getPath())->GetObject()["id"].GetString(), "text1");
}

static const char * BASIC_USING_ITEM =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"item\": ["
    "        {"
    "          \"type\": \"Text\","
    "          \"id\": \"text1\""
    "        },"
    "        {"
    "          \"type\": \"Text\","
    "          \"id\": \"text2\""
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(PathTest, BasicUsingItem)
{
    loadDocument(BASIC_USING_ITEM);

    auto text1 = context->findComponentById("text1");
    ASSERT_TRUE(text1);

    auto text2 = context->findComponentById("text2");
    ASSERT_TRUE(text2);

    ASSERT_STREQ("_main/mainTemplate/item", component->getPath().c_str());
    ASSERT_STREQ("_main/mainTemplate/item/item/0", text1->getPath().c_str());
    ASSERT_STREQ("_main/mainTemplate/item/item/1", text2->getPath().c_str());
}

static const char * CONDITIONAL_LIST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Text\","
    "          \"id\": \"text1\""
    "        },"
    "        {"
    "          \"when\": false,"
    "          \"type\": \"Text\","
    "          \"id\": \"text2\""
    "        },"
    "        {"
    "          \"type\": \"Text\","
    "          \"id\": \"text3\""
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(PathTest, ConditionalList)
{
    loadDocument(CONDITIONAL_LIST);

    auto text1 = context->findComponentById("text1");
    ASSERT_TRUE(text1);

    auto text2 = context->findComponentById("text2");
    ASSERT_FALSE(text2);

    auto text3 = context->findComponentById("text3");
    ASSERT_TRUE(text3);

    ASSERT_STREQ("_main/mainTemplate/items", component->getPath().c_str());
    ASSERT_STREQ("_main/mainTemplate/items/items/0", text1->getPath().c_str());
    ASSERT_STREQ("_main/mainTemplate/items/items/2", text3->getPath().c_str());
}

static const char *NESTING =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": ["
    "      {"
    "        \"type\": \"Container\","
    "        \"id\": \"container1\","
    "        \"when\": false"
    "      },"
    "      {"
    "        \"type\": \"Container\","
    "        \"id\": \"container2\","
    "        \"items\": ["
    "          {"
    "            \"type\": \"Frame\","
    "            \"id\": \"frame1\","
    "            \"items\": ["
    "              {"
    "                \"type\": \"Text\","
    "                \"id\": \"text1\","
    "                \"when\": false"
    "              },"
    "              {"
    "                \"type\": \"Text\","
    "                \"id\": \"text2\""
    "              }"
    "            ]"
    "          },"
    "          {"
    "            \"when\": false,"
    "            \"type\": \"Text\","
    "            \"id\": \"text3\""
    "          },"
    "          {"
    "            \"type\": \"Text\","
    "            \"id\": \"text4\""
    "          }"
    "        ]"
    "      }"
    "    ]"
    "  }"
    "}";

TEST_F(PathTest, Nesting)
{
    checkPaths(NESTING, {
        { "container1", ""},
        { "container2", "_main/mainTemplate/items/1"},
        { "frame1", "_main/mainTemplate/items/1/items/0"},
        { "text1", ""},
        { "text2", "_main/mainTemplate/items/1/items/0/items/1"},
        { "text3", ""},
        { "text4", "_main/mainTemplate/items/1/items/2"}
    });
}

static const char *FIRST_LAST_ITEM =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": ["
    "      {"
    "        \"type\": \"Container\","
    "        \"firstItem\": {"
    "          \"type\": \"Text\","
    "          \"id\": \"text1\""
    "        },"
    "        \"lastItem\": {"
    "          \"type\": \"Text\","
    "          \"id\": \"text2\""
    "        },"
    "        \"items\": ["
    "          {"
    "            \"type\": \"Text\","
    "            \"id\": \"text3\""
    "          },"
    "          {"
    "            \"type\": \"Text\","
    "            \"id\": \"text4\""
    "          }"
    "        ]"
    "      }"
    "    ]"
    "  }"
    "}";

TEST_F(PathTest, FirstLast)
{
    checkPaths(FIRST_LAST_ITEM, {
        { "text1", "_main/mainTemplate/items/0/firstItem"},
        { "text2", "_main/mainTemplate/items/0/lastItem"},
        { "text3", "_main/mainTemplate/items/0/items/0"},
        { "text4", "_main/mainTemplate/items/0/items/1"}
    });
}

static const char *DATA_SEQUENCE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": ["
    "      {"
    "        \"type\": \"Container\","
    "        \"firstItem\": {"
    "          \"type\": \"Text\","
    "          \"id\": \"text1\""
    "        },"
    "        \"lastItem\": {"
    "          \"type\": \"Text\","
    "          \"id\": \"text2\""
    "        },"
    "        \"items\": ["
    "          {"
    "            \"type\": \"Text\","
    "            \"id\": \"text3_${data}\","
    "            \"when\": \"${data%2}\""
    "          },"
    "          {"
    "            \"type\": \"Text\","
    "            \"id\": \"text4_${data}\""
    "          }"
    "        ],"
    "        \"data\": ["
    "          1,"
    "          2,"
    "          3,"
    "          4,"
    "          5,"
    "          6"
    "        ]"
    "      }"
    "    ]"
    "  }"
    "}";

TEST_F(PathTest, DataSequence)
{
    checkPaths(DATA_SEQUENCE, {
        { "text1", "_main/mainTemplate/items/0/firstItem"},
        { "text2", "_main/mainTemplate/items/0/lastItem"},
        { "text3_1", "_main/mainTemplate/items/0/items/0"},
        { "text3_2", ""},
        { "text3_3", "_main/mainTemplate/items/0/items/0"},
        { "text3_4", ""},
        { "text3_5", "_main/mainTemplate/items/0/items/0"},
        { "text3_6", ""},
        { "text4_1", ""},
        { "text4_2", "_main/mainTemplate/items/0/items/1"},
        { "text4_3", ""},
        { "text4_4", "_main/mainTemplate/items/0/items/1"},
        { "text4_5", ""},
        { "text4_6", "_main/mainTemplate/items/0/items/1"},
    });
}

static const char *CONDITIONAL_FRAME =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": ["
    "      {"
    "        \"type\": \"Frame\","
    "        \"item\": ["
    "          {"
    "            \"type\": \"Text\","
    "            \"id\": \"text1\","
    "            \"when\": false"
    "          },"
    "          {"
    "            \"type\": \"Text\","
    "            \"id\": \"text2\""
    "          }"
    "        ]"
    "      }"
    "    ]"
    "  }"
    "}";

TEST_F(PathTest, ConditionalFrame)
{
    checkPaths(CONDITIONAL_FRAME, {
        {"text1", ""},
        {"text2", "_main/mainTemplate/items/0/item/1"}
    });
}

static const char *LAYOUT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"layouts\": {"
    "    \"header\": {"
    "      \"description\": \"Fake header\","
    "      \"parameters\": ["
    "        \"title\","
    "        \"subtitle\""
    "      ],"
    "      \"items\": {"
    "        \"type\": \"Container\","
    "        \"items\": ["
    "          {"
    "            \"type\": \"Text\","
    "            \"id\": \"title\","
    "            \"text\": \"${title}\""
    "          },"
    "          {"
    "            \"type\": \"Text\","
    "            \"id\": \"subtitle\","
    "            \"text\": \"${subtitle}\""
    "          }"
    "        ]"
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": ["
    "      {"
    "        \"type\": \"Container\","
    "        \"id\": \"container1\","
    "        \"items\": ["
    "          {"
    "            \"type\": \"header\","
    "            \"id\": \"headerId\","
    "            \"title\": \"Dogs\","
    "            \"subtitle\": \"Our canine friends\""
    "          },"
    "          {"
    "            \"type\": \"Image\","
    "            \"id\": \"dogPicture\""
    "          }"
    "        ]"
    "      }"
    "    ]"
    "  }"
    "}";

TEST_F(PathTest, Layout)
{
    checkPaths(LAYOUT, {
        {"container1", "_main/mainTemplate/items/0"},
        {"headerId", "_main/layouts/header/items"},
        {"title", "_main/layouts/header/items/items/0"},
        {"subtitle", "_main/layouts/header/items/items/1"},
        {"dogPicture", "_main/mainTemplate/items/0/items/1"},
    });
}

static const char *LAYOUT_WITH_DATA =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"layouts\": {"
    "    \"ListItem\": {"
    "      \"parameters\": ["
    "        \"title\","
    "        \"subtitle\""
    "      ],"
    "      \"items\": {"
    "        \"type\": \"Container\","
    "        \"id\": \"Container${index}\","
    "        \"bind\": {"
    "          \"name\": \"cindex\","
    "          \"value\": \"${index}\""
    "        },"
    "        \"items\": ["
    "          {"
    "            \"type\": \"Text\","
    "            \"text\": \"${title}\","
    "            \"id\": \"Title${cindex}\""
    "          },"
    "          {"
    "            \"type\": \"Text\","
    "            \"test\": \"${subtitle}\","
    "            \"id\": \"Subtitle${cindex}\""
    "          }"
    "        ]"
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"id\": \"Sequence1\","
    "      \"items\": {"
    "        \"type\": \"ListItem\","
    "        \"title\": \"Title for ${data}\","
    "        \"subtitle\": \"Subtitle for ${data}\""
    "      },"
    "      \"data\": ["
    "        \"alpha\","
    "        \"bravo\""
    "      ]"
    "    }"
    "  }"
    "}";


TEST_F(PathTest, LayoutWithData)
{
    checkPaths(LAYOUT_WITH_DATA, {
        {"Sequence1", "_main/mainTemplate/items"},
        {"Container0", "_main/layouts/ListItem/items"},
        {"Title0", "_main/layouts/ListItem/items/items/0"},
        {"Subtitle0", "_main/layouts/ListItem/items/items/1"},
        {"Container1", "_main/layouts/ListItem/items"},
        {"Title1", "_main/layouts/ListItem/items/items/0"},
        {"Subtitle1", "_main/layouts/ListItem/items/items/1"},
    });
}

static const char *LAYOUT_WITH_DATA_2 =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"layouts\": {"
    "    \"HorizontalListItem\": {"
    "      \"item\": ["
    "        {"
    "          \"type\": \"Container\","
    "          \"id\": \"ItemContainer${index}\","
    "          \"bind\": {"
    "            \"name\": \"cindex\","
    "            \"value\": \"${index}\""
    "          },"
    "          \"items\": ["
    "            {"
    "              \"type\": \"Image\","
    "              \"id\": \"ItemImage${cindex}\","
    "              \"source\": \"${data.image}\""
    "            },"
    "            {"
    "              \"type\": \"Text\","
    "              \"id\": \"ItemPrimaryText${cindex}\","
    "              \"text\": \"<b>${ordinal}.</b> ${data.primaryText}\""
    "            },"
    "            {"
    "              \"type\": \"Text\","
    "              \"id\": \"ItemSecondaryText${cindex}\","
    "              \"text\": \"${data.secondaryText}\""
    "            }"
    "          ]"
    "        }"
    "      ]"
    "    },"
    "    \"ListTemplate2\": {"
    "      \"parameters\": ["
    "        \"backgroundImage\","
    "        \"listData\""
    "      ],"
    "      \"items\": ["
    "        {"
    "          \"type\": \"Container\","
    "          \"id\": \"TopContainer\","
    "          \"items\": ["
    "            {"
    "              \"type\": \"Image\","
    "              \"id\": \"BackgroundImage\","
    "              \"source\": \"${backgroundImage}\""
    "            },"
    "            {"
    "              \"type\": \"Sequence\","
    "              \"id\": \"MasterSequence\","
    "              \"scrollDirection\": \"horizontal\","
    "              \"data\": \"${listData}\","
    "              \"numbered\": true,"
    "              \"item\": ["
    "                {"
    "                  \"type\": \"HorizontalListItem\""
    "                }"
    "              ]"
    "            }"
    "          ]"
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"item\": ["
    "      {"
    "        \"type\": \"ListTemplate2\","
    "        \"backgroundImage\": \"foo\","
    "        \"listData\": ["
    "          {"
    "            \"image\": \"IMAGE1\","
    "            \"primaryText\": \"PRIMARY1\","
    "            \"secondaryText\": \"SECONDARY1\""
    "          },"
    "          {"
    "            \"image\": \"IMAGE1\","
    "            \"primaryText\": \"PRIMARY1\","
    "            \"secondaryText\": \"SECONDARY1\""
    "          }"
    "        ]"
    "      }"
    "    ]"
    "  }"
    "}";


TEST_F(PathTest, LayoutWithData2)
{
    checkPaths(LAYOUT_WITH_DATA_2, {
        {"TopContainer", "_main/layouts/ListTemplate2/items/0"},
        {"BackgroundImage", "_main/layouts/ListTemplate2/items/0/items/0"},
        {"MasterSequence", "_main/layouts/ListTemplate2/items/0/items/1"},
        {"ItemContainer0", "_main/layouts/HorizontalListItem/item/0"},
        {"ItemImage0", "_main/layouts/HorizontalListItem/item/0/items/0"},
        {"ItemPrimaryText0", "_main/layouts/HorizontalListItem/item/0/items/1"},
        {"ItemSecondaryText0", "_main/layouts/HorizontalListItem/item/0/items/2"},
        {"ItemContainer1", "_main/layouts/HorizontalListItem/item/0"},
        {"ItemImage1", "_main/layouts/HorizontalListItem/item/0/items/0"},
        {"ItemPrimaryText1", "_main/layouts/HorizontalListItem/item/0/items/1"},
        {"ItemSecondaryText1", "_main/layouts/HorizontalListItem/item/0/items/2"},
    });
}

static const char *DOCUMENT_WITH_IMPORT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"import\": ["
    "    {"
    "      \"name\": \"base\","
    "      \"version\": \"1.2\""
    "    }"
    "  ],"
    "  \"resources\": ["
    "    {"
    "      \"strings\": {"
    "        \"firstname\": \"Pebbles\""
    "      }"
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"items\": ["
    "      {"
    "        \"type\": \"Container\","
    "        \"items\": ["
    "          {"
    "            \"type\": \"Header\","
    "            \"id\": \"headerId\","
    "            \"title\": \"Dogs\","
    "            \"subtitle\": \"Our canine friends\""
    "          },"
    "          {"
    "            \"type\": \"Image\","
    "            \"id\": \"dogPicture\""
    "          }"
    "        ]"
    "      }"
    "    ]"
    "  }"
    "}";

static const char *BASE_PACKAGE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"resources\": ["
    "    {"
    "      \"strings\": {"
    "        \"firstname\": \"Fred\","
    "        \"lastname\": \"Flintstone\""
    "      }"
    "    }"
    "  ],"
    "  \"layouts\": {"
    "    \"Header\": {"
    "      \"parameters\": ["
    "        \"title\","
    "        \"subtitle\""
    "      ],"
    "      \"item\": {"
    "        \"type\": \"Container\","
    "        \"items\": ["
    "          {"
    "            \"type\": \"Text\","
    "            \"id\": \"title\","
    "            \"text\": \"${title}\""
    "          },"
    "          {"
    "            \"type\": \"Text\","
    "            \"id\": \"subtitle\","
    "            \"text\": \"${subtitle}\""
    "          }"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(PathTest, DocumentWithImport)
{
    content = Content::create(DOCUMENT_WITH_IMPORT, makeDefaultSession());
    ASSERT_TRUE(content->isWaiting());
    auto requested = content->getRequestedPackages();
    auto it = requested.begin();
    content->addPackage(*it, BASE_PACKAGE);

    inflate();

    checkPaths({
        {"headerId", "base:1.2/layouts/Header/item"},
        {"title", "base:1.2/layouts/Header/item/items/0"},
        {"subtitle", "base:1.2/layouts/Header/item/items/1"},
        {"dogPicture", "_main/mainTemplate/items/0/items/1"}
    });

    ASSERT_STREQ("_main/resources/0/strings/firstname", context->provenance("@firstname").c_str());
    ASSERT_STREQ("base:1.2/resources/0/strings/lastname", context->provenance("@lastname").c_str());
}

static const char *HIDDEN_COMPONENT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"imports\": ["
    "    {"
    "      \"name\": \"base\","
    "      \"version\": \"1.2\""
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"items\": ["
    "      {"
    "        \"type\": \"Frame\","
    "        \"bind\": {"
    "          \"name\": \"foo\","
    "          \"value\": {"
    "            \"type\": \"Text\","
    "            \"id\": \"hiddenText\""
    "          }"
    "        },"
    "        \"items\": \"${foo}\""
    "      }"
    "    ]"
    "  }"
    "}";

TEST_F(PathTest, HiddenComponent)
{
    loadDocument(HIDDEN_COMPONENT);

    ASSERT_EQ(kComponentTypeFrame, component->getType());
    ASSERT_EQ(1, component->getChildCount());
    auto child = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeText, child->getType());
    ASSERT_EQ(child, context->findComponentById("hiddenText"));

    ASSERT_EQ(std::string("_main/mainTemplate/items/0"), component->getPath());

    // TODO: This is not a real path because of the data-bound component definition.  Fix this.
    ASSERT_EQ(std::string("_main/mainTemplate/items/0/items"), child->getPath());
}
