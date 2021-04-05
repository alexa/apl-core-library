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
#include "testdatasourceprovider.h"

using namespace apl;

class DynamicSourceTest : public DocumentWrapper {
protected:
    ::testing::AssertionResult
    CheckChild(const ComponentPtr& comp, size_t idx, const std::string& exp) {
        auto actualText = comp->getChildAt(idx)->getCalculated(kPropertyText).asString();
        if (actualText != exp) {
            return ::testing::AssertionFailure()
                    << "text " << idx
                    << " is wrong. Expected: " << exp
                    << ", actual: " << actualText;
        }

        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    CheckChild(size_t idx, const std::string& exp) {
        return CheckChild(component, idx, exp);
    }
};

static const char *DATA =
        "{"
        "    \"dynamicSource\": {"
        "        \"type\": \"GenericList\","
        "        \"listId\": \"vQdpOESlok\","
        "        \"offset\": 0,"
        "        \"maxItems\": 20,"
        "        \"items\": ["
        "            { \"color\": \"#000000\", \"text\": \"0\" },"
        "            { \"color\": \"#010000\", \"text\": \"1\" },"
        "            { \"color\": \"#020000\", \"text\": \"2\" },"
        "            { \"color\": \"#030000\", \"text\": \"3\" },"
        "            { \"color\": \"#040000\", \"text\": \"4\" }"
        "        ]"
        "    }"
        "}";

static const std::vector<std::string> ITEMS = {
        R"({"color": "#000000", "text": "0"})",
        R"({"color": "#010000", "text": "1"})",
        R"({"color": "#020000", "text": "2"})",
        R"({"color": "#030000", "text": "3"})",
        R"({"color": "#040000", "text": "4"})",
        R"({"color": "#050000", "text": "5"})",
        R"({"color": "#060000", "text": "6"})",
        R"({"color": "#070000", "text": "7"})",
        R"({"color": "#080000", "text": "8"})",
        R"({"color": "#090000", "text": "9"})",
        R"({"color": "#0A0000", "text": "10"})",
        R"({"color": "#0B0000", "text": "11"})",
        R"({"color": "#0C0000", "text": "12"})",
        R"({"color": "#0D0000", "text": "13"})",
        R"({"color": "#0E0000", "text": "14"})",
        R"({"color": "#0F0000", "text": "15"})",
        R"({"color": "#100000", "text": "16"})",
        R"({"color": "#110000", "text": "17"})",
        R"({"color": "#120000", "text": "18"})",
        R"({"color": "#130000", "text": "19"})",
};

static const char *BASIC =
        "{"
        "    \"type\": \"APL\","
        "    \"version\": \"1.3\","
        "    \"theme\": \"dark\","
        "    \"layouts\": {"
        "        \"square\": {"
        "            \"parameters\": [\"color\", \"text\"],"
        "            \"item\": {"
        "                \"type\": \"Frame\","
        "                \"width\": 100,"
        "                \"height\": 100,"
        "                \"id\": \"frame${text}\","
        "                \"backgroundColor\": \"${color}\","
        "                \"item\": {"
        "                    \"type\": \"Text\","
        "                    \"text\": \"${text}\","
        "                    \"color\": \"black\","
        "                    \"width\": 100,"
        "                    \"height\": 100"
        "                }"
        "            }"
        "        }"
        "    },"
        "    \"mainTemplate\": {"
        "        \"parameters\": ["
        "            \"dynamicSource\""
        "        ],"
        "        \"item\": {"
        "            \"type\": \"Container\","
        "            \"items\": ["
        "                {"
        "                    \"type\": \"Sequence\","
        "                    \"id\": \"sequence\","
        "                    \"width\": 500,"
        "                    \"data\": \"${dynamicSource}\","
        "                    \"items\": {"
        "                        \"type\": \"square\","
        "                        \"index\": \"${index}\","
        "                        \"color\": \"${data.color}\","
        "                        \"text\": \"${data.text}\""
        "                    }"
        "                },"
        "                {"
        "                    \"type\": \"Pager\","
        "                    \"id\": \"pager\","
        "                    \"data\": \"${dynamicSource}\","
        "                    \"items\": {"
        "                        \"type\": \"square\","
        "                        \"index\": \"${index}\","
        "                        \"color\": \"${data.color}\","
        "                        \"text\": \"${data.text}\""
        "                    }"
        "                },"
        "                {"
        "                    \"type\": \"Container\","
        "                    \"id\": \"cont\","
        "                    \"data\": \"${dynamicSource}\","
        "                    \"items\": {"
        "                        \"type\": \"square\","
        "                        \"index\": \"${index}\","
        "                        \"color\": \"${data.color}\","
        "                        \"text\": \"${data.text}\""
        "                    }"
        "                }"
        "            ]"
        "        }"
        "    }"
        "}";

TEST_F(DynamicSourceTest, Basic)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(BASIC, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    auto seq = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeSequence, seq->getType());
    ASSERT_EQ(5, seq->getChildCount());
    ASSERT_EQ("frame0", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame4", seq->getChildAt(4)->getId());

    auto page = component->getChildAt(1);
    ASSERT_EQ(kComponentTypePager, page->getType());
    ASSERT_EQ(5, page->getChildCount());
    ASSERT_EQ("frame0", page->getChildAt(0)->getId());
    ASSERT_EQ("frame4", page->getChildAt(4)->getId());

    auto cont = component->getChildAt(2);
    ASSERT_EQ(kComponentTypeContainer, cont->getType());
    ASSERT_EQ(5, cont->getChildCount());
    ASSERT_EQ("frame0", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame4", cont->getChildAt(4)->getId());

    //=======================================================

    ds->getConnection()->processResponse();
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(seq));
    ASSERT_EQ(1, seq->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(page));
    ASSERT_EQ(1, page->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(cont));
    ASSERT_EQ(1, cont->getDirty().count(kPropertyNotifyChildrenChanged));

    root->clearDirty();

    ASSERT_EQ(10, seq->getChildCount());
    ASSERT_EQ("frame0", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame5", seq->getChildAt(5)->getId());
    ASSERT_EQ("frame9", seq->getChildAt(9)->getId());

    ASSERT_EQ(10, page->getChildCount());
    ASSERT_EQ("frame0", page->getChildAt(0)->getId());
    ASSERT_EQ("frame5", page->getChildAt(5)->getId());
    ASSERT_EQ("frame9", page->getChildAt(9)->getId());

    ASSERT_EQ(10, cont->getChildCount());
    ASSERT_EQ("frame0", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame5", cont->getChildAt(5)->getId());
    ASSERT_EQ("frame9", cont->getChildAt(9)->getId());
}

static const char *DATA_EMPTY =
        "{\n"
        "    \"dynamicSource\": {\n"
        "        \"type\": \"GenericList\",\n"
        "        \"listId\": \"vQdpOESlok\",\n"
        "        \"offset\": 0,\n"
        "        \"maxItems\": 20,\n"
        "        \"items\": []\n"
        "    }\n"
        "}";

TEST_F(DynamicSourceTest, Empty)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(BASIC, DATA_EMPTY);

    ASSERT_EQ(kComponentTypeContainer, component->getType());
    ASSERT_EQ(3, component->getChildCount());

    auto seq = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeSequence, seq->getType());
    ASSERT_EQ(0, seq->getChildCount());
    auto page = component->getChildAt(1);
    ASSERT_EQ(kComponentTypePager, page->getType());
    ASSERT_EQ(0, page->getChildCount());
    auto cont = component->getChildAt(2);
    ASSERT_EQ(kComponentTypeContainer, cont->getType());
    ASSERT_EQ(0, cont->getChildCount());

    ASSERT_TRUE(ds->getConnection()->processResponse());
    root->clearPending();

    ASSERT_EQ(5, seq->getChildCount());
    ASSERT_EQ("frame0", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame4", seq->getChildAt(4)->getId());

    ASSERT_EQ(5, page->getChildCount());
    ASSERT_EQ("frame0", page->getChildAt(0)->getId());
    ASSERT_EQ("frame4", page->getChildAt(4)->getId());

    ASSERT_EQ(5, cont->getChildCount());
    ASSERT_EQ("frame0", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame4", cont->getChildAt(4)->getId());

    // =======================================================

    ASSERT_TRUE(ds->getConnection()->processResponse());

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(seq));
    ASSERT_EQ(1, seq->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(page));
    ASSERT_EQ(1, page->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(cont));
    ASSERT_EQ(1, cont->getDirty().count(kPropertyNotifyChildrenChanged));

    root->clearDirty();

    ASSERT_EQ(10, seq->getChildCount());
    ASSERT_EQ("frame0", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame5", seq->getChildAt(5)->getId());
    ASSERT_EQ("frame9", seq->getChildAt(9)->getId());

    ASSERT_EQ(10, page->getChildCount());
    ASSERT_EQ("frame0", page->getChildAt(0)->getId());
    ASSERT_EQ("frame5", page->getChildAt(5)->getId());
    ASSERT_EQ("frame9", page->getChildAt(9)->getId());

    ASSERT_EQ(10, cont->getChildCount());
    ASSERT_EQ("frame0", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame5", cont->getChildAt(5)->getId());
    ASSERT_EQ("frame9", cont->getChildAt(9)->getId());
}

TEST_F(DynamicSourceTest, EmptyNotAligned)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(BASIC, DATA_EMPTY);

    ASSERT_FALSE(ds->getConnection()->processResponse(0, 5, 5));
    ASSERT_TRUE(ds->getConnection()->processResponse(0, 0, 5));
    root->clearPending();

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    auto seq = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeSequence, seq->getType());
    ASSERT_EQ(5, seq->getChildCount());
    ASSERT_EQ("frame0", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame4", seq->getChildAt(4)->getId());

    auto page = component->getChildAt(1);
    ASSERT_EQ(kComponentTypePager, page->getType());
    ASSERT_EQ(5, page->getChildCount());
    ASSERT_EQ("frame0", page->getChildAt(0)->getId());
    ASSERT_EQ("frame4", page->getChildAt(4)->getId());

    auto cont = component->getChildAt(2);
    ASSERT_EQ(kComponentTypeContainer, cont->getType());
    ASSERT_EQ(5, cont->getChildCount());
    ASSERT_EQ("frame0", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame4", cont->getChildAt(4)->getId());

    ASSERT_FALSE(ds->getConnection()->processResponse(0, 10, 5));
}

static const char *DATA_BACKWARDS =
        "{\n"
        "    \"dynamicSource\": {\n"
        "        \"type\": \"GenericList\",\n"
        "        \"listId\": \"vQdpOESlok\",\n"
        "        \"offset\": 15,\n"
        "        \"maxItems\": 20,\n"
        "        \"items\": [\n"
        "            { \"color\": \"#0F0000\", \"text\": \"15\" },\n"
        "            { \"color\": \"#100000\", \"text\": \"16\" },\n"
        "            { \"color\": \"#110000\", \"text\": \"17\" },\n"
        "            { \"color\": \"#120000\", \"text\": \"18\" },\n"
        "            { \"color\": \"#130000\", \"text\": \"19\" }\n"
        "        ]\n"
        "    }\n"
        "}";

TEST_F(DynamicSourceTest, Backwards)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(BASIC, DATA_BACKWARDS);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    auto seq = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeSequence, seq->getType());
    ASSERT_EQ(5, seq->getChildCount());
    ASSERT_EQ("frame15", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame19", seq->getChildAt(4)->getId());

    auto page = component->getChildAt(1);
    ASSERT_EQ(kComponentTypePager, page->getType());
    ASSERT_EQ(5, page->getChildCount());
    ASSERT_EQ("frame15", page->getChildAt(0)->getId());
    ASSERT_EQ("frame19", page->getChildAt(4)->getId());

    auto cont = component->getChildAt(2);
    ASSERT_EQ(kComponentTypeContainer, cont->getType());
    ASSERT_EQ(5, cont->getChildCount());
    ASSERT_EQ("frame15", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame19", cont->getChildAt(4)->getId());

    //=======================================================

    ASSERT_TRUE(ds->getConnection()->processResponse());

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(seq));
    ASSERT_EQ(1, seq->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(page));
    ASSERT_EQ(1, page->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(cont));
    ASSERT_EQ(1, cont->getDirty().count(kPropertyNotifyChildrenChanged));

    root->clearDirty();

    ASSERT_EQ(10, seq->getChildCount());
    ASSERT_EQ("frame10", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame14", seq->getChildAt(4)->getId());
    ASSERT_EQ("frame19", seq->getChildAt(9)->getId());

    ASSERT_EQ(10, page->getChildCount());
    ASSERT_EQ("frame10", page->getChildAt(0)->getId());
    ASSERT_EQ("frame14", page->getChildAt(4)->getId());
    ASSERT_EQ("frame19", page->getChildAt(9)->getId());

    ASSERT_EQ(10, cont->getChildCount());
    ASSERT_EQ("frame10", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame14", cont->getChildAt(4)->getId());
    ASSERT_EQ("frame19", cont->getChildAt(9)->getId());
}

static const char *DATA_OFFSET =
        "{"
        "    \"dynamicSource\": {"
        "        \"type\": \"GenericList\","
        "        \"listId\": \"vQdpOESlok\","
        "        \"offset\": 10,"
        "        \"maxItems\": 20,"
        "        \"items\": ["
        "            { \"color\": \"#0A0000\", \"text\": \"10\" },"
        "            { \"color\": \"#0B0000\", \"text\": \"11\" },"
        "            { \"color\": \"#0C0000\", \"text\": \"12\" },"
        "            { \"color\": \"#0D0000\", \"text\": \"13\" },"
        "            { \"color\": \"#0E0000\", \"text\": \"14\" }"
        "        ]"
        "    }"
        "}";

TEST_F(DynamicSourceTest, Offset)
{
    config->sequenceChildCache(5);
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(BASIC, DATA_OFFSET);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    auto seq = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeSequence, seq->getType());
    ASSERT_EQ(5, seq->getChildCount());
    ASSERT_EQ("frame10", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame14", seq->getChildAt(4)->getId());

    auto page = component->getChildAt(1);
    ASSERT_EQ(kComponentTypePager, page->getType());
    ASSERT_EQ(5, page->getChildCount());
    ASSERT_EQ("frame10", page->getChildAt(0)->getId());
    ASSERT_EQ("frame14", page->getChildAt(4)->getId());

    auto cont = component->getChildAt(2);
    ASSERT_EQ(kComponentTypeContainer, cont->getType());
    ASSERT_EQ(5, cont->getChildCount());
    ASSERT_EQ("frame10", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame14", cont->getChildAt(4)->getId());

    //=======================================================

    ASSERT_TRUE(ds->getConnection()->processResponse());

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(seq));
    ASSERT_EQ(1, seq->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(page));
    ASSERT_EQ(1, page->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(cont));
    ASSERT_EQ(1, cont->getDirty().count(kPropertyNotifyChildrenChanged));

    root->clearDirty();

    ASSERT_EQ(15, seq->getChildCount());
    ASSERT_EQ("frame5", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame9", seq->getChildAt(4)->getId());
    ASSERT_EQ("frame14", seq->getChildAt(9)->getId());
    ASSERT_EQ("frame19", seq->getChildAt(14)->getId());

    ASSERT_EQ(15, page->getChildCount());
    ASSERT_EQ("frame5", page->getChildAt(0)->getId());
    ASSERT_EQ("frame9", page->getChildAt(4)->getId());
    ASSERT_EQ("frame14", page->getChildAt(9)->getId());
    ASSERT_EQ("frame19", page->getChildAt(14)->getId());

    ASSERT_EQ(15, cont->getChildCount());
    ASSERT_EQ("frame5", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame9", cont->getChildAt(4)->getId());
    ASSERT_EQ("frame14", cont->getChildAt(9)->getId());
    ASSERT_EQ("frame19", cont->getChildAt(14)->getId());

    ASSERT_TRUE(ds->getConnection()->processResponse());

    ASSERT_TRUE(root->isDirty());

    dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(seq));
    ASSERT_EQ(1, seq->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(page));
    ASSERT_EQ(1, page->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(cont));
    ASSERT_EQ(1, cont->getDirty().count(kPropertyNotifyChildrenChanged));

    root->clearDirty();

    ASSERT_EQ(20, seq->getChildCount());
    ASSERT_EQ("frame0", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame4", seq->getChildAt(4)->getId());
    ASSERT_EQ("frame9", seq->getChildAt(9)->getId());
    ASSERT_EQ("frame14", seq->getChildAt(14)->getId());
    ASSERT_EQ("frame19", seq->getChildAt(19)->getId());

    ASSERT_EQ(20, page->getChildCount());
    ASSERT_EQ("frame0", page->getChildAt(0)->getId());
    ASSERT_EQ("frame4", page->getChildAt(4)->getId());
    ASSERT_EQ("frame9", page->getChildAt(9)->getId());
    ASSERT_EQ("frame14", page->getChildAt(14)->getId());
    ASSERT_EQ("frame19", page->getChildAt(19)->getId());

    ASSERT_EQ(20, cont->getChildCount());
    ASSERT_EQ("frame0", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame4", page->getChildAt(4)->getId());
    ASSERT_EQ("frame9", page->getChildAt(9)->getId());
    ASSERT_EQ("frame14", page->getChildAt(14)->getId());
    ASSERT_EQ("frame19", page->getChildAt(19)->getId());
}

TEST_F(DynamicSourceTest, OffsetSourceInitiated)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(BASIC, DATA_OFFSET);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    auto seq = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeSequence, seq->getType());
    ASSERT_EQ(5, seq->getChildCount());
    ASSERT_EQ("frame10", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame14", seq->getChildAt(4)->getId());

    auto page = component->getChildAt(1);
    ASSERT_EQ(kComponentTypePager, page->getType());
    ASSERT_EQ(5, page->getChildCount());
    ASSERT_EQ("frame10", page->getChildAt(0)->getId());
    ASSERT_EQ("frame14", page->getChildAt(4)->getId());

    auto cont = component->getChildAt(2);
    ASSERT_EQ(kComponentTypeContainer, cont->getType());
    ASSERT_EQ(5, cont->getChildCount());
    ASSERT_EQ("frame10", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame14", cont->getChildAt(4)->getId());

    //=======================================================

    ASSERT_TRUE(ds->getConnection()->processResponse(0, 5, 5));
    ASSERT_TRUE(ds->getConnection()->processResponse(0, 10, 5));
    ASSERT_TRUE(ds->getConnection()->processResponse(0, 0, 5));
    ASSERT_TRUE(ds->getConnection()->processResponse(0, 15, 5));

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(seq));
    ASSERT_EQ(1, seq->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(page));
    ASSERT_EQ(1, page->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(cont));
    ASSERT_EQ(1, cont->getDirty().count(kPropertyNotifyChildrenChanged));

    root->clearDirty();

    ASSERT_EQ(20, seq->getChildCount());
    ASSERT_EQ("frame0", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame4", seq->getChildAt(4)->getId());
    ASSERT_EQ("frame9", seq->getChildAt(9)->getId());
    ASSERT_EQ("frame14", seq->getChildAt(14)->getId());
    ASSERT_EQ("frame19", seq->getChildAt(19)->getId());

    ASSERT_EQ(20, page->getChildCount());
    ASSERT_EQ("frame0", page->getChildAt(0)->getId());
    ASSERT_EQ("frame4", page->getChildAt(4)->getId());
    ASSERT_EQ("frame9", page->getChildAt(9)->getId());
    ASSERT_EQ("frame14", page->getChildAt(14)->getId());
    ASSERT_EQ("frame19", page->getChildAt(19)->getId());

    ASSERT_EQ(20, cont->getChildCount());
    ASSERT_EQ("frame0", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame4", page->getChildAt(4)->getId());
    ASSERT_EQ("frame9", page->getChildAt(9)->getId());
    ASSERT_EQ("frame14", page->getChildAt(14)->getId());
    ASSERT_EQ("frame19", page->getChildAt(19)->getId());
}

static const char *CONDITIONAL =
        "{"
        "    \"type\": \"APL\","
        "    \"version\": \"1.3\","
        "    \"theme\": \"dark\","
        "    \"layouts\": {"
        "        \"square\": {"
        "            \"parameters\": [\"color\", \"text\"],"
        "            \"item\": {"
        "                \"type\": \"Frame\","
        "                \"width\": 100,"
        "                \"height\": 100,"
        "                \"id\": \"frame${text}\","
        "                \"backgroundColor\": \"${color}\","
        "                \"item\": {"
        "                    \"type\": \"Text\","
        "                    \"text\": \"${text}\","
        "                    \"id\": \"text${index}\","
        "                    \"color\": \"black\","
        "                    \"width\": 100,"
        "                    \"height\": 100"
        "                }"
        "            }"
        "        }"
        "    },"
        "    \"mainTemplate\": {"
        "        \"parameters\": ["
        "            \"dynamicSource\""
        "        ],"
        "        \"item\": {"
        "            \"type\": \"Container\","
        "            \"items\": ["
        "                {"
        "                    \"type\": \"Sequence\","
        "                    \"id\": \"sequence\","
        "                    \"data\": \"${dynamicSource}\","
        "                    \"items\": ["
        "                        {"
        "                            \"when\": \"${index%3 != 0}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${data.color}\","
        "                            \"text\": \"${data.text}\""
        "                        },"
        "                        {"
        "                            \"when\": \"${index%3 == 0}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${data.color}\","
        "                            \"text\": \"W ${data.text}\""
        "                        }"
        "                    ]"
        "                },"
        "                {"
        "                    \"type\": \"Pager\","
        "                    \"id\": \"pager\","
        "                    \"data\": \"${dynamicSource}\","
        "                    \"items\": ["
        "                        {"
        "                            \"when\": \"${index%3 != 0}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${data.color}\","
        "                            \"text\": \"${data.text}\""
        "                        },"
        "                        {"
        "                            \"when\": \"${index%3 == 0}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${data.color}\","
        "                            \"text\": \"W ${data.text}\""
        "                        }"
        "                    ]"
        "                },"
        "                {"
        "                    \"type\": \"Container\","
        "                    \"id\": \"cont\","
        "                    \"data\": \"${dynamicSource}\","
        "                    \"items\": ["
        "                        {"
        "                            \"when\": \"${index%3 != 0}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${data.color}\","
        "                            \"text\": \"${data.text}\""
        "                        },"
        "                        {"
        "                            \"when\": \"${index%3 == 0}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${data.color}\","
        "                            \"text\": \"W ${data.text}\""
        "                        }"
        "                    ]"
        "                }"
        "            ]"
        "        }"
        "    }"
        "}";

TEST_F(DynamicSourceTest, Conditional)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(CONDITIONAL, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    auto seq = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeSequence, seq->getType());
    ASSERT_EQ(5, seq->getChildCount());
    ASSERT_EQ("frameW0", seq->getChildAt(0)->getId());
    ASSERT_EQ("frame1", seq->getChildAt(1)->getId());
    ASSERT_EQ("frameW3", seq->getChildAt(3)->getId());
    ASSERT_EQ("frame4", seq->getChildAt(4)->getId());

    auto page = component->getChildAt(1);
    ASSERT_EQ(kComponentTypePager, page->getType());
    ASSERT_EQ(5, page->getChildCount());
    ASSERT_EQ("frameW0", page->getChildAt(0)->getId());
    ASSERT_EQ("frame1", page->getChildAt(1)->getId());
    ASSERT_EQ("frameW3", page->getChildAt(3)->getId());
    ASSERT_EQ("frame4", page->getChildAt(4)->getId());

    auto cont = component->getChildAt(2);
    ASSERT_EQ(kComponentTypeContainer, cont->getType());
    ASSERT_EQ(5, cont->getChildCount());
    ASSERT_EQ("frameW0", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame1", cont->getChildAt(1)->getId());
    ASSERT_EQ("frameW3", cont->getChildAt(3)->getId());
    ASSERT_EQ("frame4", cont->getChildAt(4)->getId());

    //=======================================================

    ASSERT_TRUE(ds->getConnection()->processResponse());

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(seq));
    ASSERT_EQ(1, seq->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(page));
    ASSERT_EQ(1, page->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, dirty.count(cont));
    ASSERT_EQ(1, cont->getDirty().count(kPropertyNotifyChildrenChanged));

    root->clearDirty();

    seq = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeSequence, seq->getType());
    ASSERT_EQ(10, seq->getChildCount());
    ASSERT_EQ("frame8", seq->getChildAt(8)->getId());
    ASSERT_EQ("frameW9", seq->getChildAt(9)->getId());

    page = component->getChildAt(1);
    ASSERT_EQ(kComponentTypePager, page->getType());
    ASSERT_EQ(10, page->getChildCount());
    ASSERT_EQ("frame8", page->getChildAt(8)->getId());
    ASSERT_EQ("frameW9", page->getChildAt(9)->getId());

    cont = component->getChildAt(2);
    ASSERT_EQ(kComponentTypeContainer, cont->getType());
    ASSERT_EQ(10, cont->getChildCount());
    ASSERT_EQ("frame8", cont->getChildAt(8)->getId());
    ASSERT_EQ("frameW9", cont->getChildAt(9)->getId());
}

static const char *EXPLICIT =
        "{"
        "    \"type\": \"APL\","
        "    \"version\": \"1.3\","
        "    \"theme\": \"dark\","
        "    \"layouts\": {"
        "        \"square\": {"
        "            \"parameters\": [\"color\", \"text\"],"
        "            \"item\": {"
        "                \"type\": \"Frame\","
        "                \"width\": 100,"
        "                \"height\": 100,"
        "                \"id\": \"frame${text}\","
        "                \"backgroundColor\": \"${color}\","
        "                \"item\": {"
        "                    \"type\": \"Text\","
        "                    \"text\": \"${text}\","
        "                    \"id\": \"text${index}\","
        "                    \"color\": \"black\","
        "                    \"width\": 100,"
        "                    \"height\": 100"
        "                }"
        "            }"
        "        }"
        "    },"
        "    \"mainTemplate\": {"
        "        \"parameters\": ["
        "            \"dynamicSource\""
        "        ],"
        "        \"item\": {"
        "            \"type\": \"Container\","
        "            \"items\": ["
        "                {"
        "                    \"type\": \"Sequence\","
        "                    \"id\": \"sequence\","
        "                    \"items\": ["
        "                        {"
        "                            \"when\": \"${dynamicSource[7]}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${dynamicSource[7].color}\","
        "                            \"text\": \"1E ${dynamicSource[7].text}\""
        "                        },"
        "                        {"
        "                            \"when\": \"${dynamicSource[10]}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${dynamicSource[10].color}\","
        "                            \"text\": \"2E ${dynamicSource[10].text}\""
        "                        }"
        "                    ]"
        "                },"
        "                {"
        "                    \"type\": \"Pager\","
        "                    \"id\": \"pager\","
        "                    \"items\": ["
        "                        {"
        "                            \"when\": \"${dynamicSource[2]}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${dynamicSource[2].color}\","
        "                            \"text\": \"1E ${dynamicSource[2].text}\""
        "                        },"
        "                        {"
        "                            \"when\": \"${dynamicSource[9]}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${dynamicSource[9].color}\","
        "                            \"text\": \"2E ${dynamicSource[9].text}\""
        "                        }"
        "                    ]"
        "                },"
        "                {"
        "                    \"type\": \"Container\","
        "                    \"id\": \"cont\","
        "                    \"items\": ["
        "                        {"
        "                            \"when\": \"${dynamicSource[2]}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${dynamicSource[2].color}\","
        "                            \"text\": \"1E ${dynamicSource[2].text}\""
        "                        },"
        "                        {"
        "                            \"when\": \"${dynamicSource[4]}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${dynamicSource[4].color}\","
        "                            \"text\": \"2E ${dynamicSource[4].text}\""
        "                        },"
        "                        {"
        "                            \"when\": \"${dynamicSource[9]}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${dynamicSource[9].color}\","
        "                            \"text\": \"3E ${dynamicSource[9].text}\""
        "                        },"
        "                        {"
        "                            \"when\": \"${dynamicSource[10]}\","
        "                            \"type\": \"square\","
        "                            \"index\": \"${index}\","
        "                            \"color\": \"${dynamicSource[10].color}\","
        "                            \"text\": \"4E ${dynamicSource[10].text}\""
        "                        }"
        "                    ]"
        "                }"
        "            ]"
        "        }"
        "    }"
        "}";

static const char *DATA_EXPLICIT =
        "{"
        "    \"dynamicSource\": {"
        "        \"type\": \"GenericList\","
        "        \"listId\": \"vQdpOESlok\","
        "        \"offset\": 5,"
        "        \"maxItems\": 20,"
        "        \"items\": ["
        "            { \"color\": \"#050000\", \"text\": \"5\" },"
        "            { \"color\": \"#060000\", \"text\": \"6\" },"
        "            { \"color\": \"#070000\", \"text\": \"7\" },"
        "            { \"color\": \"#080000\", \"text\": \"8\" },"
        "            { \"color\": \"#090000\", \"text\": \"9\" }"
        "        ]"
        "    }"
        "}";
// We assume that explicit references was present in initial array, and referred by EXISTING index, not data source index.
TEST_F(DynamicSourceTest, Explicit)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(EXPLICIT, DATA_EXPLICIT);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    auto seq = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeSequence, seq->getType());
    ASSERT_EQ(0, seq->getChildCount());

    auto page = component->getChildAt(1);
    ASSERT_EQ(kComponentTypePager, page->getType());
    ASSERT_EQ(1, page->getChildCount());
    ASSERT_EQ("frame1E7", page->getChildAt(0)->getId());

    auto cont = component->getChildAt(2);
    ASSERT_EQ(kComponentTypeContainer, cont->getType());
    ASSERT_EQ(2, cont->getChildCount());
    ASSERT_EQ("frame1E7", cont->getChildAt(0)->getId());
    ASSERT_EQ("frame2E9", cont->getChildAt(1)->getId());

    ASSERT_FALSE(root->isDirty());
}

static const char *DATA_EMPTY_OFFSET =
        "{\n"
        "    \"dynamicSource\": {\n"
        "        \"type\": \"GenericList\",\n"
        "        \"listId\": \"vQdpOESlok\",\n"
        "        \"offset\": 5,\n"
        "        \"maxItems\": 20,\n"
        "        \"items\": []\n"
        "    }\n"
        "}";

TEST_F(DynamicSourceTest, ExplicitEmpty)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(EXPLICIT, DATA_EMPTY_OFFSET);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    auto seq = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeSequence, seq->getType());
    ASSERT_EQ(0, seq->getChildCount());

    auto page = component->getChildAt(1);
    ASSERT_EQ(kComponentTypePager, page->getType());
    ASSERT_EQ(0, page->getChildCount());

    auto cont = component->getChildAt(2);
    ASSERT_EQ(kComponentTypeContainer, cont->getType());
    ASSERT_EQ(0, cont->getChildCount());

    ASSERT_FALSE(root->isDirty());
}

static const char *SIMPLE_SEQUENCE =
        "{"
        "    \"type\": \"APL\","
        "    \"version\": \"1.3\","
        "    \"theme\": \"dark\","
        "    \"mainTemplate\": {"
        "        \"parameters\": ["
        "            \"dynamicSource\""
        "        ],"
        "        \"item\": {"
        "            \"type\": \"Sequence\","
        "            \"id\": \"sequence\","
        "            \"data\": \"${dynamicSource}\","
        "            \"height\": 500,"
        "            \"items\": {"
        "                \"type\": \"Text\","
        "                \"id\": \"text${data.text}\","
        "                \"text\": \"text${data.text}\","
        "                \"color\": \"black\","
        "                \"width\": 100,"
        "                \"height\": 100"
        "            }"
        "        }"
        "    }"
        "}";

TEST_F(DynamicSourceTest, IncompleteResponse)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(SIMPLE_SEQUENCE, DATA_OFFSET);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(ds->getConnection()->processResponse(0, 7, 3));
    ASSERT_TRUE(ds->getConnection()->processResponse(1, 15, 3));
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    ASSERT_EQ(1, component->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "text7"));
    ASSERT_TRUE(CheckChild(3, "text10"));
    ASSERT_TRUE(CheckChild(7, "text14"));
    ASSERT_TRUE(CheckChild(8, "text15"));
    ASSERT_TRUE(CheckChild(10, "text17"));
}

TEST_F(DynamicSourceTest, BiggerResponse)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(SIMPLE_SEQUENCE, DATA_OFFSET);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(ds->getConnection()->processResponse(0, 3, 7));
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    ASSERT_EQ(1, component->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(12, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "text3"));
    ASSERT_TRUE(CheckChild(6, "text9"));
    ASSERT_TRUE(CheckChild(7, "text10"));
    ASSERT_TRUE(CheckChild(11, "text14"));
}

