/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <iostream>
#include <sstream>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include "testeventloop.h"

using namespace apl;

class SerializeTest : public DocumentWrapper {
protected:
    static void checkCommonProperties(const ComponentPtr& component, rapidjson::Value& json)
    {
        ASSERT_EQ(component->getUniqueId(), json["id"].GetString());
        ASSERT_EQ(component->getType(), json["type"].GetInt());
        ASSERT_EQ(component->getCalculated(kPropertyAccessibilityLabel), json["accessibilityLabel"].GetString());
        auto bounds = component->getCalculated(kPropertyBounds).getRect();
        ASSERT_EQ(bounds.getX(), json["_bounds"][0].GetFloat());
        ASSERT_EQ(bounds.getY(), json["_bounds"][1].GetFloat());
        ASSERT_EQ(bounds.getWidth(), json["_bounds"][2].GetFloat());
        ASSERT_EQ(bounds.getHeight(), json["_bounds"][3].GetFloat());
        ASSERT_EQ(component->getCalculated(kPropertyChecked), json["checked"].GetBool());
        ASSERT_EQ(component->getCalculated(kPropertyDisabled), json["disabled"].GetBool());
        ASSERT_EQ(component->getCalculated(kPropertyDisplay), json["display"].GetDouble());
        auto innerBounds = component->getCalculated(kPropertyInnerBounds).getRect();
        ASSERT_EQ(innerBounds.getX(), json["_innerBounds"][0].GetFloat());
        ASSERT_EQ(innerBounds.getY(), json["_innerBounds"][1].GetFloat());
        ASSERT_EQ(innerBounds.getWidth(), json["_innerBounds"][2].GetFloat());
        ASSERT_EQ(innerBounds.getHeight(), json["_innerBounds"][3].GetFloat());
        ASSERT_EQ(component->getCalculated(kPropertyOpacity), json["opacity"].GetDouble());
        auto transform = component->getCalculated(kPropertyTransform).getTransform2D();
        ASSERT_EQ(transform.get().at(0), json["_transform"][0].GetFloat());
        ASSERT_EQ(transform.get().at(1), json["_transform"][1].GetFloat());
        ASSERT_EQ(transform.get().at(2), json["_transform"][2].GetFloat());
        ASSERT_EQ(transform.get().at(3), json["_transform"][3].GetFloat());
        ASSERT_EQ(transform.get().at(4), json["_transform"][4].GetFloat());
        ASSERT_EQ(transform.get().at(5), json["_transform"][5].GetFloat());
        ASSERT_EQ(component->getCalculated(kPropertyUser).size(), json["_user"].MemberCount());
    }
};

