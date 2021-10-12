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

#include "rapidjson/document.h"

#include "../testeventloop.h"

#include "apl/component/component.h"
#include "apl/component/textmeasurement.h"
#include "apl/primitives/mediastate.h"

using namespace apl;

class VisualContextTest : public DocumentWrapper {
public:
    VisualContextTest() : DocumentWrapper(),
    vcDoc(rapidjson::kObjectType) {}

    void postInflate() override {
        ASSERT_TRUE(component);
        ASSERT_FALSE(root->isVisualContextDirty());
        serializeVisualContext();
    }

    void serializeVisualContext() {
        visualContext = root->serializeVisualContext(vcDoc.GetAllocator());
    }

protected:
    rapidjson::Document vcDoc;
    rapidjson::Value visualContext;
};

static const char* DATA = "{}";

static const char* BASIC = "{"
                           "  \"type\": \"APL\","
                           "  \"version\": \"1.1\","
                           "  \"mainTemplate\": {"
                           "    \"item\": {"
                           "      \"type\": \"TouchWrapper\","
                           "      \"width\": \"100%\","
                           "      \"height\": \"100%\","
                           "      \"item\":"
                           "      {"
                           "        \"type\": \"Text\","
                           "        \"id\": \"text\","
                           "        \"text\": \"Text.\","
                           "        \"entities\": [\"entity\"]"
                           "      }"
                           "    }"
                           "  }"
                           "}";

TEST_F(VisualContextTest, Basic) {
    loadDocument(BASIC);

    ASSERT_EQ(kComponentTypeTouchWrapper, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_FALSE(visualContext.HasMember("transform"));
    ASSERT_FALSE(visualContext.HasMember("id"));
    ASSERT_TRUE(visualContext.HasMember("uid"));
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_TRUE(visualContext["tags"].HasMember("clickable"));
    ASSERT_FALSE(visualContext.HasMember("visibility"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(1, visualContext["children"].GetArray().Size());
    auto& child = visualContext["children"][0];
    ASSERT_STREQ("text", child["id"].GetString());
    ASSERT_STREQ("text", child["type"].GetString());
    ASSERT_FALSE(child.HasMember("tags"));
}

static const char* BASIC_AVG = R"(
{
  "type": "APL",
  "version": "1.0",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "items": {
        "type": "path",
        "pathData": "M0,0 h100 v100 h-100 z",
        "fill": "red"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "box"
    }
  }
})";

TEST_F(VisualContextTest, BasicAVG) {
    loadDocument(BASIC_AVG);

    ASSERT_EQ(kComponentTypeVectorGraphic, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_FALSE(visualContext.HasMember("transform"));
    ASSERT_FALSE(visualContext.HasMember("id"));
    ASSERT_TRUE(visualContext.HasMember("uid"));
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_TRUE(visualContext["tags"].HasMember("clickable"));
    ASSERT_FALSE(visualContext.HasMember("visibility"));
}

static const char* TRANSFORM = "{"
                               "  \"type\": \"APL\","
                               "  \"version\": \"1.1\","
                               "  \"mainTemplate\": {"
                               "    \"item\": {"
                               "      \"type\": \"Text\","
                               "      \"id\": \"text\","
                               "      \"text\": \"Text.\","
                               "      \"entities\": [\"entity\"],"
                               "      \"transform\": [{ \"rotate\": 45}]"
                               "    }"
                               "  }"
                               "}";

TEST_F(VisualContextTest, Transform) {
    loadDocument(TRANSFORM);

    ASSERT_EQ(kComponentTypeText, component->getType());

    // Check parent
    ASSERT_STREQ("text", visualContext["id"].GetString());
    ASSERT_STREQ("text", visualContext["type"].GetString());
    ASSERT_TRUE(visualContext.HasMember("uid"));
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_FALSE(visualContext.HasMember("visibility"));

    ASSERT_FALSE(visualContext.HasMember("children"));

    ASSERT_TRUE(visualContext.HasMember("transform"));
    auto& transform = visualContext["transform"];
    ASSERT_EQ(6, transform.Size());
    ASSERT_NEAR(0.7, transform[0].GetFloat(), 0.1);
    ASSERT_NEAR(0.7, transform[1].GetFloat(), 0.1);
    ASSERT_NEAR(-0.7, transform[2].GetFloat(), 0.1);
    ASSERT_NEAR(0.7, transform[3].GetFloat(), 0.1);
    ASSERT_NEAR(432.8, transform[4].GetFloat(), 0.1);
    ASSERT_NEAR(-244.8, transform[5].GetFloat(), 0.1);
}

static const char* EMPTY_SEQUENCE = "{"
                                    "  \"type\": \"APL\","
                                    "  \"version\": \"1.1\","
                                    "  \"mainTemplate\": {"
                                    "    \"item\": {"
                                    "      \"type\": \"Sequence\""
                                    "    }"
                                    "  }"
                                    "}";

TEST_F(VisualContextTest, EmptySequence) {
    loadDocument(EMPTY_SEQUENCE);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // Check parent
    ASSERT_STREQ("empty", visualContext["type"].GetString());
    ASSERT_TRUE(visualContext.HasMember("uid"));
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_FALSE(visualContext.HasMember("visibility"));
    ASSERT_FALSE(visualContext.HasMember("children"));

    ASSERT_TRUE(visualContext.HasMember("tags"));
    auto& tags = visualContext["tags"];
    ASSERT_FALSE(tags.HasMember("list"));
}

static const char* SEQUENCE = "{"
                              "  \"type\": \"APL\","
                              "  \"version\": \"1.1\","
                              "  \"mainTemplate\": {"
                              "    \"parameters\": ["
                              "      \"payload\""
                              "    ],"
                              "    \"item\": {"
                              "      \"type\": \"Sequence\","
                              "      \"id\": \"seq\","
                              "      \"scrollDirection\": \"vertical\","
                              "      \"numbered\": true,"
                              "      \"items\": ["
                              "        {"
                              "          \"type\": \"Text\","
                              "          \"id\": \"item_${index}\","
                              "          \"height\": \"40dp\","
                              "          \"text\": \"A ${index}-${ordinal}-${length}\","
                              "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                              "        },"
                              "        {"
                              "          \"type\": \"Text\","
                              "          \"id\": \"item_${index}\","
                              "          \"height\": \"40dp\","
                              "          \"text\": \"B ${index}-${ordinal}-${length}\","
                              "          \"numbering\": \"skip\","
                              "          \"speech\": \"ssml\""
                              "        },"
                              "        {"
                              "          \"type\": \"Text\","
                              "          \"id\": \"item_${index}\","
                              "          \"height\": \"40dp\","
                              "          \"text\": \"C ${index}-${ordinal}-${length}\","
                              "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                              "        },"
                              "        {"
                              "          \"type\": \"Text\","
                              "          \"id\": \"item_${index}\","
                              "          \"height\": \"40dp\","
                              "          \"text\": \"A ${index}-${ordinal}-${length}\","
                              "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                              "        },"
                              "        {"
                              "          \"type\": \"Text\","
                              "          \"id\": \"item_${index}\","
                              "          \"height\": \"40dp\","
                              "          \"text\": \"B ${index}-${ordinal}-${length}\","
                              "          \"numbering\": \"skip\","
                              "          \"speech\": \"ssml\""
                              "        },"
                              "        {"
                              "          \"type\": \"Text\","
                              "          \"id\": \"item_${index}\","
                              "          \"height\": \"40dp\","
                              "          \"text\": \"C ${index}-${ordinal}-${length}\""
                              "        }"
                              "      ]"
                              "    }"
                              "  }"
                              "}";