TEST_F(DynamicSourceTest, IntersectResponse)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(SIMPLE_SEQUENCE, DATA_OFFSET);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(ds->getConnection()->processResponse(0, 7, 5));
    ASSERT_TRUE(ds->getConnection()->processResponse(1, 13, 5));
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    ASSERT_EQ(1, component->getDirty().count(kPropertyNotifyChildrenChanged));
    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "text7"));
    ASSERT_TRUE(CheckChild(3, "text10"));
    ASSERT_TRUE(CheckChild(7, "text14"));
    ASSERT_TRUE(CheckChild(8, "text15"));
    ASSERT_TRUE(CheckChild(10, "text17"));
}

TEST_F(DynamicSourceTest, GapResponse)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(SIMPLE_SEQUENCE, DATA_OFFSET);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());

    ASSERT_FALSE(ds->getConnection()->processResponse(0, 5, 3));
    ASSERT_FALSE(ds->getConnection()->processResponse(1, 16, 3));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "text10"));
    ASSERT_TRUE(CheckChild(4, "text14"));
}

TEST_F(DynamicSourceTest, SimpleReplace)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(SIMPLE_SEQUENCE, DATA_OFFSET);

    // We have 5 initial items.
    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());

    std::vector<std::string> replaceItems;
    replaceItems.emplace_back(R"({"color": "#DEAD00", "text": "U10"})");
    replaceItems.emplace_back(R"({"color": "#BEEF00", "text": "U11"})");
    replaceItems.emplace_back(R"({"color": "#FAFAFA", "text": "U12"})");
    replaceItems.emplace_back(R"({"color": "#BEBEBE", "text": "U13"})");
    replaceItems.emplace_back(R"({"color": "#CACECA", "text": "U14"})");

    // Replace this 5 (starting and DS index 10) with another 5.
    ds->getConnection()->replace(10, replaceItems);
    root->clearPending();

    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_EQ("textU10", component->getChildAt(0)->getCalculated(kPropertyText).asString());
    ASSERT_EQ("textU14", component->getChildAt(4)->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(CheckChild(0, "textU10"));
    ASSERT_TRUE(CheckChild(4, "textU14"));

    // Lazy load to the full list
    ASSERT_TRUE(ds->getConnection()->processResponse(0, 0, 10));
    ASSERT_TRUE(ds->getConnection()->processResponse(1, 15, 5));
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    ASSERT_EQ(1, component->getDirty().count(kPropertyNotifyChildrenChanged));
    root->clearDirty();
    ASSERT_EQ(20, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "text0"));
    ASSERT_TRUE(CheckChild(10, "textU10"));
    ASSERT_TRUE(CheckChild(19, "text19"));

    replaceItems.clear();
    replaceItems.emplace_back(R"({"color": "#DEAD00", "text": "U0"})");
    replaceItems.emplace_back(R"({"color": "#BEEF00", "text": "U1"})");
    replaceItems.emplace_back(R"({"color": "#FAFAFA", "text": "U2"})");
    replaceItems.emplace_back(R"({"color": "#BEBEBE", "text": "U3"})");
    replaceItems.emplace_back(R"({"color": "#CACECA", "text": "U4"})");

    // Replace very first 5
    ds->getConnection()->replace(0, replaceItems);
    root->clearPending();
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_EQ(20, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "textU0"));
    ASSERT_TRUE(CheckChild(4, "textU4"));
    ASSERT_TRUE(CheckChild(5, "text5"));


    replaceItems.clear();
    replaceItems.emplace_back(R"({"color": "#DEAD00", "text": "U7"})");
    replaceItems.emplace_back(R"({"color": "#BEEF00", "text": "U8"})");
    replaceItems.emplace_back(R"({"color": "#FAFAFA", "text": "U9"})");
    replaceItems.emplace_back(R"({"color": "#BEBEBE", "text": "U10"})");
    replaceItems.emplace_back(R"({"color": "#CACECA", "text": "U11"})");

    // Replace some in the middle
    ds->getConnection()->replace(7, replaceItems);
    root->clearPending();
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_EQ(20, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "textU0"));
    ASSERT_TRUE(CheckChild(6, "text6"));
    ASSERT_TRUE(CheckChild(7, "textU7"));
    ASSERT_TRUE(CheckChild(8, "textU8"));
    ASSERT_TRUE(CheckChild(9, "textU9"));
    ASSERT_TRUE(CheckChild(10, "textU10"));
    ASSERT_TRUE(CheckChild(11, "textU11"));
    ASSERT_TRUE(CheckChild(12, "textU12"));
    ASSERT_TRUE(CheckChild(19, "text19"));
}

TEST_F(DynamicSourceTest, InsertAndReplace)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(SIMPLE_SEQUENCE, DATA_OFFSET);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());

    std::vector<std::string> replaceItems;
    replaceItems.emplace_back(R"({"color": "#BEEF00", "text": "U9"})");
    replaceItems.emplace_back(R"({"color": "#BEEF00", "text": "U10"})");
    replaceItems.emplace_back(R"({"color": "#BEEF00", "text": "U11"})");
    replaceItems.emplace_back(R"({"color": "#FAFAFA", "text": "U12"})");
    replaceItems.emplace_back(R"({"color": "#BEBEBE", "text": "U13"})");
    replaceItems.emplace_back(R"({"color": "#BEBEBE", "text": "U14"})");
    replaceItems.emplace_back(R"({"color": "#BEBEBE", "text": "U15"})");

    // Replace with covering of existing 5
    ds->getConnection()->replace(9, replaceItems);
    root->clearPending();

    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_EQ(7, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "textU9"));
    ASSERT_TRUE(CheckChild(1, "textU10"));
    ASSERT_TRUE(CheckChild(5, "textU14"));
    ASSERT_TRUE(CheckChild(6, "textU15"));

    replaceItems.clear();
    replaceItems.emplace_back(R"({"color": "#BEEF00", "text": "UU15"})");
    replaceItems.emplace_back(R"({"color": "#BEEF00", "text": "UU16"})");
    replaceItems.emplace_back(R"({"color": "#BEEF00", "text": "UU17"})");

    // Replace last item and add 2 more at the end.
    ds->getConnection()->replace(15, replaceItems);
    root->clearPending();

    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_EQ(9, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "textU9"));
    ASSERT_TRUE(CheckChild(1, "textU10"));
    ASSERT_TRUE(CheckChild(5, "textU14"));
    ASSERT_TRUE(CheckChild(6, "textUU15"));
    ASSERT_TRUE(CheckChild(8, "textUU17"));

    replaceItems.clear();
    replaceItems.emplace_back(R"({"color": "#BEEF00", "text": "UU7"})");
    replaceItems.emplace_back(R"({"color": "#BEEF00", "text": "UU8"})");
    replaceItems.emplace_back(R"({"color": "#BEEF00", "text": "UU9"})");

    // Replace first item and prepend 2
    ds->getConnection()->replace(7, replaceItems);
    root->clearPending();

    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "textUU7"));
    ASSERT_TRUE(CheckChild(2, "textUU9"));
    ASSERT_TRUE(CheckChild(3, "textU10"));
    ASSERT_TRUE(CheckChild(7, "textU14"));
    ASSERT_TRUE(CheckChild(8, "textUU15"));
    ASSERT_TRUE(CheckChild(10, "textUU17"));
}

TEST_F(DynamicSourceTest, SimpleInsertAndRemove)
{
    auto ds = std::make_shared<TestDataSourceProvider>(ITEMS);
    config->dataSourceProvider("GenericList", ds);

    loadDocument(SIMPLE_SEQUENCE, DATA_OFFSET);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "text10"));
    ASSERT_TRUE(CheckChild(4, "text14"));

    ASSERT_TRUE(ds->getConnection()->insert(9, R"({"color": "#BEEF00", "text": "I9"})"));
    ASSERT_TRUE(ds->getConnection()->insert(15, R"({"color": "#BEEF00", "text": "I15"})"));
    ASSERT_TRUE(ds->getConnection()->insert(12, R"({"color": "#BEEF00", "text": "I12"})"));
    root->clearPending();
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_EQ(8, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "textI9"));
    ASSERT_TRUE(CheckChild(1, "text10"));
    ASSERT_TRUE(CheckChild(2, "text11"));
    ASSERT_TRUE(CheckChild(3, "textI12"));
    ASSERT_TRUE(CheckChild(4, "text12"));
    ASSERT_TRUE(CheckChild(5, "text13"));
    ASSERT_TRUE(CheckChild(6, "text14"));
    ASSERT_TRUE(CheckChild(7, "textI15"));

    ASSERT_FALSE(ds->getConnection()->insert(5, R"({"color": "#BEEF00", "text": "I5"})"));
    ASSERT_FALSE(ds->getConnection()->insert(19, R"({"color": "#BEEF00", "text": "I19"})"));
    ASSERT_FALSE(ds->getConnection()->remove(5));
    ASSERT_FALSE(ds->getConnection()->remove(19));

    ASSERT_TRUE(ds->getConnection()->remove(9));
    ASSERT_TRUE(ds->getConnection()->remove(14));
    root->clearPending();
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_EQ(6, component->getChildCount());
    ASSERT_TRUE(CheckChild(0, "text10"));
    ASSERT_TRUE(CheckChild(1, "text11"));
    ASSERT_TRUE(CheckChild(2, "textI12"));
    ASSERT_TRUE(CheckChild(3, "text12"));
    ASSERT_TRUE(CheckChild(4, "text13"));
    ASSERT_TRUE(CheckChild(5, "textI15"));
}