static const char *SERIALIZE_COMPONENTS = 
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Container\","
        "      \"width\": \"100%\","
        "      \"height\": \"100%\","
        "      \"numbered\": true,"
        "      \"items\": ["
        "        {"
        "          \"type\": \"Image\","
        "          \"id\": \"image\","
        "          \"source\": \"http://images.amazon.com/image/foo.png\","
        "          \"overlayColor\": \"red\","
        "          \"overlayGradient\": {"
        "            \"colorRange\": ["
        "              \"blue\","
        "              \"red\""
        "            ]"
        "          },"
        "          \"filters\": {"
        "            \"type\": \"Blur\","
        "            \"radius\": 22"
        "          }"
        "        },"
        "        {"
        "          \"type\": \"Text\","
        "          \"id\": \"text\","
        "          \"text\": \"<b>Styled</b> <i>text</i>\""
        "        },"
        "        {"
        "          \"type\": \"ScrollView\","
        "          \"id\": \"scroll\""
        "        },"
        "        {"
        "          \"type\": \"Frame\","
        "          \"id\": \"frame\","
        "          \"backgroundColor\": \"red\","
        "          \"borderColor\": \"blue\","
        "          \"borderBottomLeftRadius\": \"1dp\","
        "          \"borderBottomRightRadius\": \"2dp\","
        "          \"borderTopLeftRadius\": \"3dp\","
        "          \"borderTopRightRadius\": \"4dp\""
        "        },"
        "        {"
        "          \"type\": \"Sequence\","
        "          \"id\": \"sequence\""
        "        },"
        "        {"
        "          \"type\": \"TouchWrapper\","
        "          \"id\": \"touch\","
        "          \"onPress\": {"
        "            \"type\": \"SendEvent\","
        "            \"arguments\": [ "
        "              \"${event.source.handler}\","
        "              \"${event.source.value}\","
        "              \"${event.target.opacity}\" "
        "            ],"
        "            \"components\": [ \"text\" ]"
        "          }"
        "        },"
        "        {"
        "          \"type\": \"Pager\","
        "          \"id\": \"pager\""
        "        },"
        "        {"
        "          \"type\": \"VectorGraphic\","
        "          \"id\": \"vector\","
        "          \"source\": \"iconWifi3\""
        "        },"
        "        {"
        "          \"type\": \"Video\","
        "          \"id\": \"video\","
        "          \"source\": ["
        "            \"URL1\","
        "            {"
        "              \"url\": \"URL2\""
        "            },"
        "            {"
        "              \"description\": \"Sample video.\","
        "              \"duration\": 1000,"
        "              \"url\": \"URL3\","
        "              \"repeatCount\": 2,"
        "              \"offset\": 100"
        "            }"
        "          ]"
        "        }"
        "      ]"
        "    }"
        "  }"
        "}";