TEST_F(VisualContextTest, Sequence) {
    loadDocument(SEQUENCE, DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    auto& tags = visualContext["tags"];
    ASSERT_STREQ("seq", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    ASSERT_TRUE(tags.HasMember("focused"));
    ASSERT_TRUE(tags.HasMember("scrollable"));
    auto& scrollable = tags["scrollable"];
    ASSERT_EQ("vertical", scrollable["direction"]);
    ASSERT_EQ(true, scrollable["allowForward"]);
    ASSERT_EQ(false, scrollable["allowBackwards"]);

    ASSERT_TRUE(tags.HasMember("list"));
    auto& list = tags["list"];
    ASSERT_EQ(6, list["itemCount"]);
    ASSERT_EQ(0, list["lowestIndexSeen"]);
    ASSERT_EQ(2, list["highestIndexSeen"]);
    ASSERT_EQ(1, list["lowestOrdinalSeen"]);
    ASSERT_EQ(2, list["highestOrdinalSeen"]);

    // Check children
    ASSERT_EQ(3, visualContext["children"].GetArray().Size());

    auto& reportedChild1 = visualContext["children"][0];
    ASSERT_STREQ("item_0", reportedChild1["id"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("entities"));
    ASSERT_FALSE(reportedChild1.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild1["type"].GetString());
    ASSERT_STREQ("1024x40+0+0:0", reportedChild1["position"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("tags"));
    auto& c1t = reportedChild1["tags"];
    ASSERT_FALSE(c1t.HasMember("focused"));
    ASSERT_EQ(1, c1t["ordinal"]);
    ASSERT_TRUE(c1t.HasMember("listItem"));
    ASSERT_EQ(0, c1t["listItem"]["index"]);

    auto& reportedChild2 = visualContext["children"][1];
    ASSERT_STREQ("item_1", reportedChild2["id"].GetString());
    ASSERT_FALSE(reportedChild2.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild2["type"].GetString());
    ASSERT_STREQ("1024x40+0+40:0", reportedChild2["position"].GetString());
    ASSERT_TRUE(reportedChild2.HasMember("tags"));
    auto& c2t = reportedChild2["tags"];
    ASSERT_FALSE(c2t.HasMember("focused"));
    ASSERT_EQ(2, c2t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(1, c2t["listItem"]["index"]);

    auto& reportedChild3 = visualContext["children"][2];
    ASSERT_STREQ("item_2", reportedChild3["id"].GetString());
    ASSERT_FLOAT_EQ(0.5f, reportedChild3["visibility"].GetFloat());
    ASSERT_STREQ("text", reportedChild3["type"].GetString());
    ASSERT_STREQ("1024x40+0+80:0", reportedChild3["position"].GetString());
    ASSERT_TRUE(reportedChild3.HasMember("tags"));
    auto& c3t = reportedChild3["tags"];
    ASSERT_TRUE(reportedChild3.HasMember("entities"));
    ASSERT_FALSE(c3t.HasMember("focused"));
    ASSERT_EQ(2, c3t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(2, c3t["listItem"]["index"]);

    component->update(kUpdateScrollPosition, 100);
    root->clearPending();
    ASSERT_TRUE(CheckDirtyVisualContext(root, component));

    serializeVisualContext();

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    tags = visualContext["tags"];

    scrollable = tags["scrollable"];
    ASSERT_EQ("vertical", scrollable["direction"]);
    ASSERT_EQ(true, scrollable["allowForward"]);
    ASSERT_EQ(true, scrollable["allowBackwards"]);
    list = tags["list"];
    ASSERT_EQ(6, list["itemCount"]);
    ASSERT_EQ(0, list["lowestIndexSeen"]);
    ASSERT_EQ(4, list["highestIndexSeen"]);
    ASSERT_EQ(1, list["lowestOrdinalSeen"]);
    ASSERT_EQ(4, list["highestOrdinalSeen"]);

    // Check children
    ASSERT_EQ(3, visualContext["children"].GetArray().Size());

    reportedChild1 = visualContext["children"][0];
    ASSERT_STREQ("item_2", reportedChild1["id"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("entities"));
    ASSERT_FLOAT_EQ(0.5f, reportedChild1["visibility"].GetFloat());
    ASSERT_STREQ("text", reportedChild1["type"].GetString());
    ASSERT_STREQ("1024x40+0-20:0", reportedChild1["position"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("tags"));
    c1t = reportedChild1["tags"];
    ASSERT_FALSE(c1t.HasMember("focused"));
    ASSERT_EQ(2, c1t["ordinal"]);
    ASSERT_TRUE(c1t.HasMember("listItem"));
    ASSERT_EQ(2, c1t["listItem"]["index"]);

    reportedChild2 = visualContext["children"][1];
    ASSERT_STREQ("item_3", reportedChild2["id"].GetString());
    ASSERT_FALSE(reportedChild2.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild2["type"].GetString());
    ASSERT_STREQ("1024x40+0+20:0", reportedChild2["position"].GetString());
    ASSERT_TRUE(reportedChild2.HasMember("tags"));
    c2t = reportedChild2["tags"];
    ASSERT_FALSE(c2t.HasMember("focused"));
    ASSERT_EQ(3, c2t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(3, c2t["listItem"]["index"]);

    reportedChild3 = visualContext["children"][2];
    ASSERT_STREQ("item_4", reportedChild3["id"].GetString());
    ASSERT_FALSE(reportedChild3.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild3["type"].GetString());
    ASSERT_STREQ("1024x40+0+60:0", reportedChild3["position"].GetString());
    ASSERT_TRUE(reportedChild3.HasMember("tags"));
    c3t = reportedChild3["tags"];
    ASSERT_TRUE(c3t.HasMember("spoken"));
    ASSERT_FALSE(c3t.HasMember("focused"));
    ASSERT_EQ(4, c3t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(4, c3t["listItem"]["index"]);
}

static const char* HORIZONTAL_SEQUENCE = "{"
                                         "  \"type\": \"APL\","
                                         "  \"version\": \"1.1\","
                                         "  \"mainTemplate\": {"
                                         "    \"parameters\": ["
                                         "      \"payload\""
                                         "    ],"
                                         "    \"item\": {"
                                         "      \"type\": \"Sequence\","
                                         "      \"id\": \"seq\","
                                         "      \"scrollDirection\": \"horizontal\","
                                         "      \"numbered\": true,"
                                         "      \"items\": ["
                                         "        {"
                                         "          \"type\": \"Text\","
                                         "          \"id\": \"item_${index}\","
                                         "          \"width\": \"40dp\","
                                         "          \"text\": \"A ${index}-${ordinal}-${length}\","
                                         "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                                         "        },"
                                         "        {"
                                         "          \"type\": \"Text\","
                                         "          \"id\": \"item_${index}\","
                                         "          \"width\": \"40dp\","
                                         "          \"text\": \"B ${index}-${ordinal}-${length}\","
                                         "          \"numbering\": \"skip\","
                                         "          \"speech\": \"ssml\""
                                         "        },"
                                         "        {"
                                         "          \"type\": \"Text\","
                                         "          \"id\": \"item_${index}\","
                                         "          \"width\": \"40dp\","
                                         "          \"text\": \"C ${index}-${ordinal}-${length}\","
                                         "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                                         "        },"
                                         "        {"
                                         "          \"type\": \"Text\","
                                         "          \"id\": \"item_${index}\","
                                         "          \"width\": \"40dp\","
                                         "          \"text\": \"A ${index}-${ordinal}-${length}\","
                                         "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                                         "        },"
                                         "        {"
                                         "          \"type\": \"Text\","
                                         "          \"id\": \"item_${index}\","
                                         "          \"width\": \"40dp\","
                                         "          \"text\": \"B ${index}-${ordinal}-${length}\","
                                         "          \"numbering\": \"skip\","
                                         "          \"speech\": \"ssml\""
                                         "        },"
                                         "        {"
                                         "          \"type\": \"Text\","
                                         "          \"id\": \"item_${index}\","
                                         "          \"width\": \"40dp\","
                                         "          \"text\": \"C ${index}-${ordinal}-${length}\""
                                         "        }"
                                         "      ]"
                                         "    }"
                                         "  }"
                                         "}";

TEST_F(VisualContextTest, HorizontalSequence) {
    loadDocument(HORIZONTAL_SEQUENCE, DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    auto& tags = visualContext["tags"];
    ASSERT_STREQ("seq", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    ASSERT_TRUE(tags.HasMember("focused"));
    ASSERT_TRUE(tags.HasMember("scrollable"));
    auto& scrollable = tags["scrollable"];
    ASSERT_EQ("horizontal", scrollable["direction"]);
    ASSERT_EQ(true, scrollable["allowForward"]);
    ASSERT_EQ(false, scrollable["allowBackwards"]);

    ASSERT_TRUE(tags.HasMember("list"));
    auto& list = tags["list"];
    ASSERT_EQ(6, list["itemCount"]);
    ASSERT_EQ(0, list["lowestIndexSeen"]);
    ASSERT_EQ(2, list["highestIndexSeen"]);
    ASSERT_EQ(1, list["lowestOrdinalSeen"]);
    ASSERT_EQ(2, list["highestOrdinalSeen"]);

    // Check children
    ASSERT_EQ(3, visualContext["children"].GetArray().Size());

    auto& reportedChild1 = visualContext["children"][0];
    ASSERT_STREQ("item_0", reportedChild1["id"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("entities"));
    ASSERT_FALSE(reportedChild1.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild1["type"].GetString());
    ASSERT_STREQ("40x800+0+0:0", reportedChild1["position"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("tags"));
    auto& c1t = reportedChild1["tags"];
    ASSERT_FALSE(c1t.HasMember("focused"));
    ASSERT_EQ(1, c1t["ordinal"]);
    ASSERT_TRUE(c1t.HasMember("listItem"));
    ASSERT_EQ(0, c1t["listItem"]["index"]);

    auto& reportedChild2 = visualContext["children"][1];
    ASSERT_STREQ("item_1", reportedChild2["id"].GetString());
    ASSERT_FALSE(reportedChild2.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild2["type"].GetString());
    ASSERT_STREQ("40x800+40+0:0", reportedChild2["position"].GetString());
    ASSERT_TRUE(reportedChild2.HasMember("tags"));
    auto& c2t = reportedChild2["tags"];
    ASSERT_FALSE(c2t.HasMember("focused"));
    ASSERT_EQ(2, c2t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(1, c2t["listItem"]["index"]);

    auto& reportedChild3 = visualContext["children"][2];
    ASSERT_STREQ("item_2", reportedChild3["id"].GetString());
    ASSERT_FLOAT_EQ(0.5f, reportedChild3["visibility"].GetFloat());
    ASSERT_STREQ("text", reportedChild3["type"].GetString());
    ASSERT_STREQ("40x800+80+0:0", reportedChild3["position"].GetString());
    ASSERT_TRUE(reportedChild3.HasMember("tags"));
    auto& c3t = reportedChild3["tags"];
    ASSERT_TRUE(reportedChild3.HasMember("entities"));
    ASSERT_FALSE(c3t.HasMember("focused"));
    ASSERT_EQ(2, c3t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(2, c3t["listItem"]["index"]);

    component->update(kUpdateScrollPosition, 100);
    root->clearPending();

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    serializeVisualContext();

    // Check parent
    tags = visualContext["tags"];
    scrollable = tags["scrollable"];
    ASSERT_EQ("horizontal", scrollable["direction"]);
    ASSERT_EQ(true, scrollable["allowForward"]);
    ASSERT_EQ(true, scrollable["allowBackwards"]);
    list = tags["list"];
    ASSERT_EQ(6, list["itemCount"]);
    ASSERT_EQ(0, list["lowestIndexSeen"]);
    ASSERT_EQ(4, list["highestIndexSeen"]);
    ASSERT_EQ(1, list["lowestOrdinalSeen"]);
    ASSERT_EQ(4, list["highestOrdinalSeen"]);

    // Check children
    ASSERT_EQ(3, visualContext["children"].GetArray().Size());

    reportedChild1 = visualContext["children"][0];
    ASSERT_STREQ("item_2", reportedChild1["id"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("entities"));
    ASSERT_FLOAT_EQ(0.5f, reportedChild1["visibility"].GetFloat());
    ASSERT_STREQ("text", reportedChild1["type"].GetString());
    ASSERT_STREQ("40x800-20+0:0", reportedChild1["position"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("tags"));
    c1t = reportedChild1["tags"];
    ASSERT_FALSE(c1t.HasMember("focused"));
    ASSERT_EQ(2, c1t["ordinal"]);
    ASSERT_TRUE(c1t.HasMember("listItem"));
    ASSERT_EQ(2, c1t["listItem"]["index"]);

    reportedChild2 = visualContext["children"][1];
    ASSERT_STREQ("item_3", reportedChild2["id"].GetString());
    ASSERT_FALSE(reportedChild2.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild2["type"].GetString());
    ASSERT_STREQ("40x800+20+0:0", reportedChild2["position"].GetString());
    ASSERT_TRUE(reportedChild2.HasMember("tags"));
    c2t = reportedChild2["tags"];
    ASSERT_FALSE(c2t.HasMember("focused"));
    ASSERT_EQ(3, c2t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(3, c2t["listItem"]["index"]);

    reportedChild3 = visualContext["children"][2];
    ASSERT_STREQ("item_4", reportedChild3["id"].GetString());
    ASSERT_FALSE(reportedChild3.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild3["type"].GetString());
    ASSERT_STREQ("40x800+60+0:0", reportedChild3["position"].GetString());
    ASSERT_TRUE(reportedChild3.HasMember("tags"));
    c3t = reportedChild3["tags"];
    ASSERT_TRUE(c3t.HasMember("spoken"));
    ASSERT_FALSE(c3t.HasMember("focused"));
    ASSERT_EQ(4, c3t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(4, c3t["listItem"]["index"]);
}

TEST_F(VisualContextTest, RevertedSequence) {
    loadDocument(SEQUENCE, DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    auto& tags = visualContext["tags"];
    ASSERT_STREQ("seq", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    ASSERT_TRUE(tags.HasMember("focused"));
    ASSERT_TRUE(tags.HasMember("scrollable"));
    auto& scrollable = tags["scrollable"];
    ASSERT_EQ("vertical", scrollable["direction"]);
    ASSERT_EQ(true, scrollable["allowForward"]);
    ASSERT_EQ(false, scrollable["allowBackwards"]);

    ASSERT_TRUE(tags.HasMember("list"));
    auto& list = tags["list"];
    ASSERT_EQ(6, list["itemCount"]);
    ASSERT_EQ(0, list["lowestIndexSeen"]);
    ASSERT_EQ(2, list["highestIndexSeen"]);
    ASSERT_EQ(1, list["lowestOrdinalSeen"]);
    ASSERT_EQ(2, list["highestOrdinalSeen"]);

    // Check children
    ASSERT_EQ(3, visualContext["children"].GetArray().Size());

    auto& reportedChild1 = visualContext["children"][0];
    ASSERT_STREQ("item_0", reportedChild1["id"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("entities"));
    ASSERT_FALSE(reportedChild1.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild1["type"].GetString());
    ASSERT_STREQ("1024x40+0+0:0", reportedChild1["position"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("tags"));
    auto& c1t = reportedChild1["tags"];
    ASSERT_FALSE(c1t.HasMember("focused"));
    ASSERT_EQ(1, c1t["ordinal"]);
    ASSERT_TRUE(c1t.HasMember("listItem"));
    ASSERT_EQ(0, c1t["listItem"]["index"]);

    auto& reportedChild2 = visualContext["children"][1];
    ASSERT_STREQ("item_1", reportedChild2["id"].GetString());
    ASSERT_FALSE(reportedChild2.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild2["type"].GetString());
    ASSERT_STREQ("1024x40+0+40:0", reportedChild2["position"].GetString());
    ASSERT_TRUE(reportedChild2.HasMember("tags"));
    auto& c2t = reportedChild2["tags"];
    ASSERT_FALSE(c2t.HasMember("focused"));
    ASSERT_EQ(2, c2t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(1, c2t["listItem"]["index"]);

    auto& reportedChild3 = visualContext["children"][2];
    ASSERT_STREQ("item_2", reportedChild3["id"].GetString());
    ASSERT_FLOAT_EQ(0.5f, reportedChild3["visibility"].GetFloat());
    ASSERT_STREQ("text", reportedChild3["type"].GetString());
    ASSERT_STREQ("1024x40+0+80:0", reportedChild3["position"].GetString());
    ASSERT_TRUE(reportedChild3.HasMember("tags"));
    auto& c3t = reportedChild3["tags"];
    ASSERT_TRUE(reportedChild3.HasMember("entities"));
    ASSERT_FALSE(c3t.HasMember("focused"));
    ASSERT_EQ(2, c3t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(2, c3t["listItem"]["index"]);

    component->update(kUpdateScrollPosition, 100);
    root->clearPending();

    // Roll back.
    component->update(kUpdateScrollPosition, 0);
    root->clearPending();

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    serializeVisualContext();

    // Check parent. We've seen more than initially.
    tags = visualContext["tags"];
    scrollable = tags["scrollable"];
    ASSERT_EQ("vertical", scrollable["direction"]);
    ASSERT_EQ(true, scrollable["allowForward"]);
    ASSERT_EQ(false, scrollable["allowBackwards"]);
    list = tags["list"];
    ASSERT_EQ(6, list["itemCount"]);
    ASSERT_EQ(0, list["lowestIndexSeen"]);
    ASSERT_EQ(4, list["highestIndexSeen"]);
    ASSERT_EQ(1, list["lowestOrdinalSeen"]);
    ASSERT_EQ(4, list["highestOrdinalSeen"]);

    // Check children, that should be the same
    ASSERT_EQ(3, visualContext["children"].GetArray().Size());

    reportedChild1 = visualContext["children"][0];
    ASSERT_STREQ("item_0", reportedChild1["id"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("entities"));
    ASSERT_FALSE(reportedChild1.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild1["type"].GetString());
    ASSERT_STREQ("1024x40+0+0:0", reportedChild1["position"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("tags"));
    c1t = reportedChild1["tags"];
    ASSERT_FALSE(c1t.HasMember("focused"));
    ASSERT_EQ(1, c1t["ordinal"]);
    ASSERT_TRUE(c1t.HasMember("listItem"));
    ASSERT_EQ(0, c1t["listItem"]["index"]);

    reportedChild2 = visualContext["children"][1];
    ASSERT_STREQ("item_1", reportedChild2["id"].GetString());
    ASSERT_FALSE(reportedChild2.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild2["type"].GetString());
    ASSERT_STREQ("1024x40+0+40:0", reportedChild2["position"].GetString());
    ASSERT_TRUE(reportedChild2.HasMember("tags"));
    c2t = reportedChild2["tags"];
    ASSERT_FALSE(c2t.HasMember("focused"));
    ASSERT_EQ(2, c2t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(1, c2t["listItem"]["index"]);

    reportedChild3 = visualContext["children"][2];
    ASSERT_STREQ("item_2", reportedChild3["id"].GetString());
    ASSERT_FLOAT_EQ(0.5f, reportedChild3["visibility"].GetFloat());
    ASSERT_STREQ("text", reportedChild3["type"].GetString());
    ASSERT_STREQ("1024x40+0+80:0", reportedChild3["position"].GetString());
    ASSERT_TRUE(reportedChild3.HasMember("tags"));
    c3t = reportedChild3["tags"];
    ASSERT_TRUE(reportedChild3.HasMember("entities"));
    ASSERT_FALSE(c3t.HasMember("focused"));
    ASSERT_EQ(2, c3t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(2, c3t["listItem"]["index"]);
}

static const char* SHIFTED_SEQUENCE = "{"
                                      "  \"type\": \"APL\","
                                      "  \"version\": \"1.1\","
                                      "  \"mainTemplate\": {"
                                      "    \"parameters\": ["
                                      "      \"payload\""
                                      "    ],"
                                      "    \"item\": {"
                                      "    \"type\": \"Container\","
                                      "    \"items\": {"
                                      "      \"type\": \"Sequence\","
                                      "      \"id\": \"seq\","
                                      "      \"scrollDirection\": \"vertical\","
                                      "      \"numbered\": true,"
                                      "      \"position\": \"absolute\","
                                      "      \"left\": \"100dp\","
                                      "      \"top\": \"100dp\","
                                      "      \"items\": ["
                                      "        {"
                                      "          \"type\": \"Text\","
                                      "          \"id\": \"item_${index}\","
                                      "          \"height\": \"40dp\","
                                      "          \"text\": \"A ${index}-${ordinal}-${length}\","
                                      "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                                      "        },"
                                      "        {"
                                      "          \"type\": \"Text\","
                                      "          \"id\": \"item_${index}\","
                                      "          \"height\": \"40dp\","
                                      "          \"text\": \"B ${index}-${ordinal}-${length}\","
                                      "          \"numbering\": \"skip\","
                                      "          \"speech\": \"ssml\""
                                      "        },"
                                      "        {"
                                      "          \"type\": \"Text\","
                                      "          \"id\": \"item_${index}\","
                                      "          \"height\": \"40dp\","
                                      "          \"text\": \"C ${index}-${ordinal}-${length}\","
                                      "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                                      "        },"
                                      "        {"
                                      "          \"type\": \"Text\","
                                      "          \"id\": \"item_${index}\","
                                      "          \"height\": \"40dp\","
                                      "          \"text\": \"A ${index}-${ordinal}-${length}\","
                                      "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                                      "        },"
                                      "        {"
                                      "          \"type\": \"Text\","
                                      "          \"id\": \"item_${index}\","
                                      "          \"height\": \"40dp\","
                                      "          \"text\": \"B ${index}-${ordinal}-${length}\","
                                      "          \"numbering\": \"skip\","
                                      "          \"speech\": \"ssml\""
                                      "        },"
                                      "        {"
                                      "          \"type\": \"Text\","
                                      "          \"id\": \"item_${index}\","
                                      "          \"height\": \"40dp\","
                                      "          \"text\": \"C ${index}-${ordinal}-${length}\""
                                      "        }"
                                      "      ]"
                                      "    }"
                                      "  }"
                                      "  }"
                                      "}";

TEST_F(VisualContextTest, ShiftedSequence) {
    loadDocument(SHIFTED_SEQUENCE, DATA);
    ASSERT_EQ(kComponentTypeContainer, component->getType());

    auto seq = component->getCoreChildAt(0);
    ASSERT_EQ(kComponentTypeSequence, seq->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    auto& tags = visualContext["tags"];
    // ASSERT_STREQ("seq", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    visualContext = visualContext["children"][0];
    tags = visualContext["tags"];

    ASSERT_TRUE(tags.HasMember("focused"));
    ASSERT_TRUE(tags.HasMember("list"));
    auto& list = tags["list"];
    ASSERT_EQ(6, list["itemCount"]);
    ASSERT_EQ(0, list["lowestIndexSeen"]);
    ASSERT_EQ(2, list["highestIndexSeen"]);
    ASSERT_EQ(1, list["lowestOrdinalSeen"]);
    ASSERT_EQ(2, list["highestOrdinalSeen"]);

    // Check children
    ASSERT_EQ(3, visualContext["children"].GetArray().Size());

    auto& reportedChild1 = visualContext["children"][0];
    ASSERT_STREQ("item_0", reportedChild1["id"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("entities"));
    ASSERT_FALSE(reportedChild1.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild1["type"].GetString());
    ASSERT_STREQ("70x40+100+100:0", reportedChild1["position"].GetString()); // 70 as default text measure counts characters
    ASSERT_TRUE(reportedChild1.HasMember("tags"));
    auto& c1t = reportedChild1["tags"];
    ASSERT_FALSE(c1t.HasMember("focused"));
    ASSERT_EQ(1, c1t["ordinal"]);
    ASSERT_TRUE(c1t.HasMember("listItem"));
    ASSERT_EQ(0, c1t["listItem"]["index"]);

    auto& reportedChild2 = visualContext["children"][1];
    ASSERT_STREQ("item_1", reportedChild2["id"].GetString());
    ASSERT_FALSE(reportedChild2.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild2["type"].GetString());
    ASSERT_STREQ("70x40+100+140:0", reportedChild2["position"].GetString());
    ASSERT_TRUE(reportedChild2.HasMember("tags"));
    auto& c2t = reportedChild2["tags"];
    ASSERT_FALSE(c2t.HasMember("focused"));
    ASSERT_EQ(2, c2t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(1, c2t["listItem"]["index"]);

    auto& reportedChild3 = visualContext["children"][2];
    ASSERT_STREQ("item_2", reportedChild3["id"].GetString());
    ASSERT_FLOAT_EQ(0.5f, reportedChild3["visibility"].GetFloat());
    ASSERT_STREQ("text", reportedChild3["type"].GetString());
    ASSERT_STREQ("70x40+100+180:0", reportedChild3["position"].GetString());
    ASSERT_TRUE(reportedChild3.HasMember("tags"));
    auto& c3t = reportedChild3["tags"];
    ASSERT_TRUE(reportedChild3.HasMember("entities"));
    ASSERT_FALSE(c3t.HasMember("focused"));
    ASSERT_EQ(2, c3t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(2, c3t["listItem"]["index"]);

    seq->update(kUpdateScrollPosition, 100);
    root->clearPending();

    ASSERT_TRUE(CheckDirtyVisualContext(root, seq));
    serializeVisualContext();
    visualContext = visualContext["children"][0];

    // Check parent
    tags = visualContext["tags"];
    list = tags["list"];
    ASSERT_EQ(6, list["itemCount"]);
    ASSERT_EQ(0, list["lowestIndexSeen"]);
    ASSERT_EQ(4, list["highestIndexSeen"]);
    ASSERT_EQ(1, list["lowestOrdinalSeen"]);
    ASSERT_EQ(4, list["highestOrdinalSeen"]);

    // Check children
    ASSERT_EQ(3, visualContext["children"].GetArray().Size());

    reportedChild1 = visualContext["children"][0];
    ASSERT_STREQ("item_2", reportedChild1["id"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("entities"));
    ASSERT_FLOAT_EQ(0.5f, reportedChild1["visibility"].GetFloat());
    ASSERT_STREQ("text", reportedChild1["type"].GetString());
    ASSERT_STREQ("70x40+100+80:0", reportedChild1["position"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("tags"));
    c1t = reportedChild1["tags"];
    ASSERT_FALSE(c1t.HasMember("focused"));
    ASSERT_EQ(2, c1t["ordinal"]);
    ASSERT_TRUE(c1t.HasMember("listItem"));
    ASSERT_EQ(2, c1t["listItem"]["index"]);

    reportedChild2 = visualContext["children"][1];
    ASSERT_STREQ("item_3", reportedChild2["id"].GetString());
    ASSERT_FALSE(reportedChild2.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild2["type"].GetString());
    ASSERT_STREQ("70x40+100+120:0", reportedChild2["position"].GetString());
    ASSERT_TRUE(reportedChild2.HasMember("tags"));
    c2t = reportedChild2["tags"];
    ASSERT_FALSE(c2t.HasMember("focused"));
    ASSERT_EQ(3, c2t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(3, c2t["listItem"]["index"]);

    reportedChild3 = visualContext["children"][2];
    ASSERT_STREQ("item_4", reportedChild3["id"].GetString());
    ASSERT_FALSE(reportedChild3.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild3["type"].GetString());
    ASSERT_STREQ("70x40+100+160:0", reportedChild3["position"].GetString());
    ASSERT_TRUE(reportedChild3.HasMember("tags"));
    c3t = reportedChild3["tags"];
    ASSERT_TRUE(c3t.HasMember("spoken"));
    ASSERT_FALSE(c3t.HasMember("focused"));
    ASSERT_EQ(4, c3t["ordinal"]);
    ASSERT_TRUE(c2t.HasMember("listItem"));
    ASSERT_EQ(4, c3t["listItem"]["index"]);
}

static const char* ORDINAL_SEQUENCE = "{"
                                      "  \"type\": \"APL\","
                                      "  \"version\": \"1.1\","
                                      "  \"mainTemplate\": {"
                                      "    \"parameters\": ["
                                      "      \"payload\""
                                      "    ],"
                                      "    \"item\": {"
                                      "      \"type\": \"Sequence\","
                                      "      \"id\": \"seq\","
                                      "      \"scrollDirection\": \"vertical\","
                                      "      \"numbered\": true,"
                                      "      \"position\": \"absolute\","
                                      "      \"left\": \"100dp\","
                                      "      \"top\": \"100dp\","
                                      "      \"items\": ["
                                      "        {"
                                      "          \"type\": \"Text\","
                                      "          \"id\": \"item_${index}\","
                                      "          \"height\": \"10dp\","
                                      "          \"text\": \"A ${index}-${ordinal}-${length}\","
                                      "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                                      "        },"
                                      "        {"
                                      "          \"type\": \"Text\","
                                      "          \"id\": \"item_${index}\","
                                      "          \"height\": \"10dp\","
                                      "          \"text\": \"B ${index}-${ordinal}-${length}\","
                                      "          \"speech\": \"ssml\""
                                      "        },"
                                      "        {"
                                      "          \"type\": \"Text\","
                                      "          \"id\": \"item_${index}\","
                                      "          \"height\": \"10dp\","
                                      "          \"text\": \"C ${index}-${ordinal}-${length}\","
                                      "          \"numbering\": \"reset\","
                                      "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                                      "        },"
                                      "        {"
                                      "          \"type\": \"Text\","
                                      "          \"id\": \"item_${index}\","
                                      "          \"height\": \"10dp\","
                                      "          \"text\": \"A ${index}-${ordinal}-${length}\","
                                      "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                                      "        },"
                                      "        {"
                                      "          \"type\": \"Text\","
                                      "          \"id\": \"item_${index}\","
                                      "          \"height\": \"10dp\","
                                      "          \"text\": \"B ${index}-${ordinal}-${length}\","
                                      "          \"numbering\": \"skip\","
                                      "          \"speech\": \"ssml\""
                                      "        },"
                                      "        {"
                                      "          \"type\": \"Text\","
                                      "          \"id\": \"item_${index}\","
                                      "          \"height\": \"10dp\","
                                      "          \"text\": \"C ${index}-${ordinal}-${length}\""
                                      "        }"
                                      "      ]"
                                      "    }"
                                      "  }"
                                      "}";

TEST_F(VisualContextTest, MissingOrdinalSequence) {
    loadDocument(ORDINAL_SEQUENCE, DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    auto& tags = visualContext["tags"];
    ASSERT_STREQ("seq", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    ASSERT_TRUE(tags.HasMember("focused"));
    ASSERT_FALSE(tags.HasMember("scrollable"));

    ASSERT_TRUE(tags.HasMember("list"));
    auto& list = tags["list"];
    ASSERT_EQ(6, list["itemCount"]);
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(5, list["highestIndexSeen"].GetInt());
    ASSERT_EQ(1, list["lowestOrdinalSeen"].GetInt());
    ASSERT_EQ(3, list["highestOrdinalSeen"].GetInt());
}

static const char* NO_ORDINAL_SEQUENCE = "{"
                                         "  \"type\": \"APL\","
                                         "  \"version\": \"1.1\","
                                         "  \"mainTemplate\": {"
                                         "    \"parameters\": ["
                                         "      \"payload\""
                                         "    ],"
                                         "    \"item\": {"
                                         "      \"type\": \"Sequence\","
                                         "      \"id\": \"seq\","
                                         "      \"scrollDirection\": \"vertical\","
                                         "      \"position\": \"absolute\","
                                         "      \"left\": \"100dp\","
                                         "      \"top\": \"100dp\","
                                         "      \"items\": ["
                                         "        {"
                                         "          \"type\": \"Text\","
                                         "          \"id\": \"item_${index}\","
                                         "          \"height\": \"10dp\","
                                         "          \"text\": \"A ${index}-${ordinal}-${length}\","
                                         "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                                         "        },"
                                         "        {"
                                         "          \"type\": \"Text\","
                                         "          \"id\": \"item_${index}\","
                                         "          \"height\": \"10dp\","
                                         "          \"text\": \"B ${index}-${ordinal}-${length}\","
                                         "          \"speech\": \"ssml\""
                                         "        },"
                                         "        {"
                                         "          \"type\": \"Text\","
                                         "          \"id\": \"item_${index}\","
                                         "          \"height\": \"10dp\","
                                         "          \"text\": \"C ${index}-${ordinal}-${length}\","
                                         "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                                         "        },"
                                         "        {"
                                         "          \"type\": \"Text\","
                                         "          \"id\": \"item_${index}\","
                                         "          \"height\": \"10dp\","
                                         "          \"text\": \"A ${index}-${ordinal}-${length}\","
                                         "          \"entities\": [\"${index}\", \"${ordinal}\"]"
                                         "        },"
                                         "        {"
                                         "          \"type\": \"Text\","
                                         "          \"id\": \"item_${index}\","
                                         "          \"height\": \"10dp\","
                                         "          \"text\": \"B ${index}-${ordinal}-${length}\","
                                         "          \"speech\": \"ssml\""
                                         "        },"
                                         "        {"
                                         "          \"type\": \"Text\","
                                         "          \"id\": \"item_${index}\","
                                         "          \"height\": \"10dp\","
                                         "          \"text\": \"C ${index}-${ordinal}-${length}\""
                                         "        }"
                                         "      ]"
                                         "    }"
                                         "  }"
                                         "}";

TEST_F(VisualContextTest, NoOrdinalSequence) {
    loadDocument(NO_ORDINAL_SEQUENCE, DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    auto& tags = visualContext["tags"];
    ASSERT_STREQ("seq", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    ASSERT_TRUE(tags.HasMember("focused"));
    ASSERT_FALSE(tags.HasMember("scrollable"));

    ASSERT_TRUE(tags.HasMember("list"));
    auto& list = tags["list"];
    ASSERT_EQ(6, list["itemCount"]);
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(5, list["highestIndexSeen"].GetInt());
    ASSERT_FALSE(list.HasMember("lowestOrdinalSeen"));
    ASSERT_FALSE(list.HasMember("highestOrdinalSeen"));
}

static const char* PADDED_SEQUENCE =
    "{\n"
    "    \"type\": \"APL\",\n"
    "    \"version\": \"1.0\",\n"
    "    \"mainTemplate\": {\n"
    "        \"item\": {\n"
    "            \"type\": \"Sequence\",\n"
    "            \"id\": \"seq\","
    "            \"scrollDirection\": \"%s\","
    "            \"data\": [\"red\", \"blue\", \"green\", \"yellow\", \"purple\", \"red\", \"blue\", \"green\", \"yellow\", \"purple\", \"red\", \"blue\", \"green\", \"yellow\", \"purple\"],\n"
    "            \"width\": 200,\n"
    "            \"height\": 200,\n"
    "            \"left\": 0,\n"
    "            \"right\": 0,\n"
    "            \"paddingTop\": 50,\n"
    "            \"paddingBottom\": 25,\n"
    "            \"item\": {\n"
    "                \"type\": \"Frame\",\n"
    "                \"width\": 100,\n"
    "                \"height\": 100,\n"
    "                \"backgroundColor\": \"${data}\"\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "}";

static const char* PADDED_SCROLLVIEW =
    "{\n"
    "    \"type\": \"APL\",\n"
    "    \"version\": \"1.1\",\n"
    "    \"mainTemplate\": {\n"
    "        \"item\": {\n"
    "            \"type\": \"ScrollView\",\n"
    "            \"id\": \"seq\","
    "            \"width\": \"100%\",\n"
    "            \"height\": \"100%\",\n"
    "            \"paddingTop\": 25,\n"
    "            \"paddingLeft\": 25,\n"
    "            \"paddingBottom\": 50, \n"
    "            \"paddingRight\": 50,\n"
    "            \"item\": {\n"
    "                \"type\": \"Container\",\n"
    "                \"item\": {\n"
    "                    \"type\": \"Frame\",\n"
    "                    \"width\": 100,\n"
    "                    \"height\": 100,\n"
    "                    \"backgroundColor\": \"${data}\"\n"
    "                },\n"
    "                \"data\": [\"red\", \"blue\", \"green\", \"yellow\", \"purple\", \"red\", \"blue\", \"green\", \"yellow\", \"purple\", \"red\", \"blue\", \"green\", \"yellow\", \"purple\"]\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "}";

struct PaddedScrollableTest {
    PaddedScrollableTest(ComponentType type, const char* doc, std::string direction,
                         int scrollPosition)
        : type(type), doc(doc), direction(direction), scrollPosition(scrollPosition) {}
    ComponentType type;
    const char* doc;
    std::string direction;
    int scrollPosition;
};

TEST_F(VisualContextTest, PaddedScrollableTests) {
    const int len = strlen(PADDED_SEQUENCE) * 2;
    std::unique_ptr<char[]> horizontalSeq(new char[len]);
    std::unique_ptr<char[]> verticalSeq(new char[len]);
    snprintf(horizontalSeq.get(), len, PADDED_SEQUENCE, "horizontal");
    snprintf(verticalSeq.get(), len, PADDED_SEQUENCE, "vertical");

    std::vector<PaddedScrollableTest> tests = {
        {kComponentTypeSequence, horizontalSeq.get(), "horizontal", 1300},
        {kComponentTypeSequence, verticalSeq.get(), "vertical", 1375},
        {kComponentTypeScrollView, PADDED_SCROLLVIEW, "vertical", 775},
    };
    for (auto& test : tests) {
        loadDocument(test.doc);
        ASSERT_EQ(test.type, component->getType());
        rapidjson::Document document(rapidjson::kObjectType);

        // test before any scrolling
        auto& tags = visualContext["tags"];
        auto& scrollable = tags["scrollable"];
        ASSERT_EQ(true, scrollable["allowForward"]);
        ASSERT_EQ(false, scrollable["allowBackwards"]);

        // now scroll halfway
        // We can't scroll to position we don't laid out so scroll in steps.
        while (component->getCalculated(kPropertyScrollPosition).asNumber() !=
               (test.scrollPosition / 2)) {
            component->update(kUpdateScrollPosition, test.scrollPosition / 2);
            root->clearPending();
            root->clearDirty();
        }

        ASSERT_TRUE(CheckDirtyVisualContext(root, component));
        serializeVisualContext();

        tags = visualContext["tags"];
        scrollable = tags["scrollable"];
        ASSERT_EQ(true, scrollable["allowForward"]);
        ASSERT_EQ(true, scrollable["allowBackwards"]);

        // now scroll all the way to the bottom
        // We can't scroll to position we don't laid out so scroll in steps.
        while (component->getCalculated(kPropertyScrollPosition).asNumber() !=
               test.scrollPosition) {
            component->update(kUpdateScrollPosition, test.scrollPosition);
            root->clearPending();
            root->clearDirty();
        }

        ASSERT_TRUE(CheckDirtyVisualContext(root, component));
        serializeVisualContext();
        tags = visualContext["tags"];
        scrollable = tags["scrollable"];
        ASSERT_EQ(false, scrollable["allowForward"]);
        ASSERT_EQ(true, scrollable["allowBackwards"]);
    }
}

static const char* PAGER = "{"
                           "  \"type\": \"APL\","
                           "  \"version\": \"1.1\","
                           "  \"mainTemplate\": {"
                           "    \"parameters\": ["
                           "      \"payload\""
                           "    ],"
                           "    \"item\": {"
                           "      \"type\": \"Pager\","
                           "      \"id\": \"page\","
                           "      \"navigation\": \"forward-only\","
                           "      \"items\": ["
                           "        {"
                           "          \"type\": \"Text\","
                           "          \"id\": \"item_0\","
                           "          \"text\": \"A\","
                           "          \"speech\": \"ssml\""
                           "        },"
                           "        {"
                           "          \"type\": \"Text\","
                           "          \"id\": \"item_1\","
                           "          \"text\": \"B\","
                           "          \"entities\": [\"entity\"]"
                           "        },"
                           "        {"
                           "          \"type\": \"Text\","
                           "          \"id\": \"item_2\","
                           "          \"text\": \"C\","
                           "          \"speech\": \"ssml\""
                           "        }"
                           "      ]"
                           "    }"
                           "  }"
                           "}";

TEST_F(VisualContextTest, Pager) {
    loadDocument(PAGER, DATA);

    ASSERT_EQ(kComponentTypePager, component->getType());
    advanceTime(10);

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    auto& tags = visualContext["tags"];
    ASSERT_STREQ("page", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    ASSERT_TRUE(tags.HasMember("focused"));
    ASSERT_TRUE(tags.HasMember("pager"));
    auto& pager = tags["pager"];
    ASSERT_EQ(0, pager["index"]);
    ASSERT_EQ(3, pager["pageCount"]);
    ASSERT_EQ(true, pager["allowForward"]);
    ASSERT_EQ(false, pager["allowBackwards"]);

    // Check children
    ASSERT_EQ(1, visualContext["children"].GetArray().Size());

    auto& reportedChild1 = visualContext["children"][0];
    ASSERT_STREQ("item_0", reportedChild1["id"].GetString());
    ASSERT_FALSE(reportedChild1.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild1["type"].GetString());
    ASSERT_TRUE(reportedChild1.HasMember("tags"));
    auto& c1t = reportedChild1["tags"];
    ASSERT_TRUE(c1t.HasMember("spoken"));

    component->update(kUpdatePagerPosition, 1);
    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    serializeVisualContext();

    auto& tags2 = visualContext["tags"];
    ASSERT_TRUE(tags2.HasMember("pager"));
    auto& pager2 = tags2["pager"];
    ASSERT_EQ(1, pager2["index"]);

    auto& reportedChild2 = visualContext["children"][0];
    ASSERT_STREQ("item_1", reportedChild2["id"].GetString());
    ASSERT_FALSE(reportedChild2.HasMember("visibility"));
    ASSERT_STREQ("text", reportedChild2["type"].GetString());
    ASSERT_TRUE(reportedChild2.HasMember("entities"));
    ASSERT_FALSE(reportedChild2.HasMember("tags"));
}

static const char* MEDIA =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"theme\": \"auto\","
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"item\": {"
    "      \"type\": \"Pager\","
    "      \"id\": \"page\","
    "      \"height\": \"100%\","
    "      \"width\": \"100%\","
    "      \"items\": ["
    "        {\n"
    "          \"type\": \"Video\","
    "          \"id\": \"video\","
    "          \"height\": \"100%\","
    "          \"width\": \"100%\","
    "          \"autoplay\": true,"
    "          \"audioTrack\": \"background\","
    "          \"source\": ["
    "            \"SOURCE0\","
    "            {"
    "              \"url\": \"https://s3.amazonaws.com/elon-video-urls/minion1.mp4\","
    "              \"entities\": [\"source\"]"
    "            }"
    "          ],"
    "          \"entities\": [\"video\"]"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(VisualContextTest, Media) {
    loadDocument(MEDIA, DATA);
    ASSERT_EQ(kComponentTypePager, component->getType());
    auto video = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeVideo, video->getType());

    video->updateMediaState(MediaState(1, 2, 1000, 38000, true, false));
    ASSERT_TRUE(CheckDirtyVisualContext(root, video));
    serializeVisualContext();
    ASSERT_FALSE(CheckDirtyVisualContext(root, video));

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    auto& tags = visualContext["tags"];
    ASSERT_TRUE(tags.HasMember("focused"));
    ASSERT_STREQ("page", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("video", visualContext["type"].GetString());

    ASSERT_FALSE(tags.HasMember("pager"));

    // Check children
    ASSERT_EQ(1, visualContext["children"].Size());

    auto& reportedChild = visualContext["children"][0];
    ASSERT_STREQ("video", reportedChild["id"].GetString());
    ASSERT_FALSE(reportedChild.HasMember("visibility"));
    ASSERT_STREQ("video", reportedChild["type"].GetString());
    ASSERT_TRUE(reportedChild.HasMember("tags"));
    auto& ct = reportedChild["tags"];
    ASSERT_FALSE(ct.HasMember("focused"));
    ASSERT_TRUE(ct.HasMember("media"));
    auto& media = ct["media"];
    ASSERT_EQ(true, media["allowAdjustSeekPositionForward"].GetBool());
    ASSERT_EQ(true, media["allowAdjustSeekPositionBackwards"].GetBool());
    ASSERT_EQ(false, media["allowNext"].GetBool());
    ASSERT_EQ(true, media["allowPrevious"].GetBool());
    auto& entity = media["entities"];
    ASSERT_EQ(1, entity.Size());
    ASSERT_STREQ("source", entity[0].GetString());
    ASSERT_EQ(1000, media["positionInMilliseconds"].GetInt());
    ASSERT_STREQ("paused", media["state"].GetString());
    ASSERT_STREQ("https://s3.amazonaws.com/elon-video-urls/minion1.mp4", media["url"].GetString());
}

static const char* EMPTY_MEDIA = "{"
                                 "  \"type\": \"APL\","
                                 "  \"version\": \"1.1\","
                                 "  \"theme\": \"auto\","
                                 "  \"mainTemplate\": {"
                                 "    \"parameters\": ["
                                 "      \"payload\""
                                 "    ],"
                                 "    \"item\": {"
                                 "      \"type\": \"Video\""
                                 "    }"
                                 "  }"
                                 "}";

TEST_F(VisualContextTest, EmptyMedia) {
    loadDocument(EMPTY_MEDIA, DATA);
    ASSERT_EQ(kComponentTypeVideo, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    auto& tags = visualContext["tags"];
    ASSERT_TRUE(tags.HasMember("viewport"));
    ASSERT_FALSE(tags.HasMember("media"));
}

static const char* DEEP = "{"
                          "  \"type\": \"APL\","
                          "  \"version\": \"1.1\","
                          "  \"mainTemplate\": {"
                          "    \"parameters\": ["
                          "      \"payload\""
                          "    ],"
                          "    \"item\": {"
                          "      \"type\": \"Container\","
                          "      \"id\": \"ctr\","
                          "      \"width\": \"100%\","
                          "      \"height\": \"157dp\","
                          "      \"items\": ["
                          "        {"
                          "          \"type\": \"TouchWrapper\","
                          "          \"id\": \"touchWrapper\","
                          "          \"width\": \"100%\","
                          "          \"height\": \"50%\","
                          "          \"item\": {"
                          "            \"type\": \"Text\","
                          "            \"id\": \"text\","
                          "            \"text\": \"Short text.\","
                          "            \"inheritParentState\": true,"
                          "            \"entities\": [\"deep text\"]"
                          "          }"
                          "        }"
                          "      ]"
                          "    }"
                          "  }"
                          "}";

TEST_F(VisualContextTest, Deep) {
    loadDocument(DEEP, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    auto& tags = visualContext["tags"];
    ASSERT_FALSE(tags.HasMember("focused"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(1, visualContext["children"].GetArray().Size());
    auto& touchWrapper = visualContext["children"][0];
    ASSERT_STREQ("touchWrapper", touchWrapper["id"].GetString());
    ASSERT_FALSE(touchWrapper.HasMember("visibility"));
    ASSERT_STREQ("text", touchWrapper["type"].GetString());
    ASSERT_TRUE(touchWrapper.HasMember("tags"));
    auto& twt = touchWrapper["tags"];
    ASSERT_TRUE(twt.HasMember("focused"));
    ASSERT_TRUE(twt.HasMember("clickable"));

    // Check children
    ASSERT_EQ(1, touchWrapper["children"].GetArray().Size());
    auto& text = touchWrapper["children"][0];
    ASSERT_STREQ("text", text["id"].GetString());
    ASSERT_FALSE(text.HasMember("visibility"));
    ASSERT_STREQ("text", text["type"].GetString());
    ASSERT_FALSE(text.HasMember("tags"));
    ASSERT_STREQ("1024x10+0+0:0", text["position"].GetString());
}

static const char* EMPTY = "{"
                           "  \"type\": \"APL\","
                           "  \"version\": \"1.1\","
                           "  \"mainTemplate\": {"
                           "    \"parameters\": ["
                           "      \"payload\""
                           "    ],"
                           "    \"item\": {"
                           "      \"type\": \"Container\","
                           "      \"id\": \"ctr\","
                           "      \"width\": \"100%\","
                           "      \"height\": \"157dp\","
                           "      \"items\": ["
                           "        {"
                           "          \"type\": \"Text\","
                           "          \"id\": \"item_${index}\","
                           "          \"text\": \"Text without entity or spokeability.\""
                           "        }"
                           "      ]"
                           "    }"
                           "  }"
                           "}";

TEST_F(VisualContextTest, Empty) {
    loadDocument(EMPTY, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_FALSE(visualContext.HasMember("children"));
}

static const char* INHERIT_STATE = "{"
                                   "  \"type\": \"APL\","
                                   "  \"version\": \"1.1\","
                                   "  \"mainTemplate\": {"
                                   "    \"parameters\": ["
                                   "      \"payload\""
                                   "    ],"
                                   "    \"item\": {"
                                   "      \"type\": \"TouchWrapper\","
                                   "      \"width\": \"100%\","
                                   "      \"height\": \"100%\","
                                   "      \"items\":"
                                   "      {"
                                   "        \"type\": \"Text\","
                                   "        \"id\": \"item-0\","
                                   "        \"text\": \"Inherit.\","
                                   "        \"entities\": [\"entity\"],"
                                   "        \"inheritParentState\": true"
                                   "      }"
                                   "    }"
                                   "  }"
                                   "}";

TEST_F(VisualContextTest, InheritState) {
    loadDocument(INHERIT_STATE, DATA);

    ASSERT_EQ(kComponentTypeTouchWrapper, component->getType());

    auto text = component->getCoreChildAt(0);
    ASSERT_EQ(kComponentTypeText, text->getType());

    component->setState(kStateChecked, true);
    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    serializeVisualContext();
    component->setState(kStateDisabled, true);
    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    serializeVisualContext();

    // Check parent
    ASSERT_STREQ("text", visualContext["type"].GetString());
    ASSERT_TRUE(visualContext.HasMember("tags"));
    auto& tags = visualContext["tags"];
    ASSERT_TRUE(tags["checked"].GetBool());
    ASSERT_TRUE(tags["disabled"].GetBool());
    ASSERT_TRUE(tags["clickable"].GetBool());

    // Check children
    ASSERT_EQ(1, visualContext["children"].GetArray().Size());
    auto& textContext = visualContext["children"][0];
    ASSERT_TRUE(textContext.HasMember("tags"));
    ASSERT_TRUE(textContext["tags"]["disabled"].GetBool());
    ASSERT_FALSE(textContext["tags"].HasMember("checked"));
}

static const char* STATES = "{"
                            "  \"type\": \"APL\","
                            "  \"version\": \"1.1\","
                            "  \"mainTemplate\": {"
                            "    \"parameters\": ["
                            "      \"payload\""
                            "    ],"
                            "    \"item\": {"
                            "      \"type\": \"Container\","
                            "      \"id\": \"ctr\","
                            "      \"width\": \"100%\","
                            "      \"height\": \"157dp\","
                            "      \"items\": ["
                            "        {"
                            "          \"type\": \"TouchWrapper\","
                            "          \"id\": \"item_0\","
                            "          \"item\": {"
                            "            \"type\": \"Text\","
                            "            \"text\": \"Disabled clickable.\""
                            "          }"
                            "        },"
                            "        {"
                            "          \"type\": \"TouchWrapper\","
                            "          \"id\": \"item_1\","
                            "          \"item\": {"
                            "            \"type\": \"Text\","
                            "            \"text\": \"Disabled but with entity.\""
                            "          },"
                            "          \"entities\": [\"entity\"]"
                            "        }"
                            "      ]"
                            "    }"
                            "  }"
                            "}";

TEST_F(VisualContextTest, States) {
    loadDocument(STATES, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // change state and assert the visual context set/reset dirty
    component->getCoreChildAt(0)->setState(kStateChecked, true);
    ASSERT_TRUE(CheckDirtyVisualContext(root, component->getCoreChildAt(0)));
    serializeVisualContext();
    component->getCoreChildAt(1)->setState(kStateFocused, true);
    ASSERT_TRUE(CheckDirtyVisualContext(root, component->getCoreChildAt(1)));
    serializeVisualContext();
    component->getCoreChildAt(0)->setState(kStateDisabled, true);
    ASSERT_TRUE(CheckDirtyVisualContext(root, component->getCoreChildAt(0)));
    serializeVisualContext();
    component->getCoreChildAt(1)->setState(kStateDisabled, true);
    ASSERT_TRUE(CheckDirtyVisualContext(root, component->getCoreChildAt(1)));
    serializeVisualContext();

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(2, visualContext["children"].GetArray().Size());
    auto& childContext = visualContext["children"][0];
    ASSERT_STREQ("item_0", childContext["id"].GetString());
    ASSERT_TRUE(childContext["tags"].HasMember("disabled"));
    ASSERT_TRUE(childContext["tags"].HasMember("clickable"));
    ASSERT_TRUE(childContext["tags"].HasMember("checked"));

    childContext = visualContext["children"][1];
    ASSERT_STREQ("item_1", childContext["id"].GetString());
    ASSERT_TRUE(childContext.HasMember("entities"));
    ASSERT_TRUE(childContext["tags"].HasMember("disabled"));
    ASSERT_TRUE(childContext["tags"].HasMember("focused"));

    // change state and assert the visual context set/reset dirty
    component->getCoreChildAt(0)->setState(kStateChecked, false);
    ASSERT_TRUE(CheckDirtyVisualContext(root, component->getCoreChildAt(0)));
    serializeVisualContext();
    component->getCoreChildAt(0)->setState(kStateFocused, true);
    ASSERT_TRUE(CheckDirtyVisualContext(root, component->getCoreChildAt(0)));
    serializeVisualContext();
    component->getCoreChildAt(0)->setState(kStateDisabled, false);
    ASSERT_TRUE(CheckDirtyVisualContext(root, component->getCoreChildAt(0)));
    serializeVisualContext();
    component->getCoreChildAt(1)->setState(kStateDisabled, false);
    ASSERT_TRUE(CheckDirtyVisualContext(root, component->getCoreChildAt(1)));
    serializeVisualContext();

    // Check children
    ASSERT_EQ(2, visualContext["children"].GetArray().Size());
    childContext = visualContext["children"][0];
    ASSERT_STREQ("item_0", childContext["id"].GetString());
    ASSERT_FALSE(childContext["tags"].HasMember("disabled"));
    ASSERT_TRUE(childContext["tags"].HasMember("clickable"));
    ASSERT_FALSE(childContext["tags"].HasMember("checked"));
    ASSERT_TRUE(childContext["tags"].HasMember("focused"));

    childContext = visualContext["children"][1];
    ASSERT_STREQ("item_1", childContext["id"].GetString());
    ASSERT_TRUE(childContext.HasMember("entities"));
    ASSERT_FALSE(childContext["tags"].HasMember("disabled"));
}

static const char* TYPE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"item\":"
    "    {"
    "      \"type\": \"Container\","
    "      \"id\": \"ctr\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Text\","
    "          \"id\": \"text\","
    "          \"text\": \"Text.\","
    "          \"entities\": [\"entity\"]"
    "        },"
    "        {"
    "          \"type\": \"Video\","
    "          \"id\": \"video\","
    "          \"height\": 300,"
    "          \"width\": 716.8,"
    "          \"top\": 10,"
    "          \"left\": 100,"
    "          \"autoplay\": true,"
    "          \"audioTrack\": \"background\","
    "          \"source\": ["
    "            {"
    "              \"url\": \"https://s3.amazonaws.com/elon-video-urls/minion1.mp4\""
    "            }"
    "          ],"
    "          \"entities\": [\"video\"]"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"tw\","
    "          \"item\": {"
    "            \"type\": \"Text\","
    "            \"id\": \"item_20\","
    "            \"text\": \"Clickable.\""
    "          }"
    "        },"
    "        {"
    "          \"type\": \"Image\","
    "          \"id\": \"image\","
    "          \"source\": \"http://images.amazon.com/image/foo.png\","
    "          \"scale\": \"fill\","
    "          \"width\": 300,"
    "          \"height\": 300,"
    "          \"entities\": [\"entity\"]"
    "        },"
    "        {"
    "          \"type\": \"Text\","
    "          \"id\": \"empty\","
    "          \"text\": \"\","
    "          \"entities\": [\"entity\"]"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(VisualContextTest, Type) {
    loadDocument(TYPE, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("mixed", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(4, visualContext["children"].Size());
    auto& c1 = visualContext["children"][0];
    ASSERT_STREQ("text", c1["id"].GetString());
    ASSERT_STREQ("text", c1["type"].GetString());

    auto& c2 = visualContext["children"][1];
    ASSERT_STREQ("video", c2["id"].GetString());
    ASSERT_STREQ("video", c2["type"].GetString());

    auto& c3 = visualContext["children"][2];
    ASSERT_STREQ("tw", c3["id"].GetString());
    ASSERT_STREQ("text", c3["type"].GetString());

    auto& c4 = visualContext["children"][3];
    ASSERT_STREQ("image", c4["id"].GetString());
    ASSERT_STREQ("graphic", c4["type"].GetString());
}

static const char* TYPE_PROPAGATE = "{"
                                    "  \"type\": \"APL\","
                                    "  \"version\": \"1.1\","
                                    "  \"mainTemplate\": {"
                                    "    \"parameters\": ["
                                    "      \"payload\""
                                    "    ],"
                                    "    \"item\":"
                                    "    {"
                                    "      \"type\": \"Container\","
                                    "      \"id\": \"ctr\","
                                    "      \"width\": \"100%\","
                                    "      \"height\": \"100%\","
                                    "      \"items\": ["
                                    "        {"
                                    "          \"type\": \"Text\","
                                    "          \"id\": \"empty\","
                                    "          \"text\": \"text\","
                                    "          \"entities\": [\"entity\"]"
                                    "        }"
                                    "      ]"
                                    "    }"
                                    "  }"
                                    "}";

TEST_F(VisualContextTest, TypePropagate) {
    loadDocument(TYPE_PROPAGATE, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(1, visualContext["children"].Size());

    auto& c1 = visualContext["children"][0];
    ASSERT_STREQ("empty", c1["id"].GetString());
    ASSERT_STREQ("text", c1["type"].GetString());
}

static const char* OPACITY = "{"
                             "  \"type\": \"APL\","
                             "  \"version\": \"1.1\","
                             "  \"mainTemplate\": {"
                             "    \"parameters\": ["
                             "      \"payload\""
                             "    ],"
                             "    \"item\": {"
                             "      \"type\": \"Container\","
                             "      \"id\": \"ctr\","
                             "      \"width\": \"100%\","
                             "      \"height\": \"100%\","
                             "      \"opacity\": 0.5,"
                             "      \"items\": ["
                             "        {"
                             "          \"type\": \"Container\","
                             "          \"id\": \"ctr\","
                             "          \"width\": \"100%\","
                             "          \"height\": \"100%\","
                             "          \"opacity\": 0.5,"
                             "          \"items\": ["
                             "            {"
                             "              \"type\": \"Text\","
                             "              \"id\": \"text\","
                             "              \"text\": \"Magic visible text.\","
                             "              \"entities\": [\"blah\"],"
                             "              \"opacity\": 1.0"
                             "            }"
                             "          ]"
                             "        }"
                             "      ]"
                             "    }"
                             "  }"
                             "}";

TEST_F(VisualContextTest, Opacity) {
    loadDocument(OPACITY, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_EQ(0.5, visualContext["visibility"]);
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(1, visualContext["children"].GetArray().Size());
    auto& opaqueChild = visualContext["children"][0];

    ASSERT_EQ(0.25, opaqueChild["visibility"]);
}

static const char* LAYERING_DEEP = "{"
                                   "  \"type\": \"APL\","
                                   "  \"version\": \"1.1\","
                                   "  \"mainTemplate\": {"
                                   "    \"parameters\": ["
                                   "      \"payload\""
                                   "    ],"
                                   "    \"item\": {"
                                   "      \"type\": \"Container\","
                                   "      \"id\": \"ctr\","
                                   "      \"width\": \"100%\","
                                   "      \"height\": \"100%\","
                                   "      \"items\": ["
                                   "        {"
                                   "          \"type\": \"Text\","
                                   "          \"id\": \"text1\","
                                   "          \"height\": \"100dp\","
                                   "          \"width\": \"100dp\","
                                   "          \"position\": \"absolute\","
                                   "          \"left\": \"10dp\","
                                   "          \"top\": \"10dp\","
                                   "          \"text\": \"Background.\","
                                   "          \"entities\": [\"blah\"]"
                                   "        },"
                                   "        {"
                                   "          \"type\": \"Text\","
                                   "          \"id\": \"text2\","
                                   "          \"height\": \"100dp\","
                                   "          \"width\": \"100dp\","
                                   "          \"position\": \"absolute\","
                                   "          \"left\": \"20dp\","
                                   "          \"top\": \"20dp\","
                                   "          \"text\": \"Middle.\","
                                   "          \"entities\": [\"blah\"]"
                                   "        },"
                                   "        {"
                                   "          \"type\": \"Text\","
                                   "          \"id\": \"text3\","
                                   "          \"height\": \"100dp\","
                                   "          \"width\": \"100dp\","
                                   "          \"position\": \"absolute\","
                                   "          \"left\": \"30dp\","
                                   "          \"top\": \"30dp\","
                                   "          \"text\": \"Forward.\","
                                   "          \"entities\": [\"blah\"]"
                                   "        }"
                                   "      ]"
                                   "    }"
                                   "  }"
                                   "}";

TEST_F(VisualContextTest, LayeringDeep) {
    loadDocument(LAYERING_DEEP, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(3, visualContext["children"].GetArray().Size());
    auto& child1 = visualContext["children"][0];
    ASSERT_STREQ("100x100+10+10:0", child1["position"].GetString());
    auto& child2 = visualContext["children"][1];
    ASSERT_STREQ("100x100+20+20:1", child2["position"].GetString());
    auto& child3 = visualContext["children"][2];
    ASSERT_STREQ("100x100+30+30:2", child3["position"].GetString());
}

static const char* LAYERING_ONE = "{"
                                  "  \"type\": \"APL\","
                                  "  \"version\": \"1.1\","
                                  "  \"mainTemplate\": {"
                                  "    \"parameters\": ["
                                  "      \"payload\""
                                  "    ],"
                                  "    \"item\": {"
                                  "      \"type\": \"Container\","
                                  "      \"id\": \"ctr\","
                                  "      \"width\": \"100%\","
                                  "      \"height\": \"100%\","
                                  "      \"items\": ["
                                  "        {"
                                  "          \"type\": \"Text\","
                                  "          \"id\": \"text1\","
                                  "          \"height\": \"100dp\","
                                  "          \"width\": \"100dp\","
                                  "          \"position\": \"absolute\","
                                  "          \"left\": \"100dp\","
                                  "          \"top\": \"100dp\","
                                  "          \"text\": \"Background.\","
                                  "          \"entities\": [\"blah\"]"
                                  "        },"
                                  "        {"
                                  "          \"type\": \"Text\","
                                  "          \"id\": \"text2\","
                                  "          \"height\": \"100dp\","
                                  "          \"width\": \"100dp\","
                                  "          \"position\": \"absolute\","
                                  "          \"left\": \"50dp\","
                                  "          \"top\": \"50dp\","
                                  "          \"text\": \"Middle.\","
                                  "          \"entities\": [\"blah\"]"
                                  "        },"
                                  "        {"
                                  "          \"type\": \"Text\","
                                  "          \"id\": \"text3\","
                                  "          \"height\": \"100dp\","
                                  "          \"width\": \"100dp\","
                                  "          \"position\": \"absolute\","
                                  "          \"left\": \"200dp\","
                                  "          \"top\": \"200dp\","
                                  "          \"text\": \"Forward.\","
                                  "          \"entities\": [\"blah\"]"
                                  "        }"
                                  "      ]"
                                  "    }"
                                  "  }"
                                  "}";

TEST_F(VisualContextTest, LayeringOne) {
    loadDocument(LAYERING_ONE, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(3, visualContext["children"].GetArray().Size());
    auto& child1 = visualContext["children"][0];
    ASSERT_STREQ("100x100+100+100:0", child1["position"].GetString());
    auto& child2 = visualContext["children"][1];
    ASSERT_STREQ("100x100+50+50:1", child2["position"].GetString());
    auto& child3 = visualContext["children"][2];
    ASSERT_STREQ("100x100+200+200:0", child3["position"].GetString());
}

static const char* LAYERING_SINGLE = "{"
                                     "  \"type\": \"APL\","
                                     "  \"version\": \"1.1\","
                                     "  \"mainTemplate\": {"
                                     "    \"parameters\": ["
                                     "      \"payload\""
                                     "    ],"
                                     "    \"item\": {"
                                     "      \"type\": \"Container\","
                                     "      \"id\": \"ctr\","
                                     "      \"width\": \"100%\","
                                     "      \"height\": \"100%\","
                                     "      \"items\": ["
                                     "        {"
                                     "          \"type\": \"Text\","
                                     "          \"id\": \"text1\","
                                     "          \"height\": \"100dp\","
                                     "          \"width\": \"100dp\","
                                     "          \"position\": \"absolute\","
                                     "          \"left\": \"100dp\","
                                     "          \"top\": \"100dp\","
                                     "          \"text\": \"Background.\","
                                     "          \"entities\": [\"blah\"]"
                                     "        }"
                                     "      ]"
                                     "    }"
                                     "  }"
                                     "}";

TEST_F(VisualContextTest, LayeringSingle) {
    loadDocument(LAYERING_SINGLE, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(1, visualContext["children"].GetArray().Size());
    auto& child = visualContext["children"][0];
    ASSERT_STREQ("100x100+100+100:0", child["position"].GetString());
}

static const char* LAYERING_TWO = "{"
                                  "  \"type\": \"APL\","
                                  "  \"version\": \"1.1\","
                                  "  \"mainTemplate\": {"
                                  "    \"parameters\": ["
                                  "      \"payload\""
                                  "    ],"
                                  "    \"item\": {"
                                  "      \"type\": \"Container\","
                                  "      \"id\": \"ctr\","
                                  "      \"width\": \"100%\","
                                  "      \"height\": \"100%\","
                                  "      \"items\": ["
                                  "        {"
                                  "          \"type\": \"Text\","
                                  "          \"id\": \"text1\","
                                  "          \"height\": \"100dp\","
                                  "          \"width\": \"100dp\","
                                  "          \"position\": \"absolute\","
                                  "          \"left\": \"100dp\","
                                  "          \"top\": \"100dp\","
                                  "          \"text\": \"Background.\","
                                  "          \"entities\": [\"blah\"]"
                                  "        },"
                                  "        {"
                                  "          \"type\": \"Text\","
                                  "          \"id\": \"text2\","
                                  "          \"height\": \"100dp\","
                                  "          \"width\": \"100dp\","
                                  "          \"position\": \"absolute\","
                                  "          \"left\": \"50dp\","
                                  "          \"top\": \"50dp\","
                                  "          \"text\": \"Middle.\","
                                  "          \"entities\": [\"blah\"]"
                                  "        },"
                                  "        {"
                                  "          \"type\": \"Text\","
                                  "          \"id\": \"text3\","
                                  "          \"height\": \"100dp\","
                                  "          \"width\": \"100dp\","
                                  "          \"position\": \"absolute\","
                                  "          \"left\": \"150dp\","
                                  "          \"top\": \"150dp\","
                                  "          \"text\": \"Forward.\","
                                  "          \"entities\": [\"blah\"]"
                                  "        }"
                                  "      ]"
                                  "    }"
                                  "  }"
                                  "}";

TEST_F(VisualContextTest, LayeringTwo) {
    loadDocument(LAYERING_TWO, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(3, visualContext["children"].GetArray().Size());
    auto& child1 = visualContext["children"][0];
    ASSERT_STREQ("100x100+100+100:0", child1["position"].GetString());
    auto& child2 = visualContext["children"][1];
    ASSERT_STREQ("100x100+50+50:1", child2["position"].GetString());
    auto& child3 = visualContext["children"][2];
    ASSERT_STREQ("100x100+150+150:1", child3["position"].GetString());
}

static const char* LAYERING_INC = "{"
                                  "  \"type\": \"APL\","
                                  "  \"version\": \"1.1\","
                                  "  \"mainTemplate\": {"
                                  "    \"parameters\": ["
                                  "      \"payload\""
                                  "    ],"
                                  "    \"item\": {"
                                  "      \"type\": \"Container\","
                                  "      \"id\": \"ctr\","
                                  "      \"width\": \"100%\","
                                  "      \"height\": \"100%\","
                                  "      \"items\": ["
                                  "        {"
                                  "          \"type\": \"Text\","
                                  "          \"id\": \"text1\","
                                  "          \"height\": \"100dp\","
                                  "          \"width\": \"100dp\","
                                  "          \"position\": \"absolute\","
                                  "          \"left\": \"100dp\","
                                  "          \"top\": \"100dp\","
                                  "          \"text\": \"Background.\","
                                  "          \"entities\": [\"blah\"]"
                                  "        },"
                                  "        {"
                                  "          \"type\": \"Container\","
                                  "          \"id\": \"ctr2\","
                                  "          \"height\": \"100dp\","
                                  "          \"width\": \"100dp\","
                                  "          \"position\": \"absolute\","
                                  "          \"left\": \"50dp\","
                                  "          \"top\": \"50dp\","
                                  "          \"items\":"
                                  "          ["
                                  "            {"
                                  "              \"type\": \"Text\","
                                  "              \"id\": \"text3\","
                                  "              \"height\": \"100%\","
                                  "              \"width\": \"100%\","
                                  "              \"text\": \"Forward.\","
                                  "              \"entities\": [\"blah\"]"
                                  "            }"
                                  "          ]"
                                  "        }"
                                  "      ]"
                                  "    }"
                                  "  }"
                                  "}";

TEST_F(VisualContextTest, LayeringIncapsulated) {
    loadDocument(LAYERING_INC, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(2, visualContext["children"].GetArray().Size());
    auto& child1 = visualContext["children"][0];
    ASSERT_STREQ("100x100+100+100:0", child1["position"].GetString());
    auto& child2 = visualContext["children"][1];
    ASSERT_STREQ("100x100+50+50:1", child2["position"].GetString());
}

static const char* OPACITY_CHANGE = "{"
                                    "  \"type\": \"APL\","
                                    "  \"version\": \"1.1\","
                                    "  \"mainTemplate\": {"
                                    "    \"parameters\": ["
                                    "      \"payload\""
                                    "    ],"
                                    "    \"item\": {"
                                    "      \"type\": \"Container\","
                                    "      \"id\": \"ctr\","
                                    "      \"width\": \"100%\","
                                    "      \"height\": \"157dp\","
                                    "      \"items\": ["
                                    "        {"
                                    "          \"type\": \"Text\","
                                    "          \"id\": \"item_0\","
                                    "          \"text\": \"Text.\","
                                    "          \"entities\": [\"entity\"],"
                                    "          \"opacity\": 0.0"
                                    "        }"
                                    "      ]"
                                    "    }"
                                    "  }"
                                    "}";

TEST_F(VisualContextTest, OpacityChange) {
    loadDocument(OPACITY_CHANGE, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_FALSE(visualContext.HasMember("children"));

    // Change opacity
    component->getCoreChildAt(0)->setProperty(kPropertyOpacity, 1.0);
    root->clearPending();

    ASSERT_TRUE(CheckDirtyVisualContext(root,  component->getCoreChildAt(0)));
    serializeVisualContext();

    // Check children
    ASSERT_EQ(1, visualContext["children"].GetArray().Size());
    auto& child = visualContext["children"][0];
    ASSERT_STREQ("item_0", child["id"].GetString());
    ASSERT_TRUE(child.HasMember("entities"));

    // Change parent opacity
    component->setProperty(kPropertyOpacity, 0.0);
    root->clearPending();

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    serializeVisualContext();

    // Check children
    ASSERT_FALSE(visualContext.HasMember("children"));
}

static const char* DISPLAY_CHANGE = "{"
                                    "  \"type\": \"APL\","
                                    "  \"version\": \"1.1\","
                                    "  \"mainTemplate\": {"
                                    "    \"parameters\": ["
                                    "      \"payload\""
                                    "    ],"
                                    "    \"item\": {"
                                    "      \"type\": \"Container\","
                                    "      \"id\": \"ctr\","
                                    "      \"width\": \"100%\","
                                    "      \"height\": \"157dp\","
                                    "      \"items\": ["
                                    "        {"
                                    "          \"type\": \"Text\","
                                    "          \"id\": \"item_0\","
                                    "          \"text\": \"Text.\","
                                    "          \"entities\": [\"entity\"]"
                                    "        }"
                                    "      ]"
                                    "    }"
                                    "  }"
                                    "}";

TEST_F(VisualContextTest, DisplayChange) {
    loadDocument(DISPLAY_CHANGE, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(1, visualContext["children"].GetArray().Size());
    auto& child = visualContext["children"][0];
    ASSERT_STREQ("item_0", child["id"].GetString());
    ASSERT_TRUE(child.HasMember("entities"));

    // Change display
    component->getCoreChildAt(0)->setProperty(kPropertyDisplay, "invisible");
    root->clearPending();
    serializeVisualContext();

    // Check children
    ASSERT_FALSE(visualContext.HasMember("children"));

    // Change parent display
    component->getCoreChildAt(0)->setProperty(kPropertyDisplay, "normal");
    component->setProperty(kPropertyDisplay, "invisible");
    root->clearPending();

    // Check children
    ASSERT_FALSE(visualContext.HasMember("children"));

    root->clearPending();
}

static const char* LAYOUT_CHANGE = "{"
                                   "  \"type\": \"APL\","
                                   "  \"version\": \"1.1\","
                                   "  \"mainTemplate\": {"
                                   "    \"parameters\": ["
                                   "      \"payload\""
                                   "    ],"
                                   "    \"item\": {"
                                   "      \"type\": \"Container\","
                                   "      \"id\": \"ctr\","
                                   "      \"width\": \"50dp\","
                                   "      \"height\": \"50dp\","
                                   "      \"direction\": \"column\","
                                   "      \"items\": ["
                                   "        {"
                                   "          \"type\": \"Text\","
                                   "          \"id\": \"item_0\","
                                   "          \"text\": \"Text.\","
                                   "          \"shrink\": 1,"
                                   "          \"entities\": [\"entity\"]"
                                   "        }"
                                   "      ]"
                                   "    }"
                                   "  }"
                                   "}";

class VCTextMeasure : public TextMeasurement {
public:
    LayoutSize measure(Component* component, float width, MeasureMode widthMode, float height,
                       MeasureMode heightMode) override {
        int symbols = component->getCalculated(kPropertyText).asString().size();
        int line = std::min(static_cast<int>(width), symbols * 10);
        int count = symbols * 10 / line;

        float resultingWidth = line;
        float resultingHeight = count * 10;

        return LayoutSize({resultingWidth, resultingHeight});
    }

    float baseline(Component* component, float width, float height) override { return height; }
};

TEST_F(VisualContextTest, LayoutChange) {
    config->measure(std::make_shared<VCTextMeasure>());
    loadDocument(LAYOUT_CHANGE, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));
    ASSERT_STREQ("text", visualContext["type"].GetString());

    // Check children
    ASSERT_EQ(1, visualContext["children"].GetArray().Size());
    auto& child = visualContext["children"][0];
    ASSERT_STREQ("item_0", child["id"].GetString());
    ASSERT_TRUE(child.HasMember("entities"));
    ASSERT_EQ("50x10+0+0:0", child["position"]);

    // Enlarge text that actually changes layout.
    component->getCoreChildAt(0)->setProperty(kPropertyText, "Much longer text.");
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    root->clearDirty();
    ASSERT_TRUE(CheckDirtyVisualContext(root, component->getCoreChildAt(0)));
    serializeVisualContext();

    // Check children
    ASSERT_EQ(1, visualContext["children"].GetArray().Size());
    child = visualContext["children"][0];
    ASSERT_STREQ("item_0", child["id"].GetString());
    ASSERT_TRUE(child.HasMember("entities"));
    ASSERT_EQ("50x30+0+0:0", child["position"]);
}

static const char* EDIT_TEXT_LAYOUT_CHANGE = R"(
{
    "type":"APL",
    "version":"1.4",
    "mainTemplate":{
        "parameters":[
            "payload"
        ],
        "item":{
            "type":"Container",
            "id":"ctr",
            "width":"50dp",
            "height":"50dp",
            "direction":"column",
            "items":[
                {
                    "type":"EditText",
                    "id":"item_0",
                    "text":"Text.",
                    "shrink":1,
                    "entities":[
                        "entity"
                    ]
                }
            ]
        }
    }
}
)";

TEST_F(VisualContextTest, EditTextLayoutChange) {
    config->measure(std::make_shared<VCTextMeasure>());
    loadDocument(EDIT_TEXT_LAYOUT_CHANGE, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Check parent
    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_STREQ("ctr", visualContext["id"].GetString());
    ASSERT_TRUE(visualContext["tags"].HasMember("viewport"));

    // Check children
    ASSERT_EQ(1, visualContext["children"].GetArray().Size());
    auto& child = visualContext["children"][0];
    ASSERT_STREQ("item_0", child["id"].GetString());
    ASSERT_TRUE(child.HasMember("entities"));
    ASSERT_EQ("50x10+0+0:0", child["position"]);

    // Enlarge text that should not change layout.
    component->getCoreChildAt(0)->setProperty(kPropertyText, "Much longer text.");
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    root->clearDirty();
    ASSERT_TRUE(CheckDirtyVisualContext(root, component->getCoreChildAt(0)));
    serializeVisualContext();

    // Check children
    ASSERT_EQ(1, visualContext["children"].GetArray().Size());
    child = visualContext["children"][0];
    ASSERT_STREQ("item_0", child["id"].GetString());
    ASSERT_TRUE(child.HasMember("entities"));
    ASSERT_EQ("50x10+0+0:0", child["position"]);
}

static const char* GRID_SEQUENCE_WITH_HOLE = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "GridSequence",
          "width": 400,
          "height": 400,
          "childHeights": 100,
          "childWidths": 200,
          "item": {
            "type": "TouchWrapper",
            "id": "Item{index}",
            "width": "100%",
            "height": "100%",
            "opacity": "${index == 3 ? 0 : 1}"
          },
          "data": [
            0,
            1,
            2,
            3,
            4,
            5
          ]
        }
      }
    }
)apl";

TEST_F(VisualContextTest, GridHole) {
    loadDocument(GRID_SEQUENCE_WITH_HOLE);
    ASSERT_TRUE(component);

    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_TRUE(visualContext["tags"].HasMember("list"));

    const auto& list = visualContext["tags"]["list"];
    ASSERT_TRUE(list.HasMember("itemCount"));
    ASSERT_EQ(6, list["itemCount"].GetInt());
    ASSERT_TRUE(list.HasMember("lowestIndexSeen"));
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_TRUE(list.HasMember("highestIndexSeen"));
    ASSERT_EQ(5, list["highestIndexSeen"].GetInt());
}

static const char* SEQUENCE_WITH_HOLE = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Sequence",
          "width": 400,
          "height": 600,
          "item": {
            "type": "TouchWrapper",
            "id": "Item{index}",
            "width": "100%",
            "height": 100,
            "opacity": "${index == 3 ? 0 : 1}"
          },
          "data": [
            0,
            1,
            2,
            3,
            4,
            5
          ]
        }
      }
    }
)apl";

TEST_F(VisualContextTest, SequenceHole) {
    loadDocument(SEQUENCE_WITH_HOLE);
    ASSERT_TRUE(component);

    ASSERT_TRUE(visualContext.HasMember("tags"));
    ASSERT_TRUE(visualContext["tags"].HasMember("list"));

    const auto& list = visualContext["tags"]["list"];
    ASSERT_TRUE(list.HasMember("itemCount"));
    ASSERT_EQ(6, list["itemCount"].GetInt());
    ASSERT_TRUE(list.HasMember("lowestIndexSeen"));
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_TRUE(list.HasMember("highestIndexSeen"));
    ASSERT_EQ(5, list["highestIndexSeen"].GetInt());
}


/**
 * The visual context dirty state propagates from child to parent.
 */
TEST_F(VisualContextTest, IsDirtyBasic) {
    loadDocument(BASIC);
    ASSERT_EQ(kComponentTypeTouchWrapper, component->getType());


    auto txt = component->getCoreChildAt(0);
    ASSERT_TRUE(txt);

    // change the child, verify child and parent tree
    txt->setProperty(kPropertyText, "spud");
    ASSERT_TRUE(CheckDirtyVisualContext(root, txt));

    // serialize vc verify all are clean
    serializeVisualContext();
    ASSERT_FALSE(CheckDirtyVisualContext(root, txt));
}


/**
 * A dirty parent makes child dirty.
 */
TEST_F(VisualContextTest, IsDirtySubTree) {
    loadDocument(SEQUENCE, DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    auto txt0 = component->getCoreChildAt(0);
    ASSERT_TRUE(txt0);
    auto txt1 = component->getCoreChildAt(1);
    ASSERT_TRUE(txt1);

    // change first child verify whole tree is dirty
    txt0->setProperty(kPropertyText, "spud");
    ASSERT_TRUE(CheckDirtyVisualContext(root, txt0));

}


/**
 * Serialize top component visual context clears the whole tree dirty state.
 */
TEST_F(VisualContextTest, SerializeClearsTree) {
    loadDocument(SEQUENCE, DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    auto txt0 = component->getCoreChildAt(0);
    ASSERT_TRUE(txt0);
    auto txt1 = component->getCoreChildAt(1);
    ASSERT_TRUE(txt1);

    // change first child verify whole tree is dirty
    txt0->setProperty(kPropertyText, "spud");
    ASSERT_TRUE(CheckDirtyVisualContext(root, txt0));

    serializeVisualContext();
    ASSERT_FALSE(CheckDirtyVisualContext(root, txt0, txt1));
}

static const char *ODD_DPI = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "red",
      "entities": ["one potato"],
      "item": {
        "type": "Frame",
        "backgroundColor": "green",
        "width": "100%",
        "height": "200%",
        "entities": ["two potato"]
      }
    }
  }
})";

TEST_F(VisualContextTest, OddDPI) {
    metrics.dpi(213).size(960, 600);
    loadDocument(ODD_DPI);

    serializeVisualContext();

    // Check parent
    ASSERT_FALSE(visualContext.HasMember("visibility"));

    auto& child = visualContext["children"][0];
    ASSERT_EQ(0.5, child["visibility"].GetDouble());
}



