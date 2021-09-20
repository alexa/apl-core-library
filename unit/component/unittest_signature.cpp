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

#include "apl/engine/evaluate.h"

#include "../testeventloop.h"

using namespace apl;

static const char *DATA = "{"
                          "\"title\": \"Pecan Pie V\""
                          "}";

class SignatureTest : public DocumentWrapper {};

static const char *SIMPLE_LAYOUT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"layouts\": {"
    "    \"SimpleLayout\": {"
    "      \"parameters\": [],"
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"text\": \"${payload.title}\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"item\": {"
    "      \"type\": \"SimpleLayout\""
    "    }"
    "  }"
    "}";

TEST_F(SignatureTest, Simple)
{
    loadDocument(SIMPLE_LAYOUT, DATA);
    ASSERT_STREQ("T", component->getHierarchySignature().c_str());
}

static const char *SEQUENCE_LAYOUT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Sequence\","
    "      \"data\": ["
    "        1,"
    "        0"
    "      ],"
    "      \"items\": ["
    "        {"
    "          \"when\": \"${data == 0}\","
    "          \"type\": \"Image\""
    "        },"
    "        {"
    "          \"type\": \"Container\","
    "          \"items\": ["
    "            {"
    "              \"type\": \"Frame\","
    "              \"items\": {"
    "                \"type\": \"Video\""
    "              }"
    "            }"
    "          ]"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";


TEST_F(SignatureTest, Sequence)
{
    loadDocument(SEQUENCE_LAYOUT);
    ASSERT_STREQ("Q[C[F[V]]I]", component->getHierarchySignature().c_str());
    ASSERT_STREQ("C[F[V]]", component->getChildAt(0)->getHierarchySignature().c_str());
    ASSERT_STREQ("I", component->getChildAt(1)->getHierarchySignature().c_str());
}

static const char *PAGER_LAYOUT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Pager\","
    "      \"data\": ["
    "        0,"
    "        1"
    "      ],"
    "      \"items\": ["
    "        {"
    "          \"when\": \"${data == 0}\","
    "          \"type\": \"ScrollView\","
    "          \"items\": {"
    "            \"type\": \"TouchWrapper\","
    "            \"items\": {"
    "              \"type\": \"Image\""
    "            }"
    "          }"
    "        },"
    "        {"
    "          \"type\": \"ScrollView\","
    "          \"items\": ["
    "            {"
    "              \"type\": \"TouchWrapper\","
    "              \"items\": {"
    "                \"type\": \"Text\""
    "              }"
    "            }"
    "          ]"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(SignatureTest, Pager)
{
    loadDocument(PAGER_LAYOUT);
    advanceTime(10);
    ASSERT_STREQ("P[S[W[I]]S[W[T]]]", component->getHierarchySignature().c_str());
    ASSERT_STREQ("S[W[I]]", component->getChildAt(0)->getHierarchySignature().c_str());
    ASSERT_STREQ("S[W[T]]", component->getChildAt(1)->getHierarchySignature().c_str());
}

static const char *EDITTEXT_LAYOUT =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"EditText\""
        "    }"
        "  }"
        "}";

TEST_F(SignatureTest, EditText) {
    loadDocument(EDITTEXT_LAYOUT);
    ASSERT_STREQ("E", component->getHierarchySignature().c_str());
}



TEST_F(SignatureTest, TypesAndString)
{
    unsigned long len = std::string("CEXFMIPSQTWGV").length();
    for (ComponentType type : {kComponentTypeContainer,
                               kComponentTypeEditText,
                               kComponentTypeExtension,
                               kComponentTypeFrame,
                               kComponentTypeGridSequence,
                               kComponentTypeImage,
                               kComponentTypePager,
                               kComponentTypeScrollView,
                               kComponentTypeSequence,
                               kComponentTypeText,
                               kComponentTypeTouchWrapper,
                               kComponentTypeVectorGraphic,
                               kComponentTypeVideo}) {
        ASSERT_TRUE(type < len);
    }
}

static const char *EXTENSION_COMPONENT_LAYOUT = R"({
        "type": "APL",
        "version": "1.7",
        "extensions": [
          {
            "uri": "ext:cmp:1",
            "name": "Ext"
          }
        ],
        "mainTemplate": {
          "item": {
            "type": "Ext:ExtensionComponent"
          }
        }
      })";

TEST_F(SignatureTest, ExtensionComponent) {
    ExtensionComponentDefinition componentDef = ExtensionComponentDefinition("ext:cmp:1", "ExtensionComponent");
    config->registerExtensionComponent(componentDef);

    loadDocument(EXTENSION_COMPONENT_LAYOUT);
    ASSERT_EQ(kComponentTypeExtension, component->getType());
    ASSERT_STREQ("X", component->getHierarchySignature().c_str());
}

static const char *VIDEO_LAYOUT =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"Video\""
        "    }"
        "  }"
        "}";

TEST_F(SignatureTest, Video) {
    // video is the current last component int the list,
    // testing it is likely to catch signature assignment issues
    loadDocument(VIDEO_LAYOUT);
    ASSERT_EQ(kComponentTypeVideo, component->getType());
    ASSERT_STREQ("V", component->getHierarchySignature().c_str());
}