TEST_F(SerializeTest, Components)
{
    loadDocument(SERIALIZE_COMPONENTS);

    rapidjson::Document doc;
    auto json = component->serialize(doc.GetAllocator());

    ASSERT_EQ(kComponentTypeContainer, component->getType());
    checkCommonProperties(component, json);

    auto image = context->findComponentById("image");
    ASSERT_TRUE(image);
    auto& imageJson = json["children"][0];
    checkCommonProperties(image, imageJson);
    ASSERT_EQ(image->getCalculated(kPropertyAlign), imageJson["align"].GetDouble());
    ASSERT_EQ(image->getCalculated(kPropertyBorderRadius).getAbsoluteDimension(), imageJson["borderRadius"].GetDouble());
    auto filter = image->getCalculated(kPropertyFilters).getArray().at(0).getFilter();
    ASSERT_EQ(filter.getType(), imageJson["filters"][0]["type"]);
    ASSERT_EQ(filter.getValue(FilterProperty::kFilterPropertyRadius).getAbsoluteDimension(), imageJson["filters"][0]["radius"].GetDouble());
    ASSERT_EQ(image->getCalculated(kPropertyOverlayColor).getColor(), Color(session, imageJson["overlayColor"].GetString()));
    auto gradient = image->getCalculated(kPropertyOverlayGradient).getGradient();
    ASSERT_EQ(gradient.getType(), imageJson["overlayGradient"]["type"].GetDouble());
    ASSERT_EQ(gradient.getAngle(), imageJson["overlayGradient"]["angle"].GetDouble());
    ASSERT_EQ(gradient.getColorRange().size(), imageJson["overlayGradient"]["colorRange"].Size());
    ASSERT_EQ(gradient.getInputRange().size(), imageJson["overlayGradient"]["inputRange"].Size());
    ASSERT_EQ(image->getCalculated(kPropertyScale), imageJson["scale"].GetDouble());
    ASSERT_EQ(image->getCalculated(kPropertySource), imageJson["source"].GetString());

    auto text = context->findComponentById("text");
    ASSERT_TRUE(text);
    auto& textJson = json["children"][1];
    checkCommonProperties(text, textJson);
    ASSERT_EQ(text->getCalculated(kPropertyColor).getColor(), Color(session, textJson["color"].GetString()));
    ASSERT_EQ(text->getCalculated(kPropertyColorKaraokeTarget).getColor(), Color(session, textJson["_colorKaraokeTarget"].GetString()));
    ASSERT_EQ(text->getCalculated(kPropertyFontFamily), textJson["fontFamily"].GetString());
    ASSERT_EQ(text->getCalculated(kPropertyFontSize).getAbsoluteDimension(), textJson["fontSize"].GetDouble());
    ASSERT_EQ(text->getCalculated(kPropertyFontStyle), textJson["fontStyle"].GetDouble());
    ASSERT_EQ(text->getCalculated(kPropertyFontWeight), textJson["fontWeight"].GetDouble());
    ASSERT_EQ(text->getCalculated(kPropertyLetterSpacing).getAbsoluteDimension(), textJson["letterSpacing"].GetDouble());
    ASSERT_EQ(text->getCalculated(kPropertyLineHeight), textJson["lineHeight"].GetDouble());
    ASSERT_EQ(text->getCalculated(kPropertyMaxLines), textJson["maxLines"].GetDouble());
    auto styledText = text->getCalculated(kPropertyText).getStyledText();
    ASSERT_EQ(styledText.getText(), textJson["text"]["text"].GetString());
    ASSERT_EQ(styledText.getSpans().size(), textJson["text"]["spans"].Size());
    ASSERT_EQ(text->getCalculated(kPropertyTextAlign), textJson["textAlign"].GetDouble());
    ASSERT_EQ(text->getCalculated(kPropertyTextAlignVertical), textJson["textAlignVertical"].GetDouble());

    auto scroll = context->findComponentById("scroll");
    ASSERT_TRUE(scroll);
    auto& scrollJson = json["children"][2];
    checkCommonProperties(scroll, scrollJson);

    auto frame = context->findComponentById("frame");
    ASSERT_TRUE(frame);
    auto& frameJson = json["children"][3];
    checkCommonProperties(frame, frameJson);
    ASSERT_EQ(frame->getCalculated(kPropertyBackgroundColor).getColor(), Color(session, frameJson["backgroundColor"].GetString()));
    auto radii = frame->getCalculated(kPropertyBorderRadii).getRadii();
    ASSERT_EQ(radii.get().at(0), frameJson["_borderRadii"][0].GetFloat());
    ASSERT_EQ(radii.get().at(1), frameJson["_borderRadii"][1].GetFloat());
    ASSERT_EQ(radii.get().at(2), frameJson["_borderRadii"][2].GetFloat());
    ASSERT_EQ(radii.get().at(3), frameJson["_borderRadii"][3].GetFloat());
    ASSERT_EQ(frame->getCalculated(kPropertyBorderColor).getColor(), Color(session, frameJson["borderColor"].GetString()));
    ASSERT_EQ(frame->getCalculated(kPropertyBorderWidth).getAbsoluteDimension(), frameJson["borderWidth"].GetDouble());

    auto sequence = context->findComponentById("sequence");
    ASSERT_TRUE(sequence);
    auto& sequenceJson = json["children"][4];
    checkCommonProperties(sequence, sequenceJson);
    ASSERT_EQ(sequence->getCalculated(kPropertyScrollDirection), sequenceJson["scrollDirection"].GetDouble());

    auto touch = context->findComponentById("touch");
    ASSERT_TRUE(touch);
    auto& touchJson = json["children"][5];
    checkCommonProperties(touch, touchJson);

    auto pager = context->findComponentById("pager");
    ASSERT_TRUE(pager);
    auto& pagerJson = json["children"][6];
    checkCommonProperties(pager, pagerJson);
    ASSERT_EQ(pager->getCalculated(kPropertyNavigation), pagerJson["navigation"].GetDouble());

    auto vector = context->findComponentById("vector");
    ASSERT_TRUE(vector);
    auto& vectorJson = json["children"][7];
    checkCommonProperties(vector, vectorJson);
    ASSERT_EQ(vector->getCalculated(kPropertyAlign), vectorJson["align"].GetDouble());
    //TODO: More sensible tests for vector graphics.
    ASSERT_TRUE(vectorJson["graphic"].IsNull());
    ASSERT_TRUE(vectorJson["mediaBounds"].IsNull());
    ASSERT_EQ(vector->getCalculated(kPropertyScale), vectorJson["scale"].GetDouble());
    ASSERT_EQ(vector->getCalculated(kPropertySource), vectorJson["source"].GetString());

    auto video = context->findComponentById("video");
    ASSERT_TRUE(video);
    auto& videoJson = json["children"][8];
    checkCommonProperties(video, videoJson);
    ASSERT_EQ(video->getCalculated(kPropertyAudioTrack), videoJson["audioTrack"].GetDouble());
    ASSERT_EQ(video->getCalculated(kPropertyAutoplay), videoJson["autoplay"].GetBool());
    ASSERT_EQ(video->getCalculated(kPropertyScale), videoJson["scale"].GetDouble());
    auto videoSource = video->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(3, videoSource.size());
    ASSERT_EQ(videoSource.size(), videoJson["source"].Size());
    auto source3 = videoSource.at(2).getMediaSource();
    ASSERT_EQ(source3.getUrl(), videoJson["source"][2]["url"].GetString());
    ASSERT_EQ(source3.getDescription(), videoJson["source"][2]["description"].GetString());
    ASSERT_EQ(source3.getDuration(), videoJson["source"][2]["duration"].GetInt());
    ASSERT_EQ(source3.getRepeatCount(), videoJson["source"][2]["repeatCount"].GetInt());
    ASSERT_EQ(source3.getOffset(), videoJson["source"][2]["offset"].GetInt());
}

TEST_F(SerializeTest, Dirty)
{
    loadDocument(SERIALIZE_COMPONENTS);

    ASSERT_EQ(kComponentTypeContainer, component->getType());
    auto text = std::static_pointer_cast<CoreComponent>(context->findComponentById("text"));
    ASSERT_TRUE(text);

    text->setProperty(kPropertyText, "Not very styled text.");

    rapidjson::Document doc;

    auto json = text->serializeDirty(doc.GetAllocator());

    ASSERT_EQ(2, json.MemberCount());
    ASSERT_STREQ("Not very styled text.", json["text"]["text"].GetString());
    ASSERT_TRUE(json["text"]["spans"].Empty());
}

TEST_F(SerializeTest, Event)
{
    loadDocument(SERIALIZE_COMPONENTS);

    ASSERT_EQ(kComponentTypeContainer, component->getType());
    auto text = std::static_pointer_cast<CoreComponent>(context->findComponentById("text"));
    ASSERT_TRUE(text);

    auto touch = context->findComponentById("touch");
    ASSERT_TRUE(touch);

    touch->update(kUpdatePressed);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    rapidjson::Document doc;

    auto json = event.serialize(doc.GetAllocator());

    //TODO: Unclear what we should do with actionRef in terms of serialization.
    ASSERT_EQ(4, json.MemberCount());
    ASSERT_EQ(kEventTypeSendEvent, json["type"].GetInt());
    ASSERT_STREQ("Press", json["arguments"][0].GetString());
    ASSERT_EQ(false, json["arguments"][1].GetBool());
    ASSERT_EQ(1.0, json["arguments"][2].GetDouble());

    ASSERT_TRUE(json["components"].HasMember("text"));
    ASSERT_STREQ("<b>Styled</b> <i>text</i>", json["components"]["text"].GetString());

    ASSERT_STREQ("Press", json["source"]["handler"].GetString());
    ASSERT_STREQ("touch", json["source"]["id"].GetString());
    ASSERT_STREQ("TouchWrapper", json["source"]["source"].GetString());
    ASSERT_EQ(touch->getUniqueId(), json["source"]["uid"].GetString());
    ASSERT_EQ(false, json["source"]["value"].GetBool());
}

const static char * SERIALIZE_ALL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"layouts\": {"
    "    \"MyLayout\": {"
    "      \"parameters\": \"MyText\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"text\": \"${MyText}\","
    "        \"width\": \"100%\","
    "        \"textAlign\": \"center\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"MyLayout\","
    "      \"MyText\": \"Hello\","
    "      \"width\": \"100%\","
    "      \"height\": \"50%\""
    "    }"
    "  }"
    "}";

const static char *SERIALIZE_ALL_RESULT =
    "{"
    "  \"type\": \"Text\","
    "  \"__id\": \"\","
    "  \"__inheritParentState\": false,"
    "  \"__style\": \"\","
    "  \"__path\": \"_main/layouts/MyLayout/items\","
    "  \"accessibilityLabel\": \"\","
    "  \"_bounds\": ["
    "    0.0,"
    "    0.0,"
    "    1280.0,"
    "    400.0"
    "  ],"
    "  \"checked\": false,"
    "  \"color\": \"#fafafaff\","
    "  \"_colorKaraokeTarget\": \"#fafafaff\","
    "  \"_colorNonKaraoke\": \"#fafafaff\","
    "  \"description\": \"\","
    "  \"disabled\": false,"
    "  \"display\": \"normal\","
    "  \"entities\": [],"
    "  \"fontFamily\": \"sans-serif\","
    "  \"fontSize\": 40.0,"
    "  \"fontStyle\": \"normal\","
    "  \"fontWeight\": \"normal\","
    "  \"height\": \"50%\","
    "  \"_innerBounds\": ["
    "    0.0,"
    "    0.0,"
    "    1280.0,"
    "    400.0"
    "  ],"
    "  \"letterSpacing\": 0.0,"
    "  \"lineHeight\": 1.25,"
    "  \"maxHeight\": null,"
    "  \"maxLines\": 0.0,"
    "  \"maxWidth\": null,"
    "  \"minHeight\": 0.0,"
    "  \"minWidth\": 0.0,"
    "  \"onMount\": [],"
    "  \"opacity\": 1.0,"
    "  \"paddingBottom\": 0.0,"
    "  \"paddingLeft\": 0.0,"
    "  \"paddingRight\": 0.0,"
    "  \"paddingTop\": 0.0,"
    "  \"shadowColor\": \"#00000000\","
    "  \"shadowHorizontalOffset\": 0.0,"
    "  \"shadowRadius\": 0.0,"
    "  \"shadowVerticalOffset\": 0.0,"
    "  \"speech\": \"\","
    "  \"text\": {"
    "    \"text\": \"Hello\","
    "    \"spans\": []"
    "  },"
    "  \"textAlign\": \"center\","
    "  \"textAlignVertical\": \"auto\","
    "  \"_transform\": ["
    "    1.0,"
    "    0.0,"
    "    0.0,"
    "    1.0,"
    "    0.0,"
    "    0.0"
    "  ],"
    "  \"transform\": null,"
    "  \"_user\": {},"
    "  \"width\": \"100%\","
    "  \"onCursorEnter\": [],"
    "  \"onCursorExit\": []"
    "}";

#include "rapidjson/filewritestream.h"

TEST_F(SerializeTest, SerializeAll)
{
    metrics.size(1280, 800);
    loadDocument(SERIALIZE_ALL);
    ASSERT_TRUE(component);

    rapidjson::Document doc;
    auto json = component->serializeAll(doc.GetAllocator());

    // Need to remove the "id" element - it changes depending on the number of unit tests executed.
    json.RemoveMember("id");

    // Load the JSON result above
    rapidjson::Document result;
    rapidjson::ParseResult ok = result.Parse(SERIALIZE_ALL_RESULT);
    ASSERT_TRUE(ok);

    // Compare the output - they should be the same
    ASSERT_TRUE(json == result);
}