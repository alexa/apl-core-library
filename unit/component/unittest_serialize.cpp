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

#include <iostream>
#include <sstream>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include "../testeventloop.h"

using namespace apl;

class SerializeTest : public DocumentWrapper {
protected:
    static void checkCommonProperties(const ComponentPtr& component, rapidjson::Value& json) {
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
        ASSERT_EQ(component->getCalculated(kPropertyFocusable), json["_focusable"].GetBool());
    }
};

static const char *SERIALIZE_COMPONENTS = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "numbered": true,
      "items": [
        {
          "type": "Image",
          "id": "image",
          "source": "http://images.amazon.com/image/foo.png",
          "overlayColor": "red",
          "overlayGradient": {
            "colorRange": [
              "blue",
              "red"
            ]
          },
          "filters": {
            "type": "Blur",
            "radius": 22
          }
        },
        {
          "type": "Text",
          "id": "text",
          "text": "<span color='red'>colorful</span> <b>Styled</b> <i>text</i>"
        },
        {
          "type": "ScrollView",
          "id": "scroll"
        },
        {
          "type": "Frame",
          "id": "frame",
          "backgroundColor": "red",
          "borderColor": "blue",
          "borderBottomLeftRadius": "1dp",
          "borderBottomRightRadius": "2dp",
          "borderTopLeftRadius": "3dp",
          "borderTopRightRadius": "4dp",
          "actions": {
            "name": "green",
            "label": "Change the border to green",
            "commands": {
              "type": "SetValue",
              "property": "borderColor",
              "value": "green"
            }
          }
        },
        {
          "type": "Sequence",
          "id": "sequence",
          "data": [
            1,
            2,
            3,
            4,
            5
          ],
          "items": [
            {
              "type": "Text",
              "id": "text",
              "width": 100,
              "height": 100,
              "text": "${data}"
            }
          ]
        },
        {
          "type": "TouchWrapper",
          "id": "touch",
          "height": 50,
          "onPress": {
            "type": "SendEvent",
            "arguments": [
              "${event.source.handler}",
              "${event.source.value}",
              "${event.target.opacity}"
            ],
            "components": [
              "text"
            ]
          }
        },
        {
          "type": "Pager",
          "id": "pager",
          "data": [
            1,
            2,
            3,
            4,
            5
          ],
          "items": [
            {
              "type": "Text",
              "id": "text",
              "width": 100,
              "height": 100,
              "text": "${data}"
            }
          ]
        },
        {
          "type": "VectorGraphic",
          "id": "vector",
          "source": "iconWifi3"
        },
        {
          "type": "Video",
          "id": "video",
          "source": [
            "URL1",
            {
              "url": "URL2"
            },
            {
              "description": "Sample video.",
              "duration": 1000,
              "url": "URL3",
              "repeatCount": 2,
              "offset": 100
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(SerializeTest, Components)
{
    loadDocument(SERIALIZE_COMPONENTS);
    ASSERT_TRUE(component);

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
    ASSERT_EQ(styledText.getSpans()[0].attributes.size(), textJson["text"]["spans"][0][3].Size());
    ASSERT_EQ(text->getCalculated(kPropertyTextAlignAssigned), textJson["_textAlign"].GetDouble());
    ASSERT_EQ(text->getCalculated(kPropertyTextAlignVertical), textJson["textAlignVertical"].GetDouble());

    auto scroll = context->findComponentById("scroll");
    ASSERT_TRUE(scroll);
    auto& scrollJson = json["children"][2];
    checkCommonProperties(scroll, scrollJson);
    ASSERT_EQ(scroll->getCalculated(kPropertyScrollPosition).asNumber(), scrollJson["_scrollPosition"].GetDouble());

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
    auto action = frame->getCalculated(kPropertyAccessibilityActions).at(0).getAccessibilityAction();
    ASSERT_EQ(action->getName(), frameJson["action"][0]["name"].GetString());
    ASSERT_EQ(action->getLabel(), frameJson["action"][0]["label"].GetString());
    ASSERT_EQ(action->enabled(), frameJson["action"][0]["enabled"].GetBool());
    ASSERT_FALSE(frameJson["action"][0].HasMember("commands"));  // Commands don't get serialized

    auto sequence = context->findComponentById("sequence");
    ASSERT_TRUE(sequence);
    auto& sequenceJson = json["children"][4];
    checkCommonProperties(sequence, sequenceJson);
    ASSERT_EQ(sequence->getCalculated(kPropertyScrollDirection), sequenceJson["scrollDirection"].GetDouble());
    ASSERT_EQ(sequence->getCalculated(kPropertyScrollPosition).asNumber(), sequenceJson["_scrollPosition"].GetDouble());

    auto touch = context->findComponentById("touch");
    ASSERT_TRUE(touch);
    auto& touchJson = json["children"][5];
    checkCommonProperties(touch, touchJson);

    auto pager = context->findComponentById("pager");
    ASSERT_TRUE(pager);
    auto& pagerJson = json["children"][6];
    checkCommonProperties(pager, pagerJson);
    ASSERT_EQ(pager->getCalculated(kPropertyNavigation), pagerJson["navigation"].GetDouble());
    ASSERT_EQ(pager->getCalculated(kPropertyCurrentPage), pagerJson["_currentPage"].GetDouble());

    auto vector = context->findComponentById("vector");
    ASSERT_TRUE(vector);
    auto& vectorJson = json["children"][7];
    checkCommonProperties(vector, vectorJson);
    ASSERT_EQ(vector->getCalculated(kPropertyAlign), vectorJson["align"].GetDouble());
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

    auto touchBounds = touch->getGlobalBounds();
    ASSERT_EQ(Rect(0, 310, 1024, 50), touchBounds);

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(1,311)));
    root->handlePointerEvent(PointerEvent(kPointerUp, Point(1,311)));

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
    ASSERT_STREQ("<span color='red'>colorful</span> <b>Styled</b> <i>text</i>", json["components"]["text"].GetString());

    ASSERT_STREQ("Press", json["source"]["handler"].GetString());
    ASSERT_STREQ("touch", json["source"]["id"].GetString());
    ASSERT_STREQ("TouchWrapper", json["source"]["source"].GetString());
    ASSERT_EQ(touch->getUniqueId(), json["source"]["uid"].GetString());
    ASSERT_EQ(false, json["source"]["value"].GetBool());
}

const static char * SERIALIZE_ALL = R"({
  "type": "APL",
  "version": "1.1",
  "layouts": {
    "MyLayout": {
      "parameters": "MyText",
      "items": {
        "type": "Text",
        "text": "${MyText}",
        "width": "100%",
        "textAlign": "center"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "MyLayout",
      "MyText": "Hello",
      "width": "100%",
      "height": "50%"
    }
  }
})";

const static char *SERIALIZE_ALL_RESULT = R"({
  "type": "Text",
  "__id": "",
  "__inheritParentState": false,
  "__style": "",
  "__path": "_main/layouts/MyLayout/items",
  "accessibilityLabel": "",
  "action": [],
  "_bounds": [
    0,
    0,
    1280,
    400
  ],
  "checked": false,
  "color": "#fafafaff",
  "_colorKaraokeTarget": "#fafafaff",
  "_colorNonKaraoke": "#fafafaff",
  "description": "",
  "disabled": false,
  "display": "normal",
  "entities": [],
  "_focusable": false,
  "fontFamily": "sans-serif",
  "fontSize": 40,
  "fontStyle": "normal",
  "fontWeight": "normal",
  "handleTick": [],
  "height": "50%",
  "_innerBounds": [
    0,
    0,
    1280,
    400
  ],
  "lang": "",
  "layoutDirection": "inherit",
  "_layoutDirection": "LTR",
  "letterSpacing": 0,
  "lineHeight": 1.25,
  "maxHeight": null,
  "maxLines": 0,
  "maxWidth": null,
  "minHeight": 0,
  "minWidth": 0,
  "onMount": [],
  "opacity": 1,
  "padding": [],
  "paddingBottom": null,
  "paddingEnd": null,
  "paddingLeft": null,
  "paddingRight": null,
  "paddingTop": null,
  "paddingStart": null,
  "preserve": [],
  "role": "none",
  "shadowColor": "#00000000",
  "shadowHorizontalOffset": 0,
  "shadowRadius": 0,
  "shadowVerticalOffset": 0,
  "speech": "",
  "text": {
    "text": "Hello",
    "spans": []
  },
  "textAlign": "center",
  "_textAlign": "center",
  "textAlignVertical": "auto",
  "_transform": [
    1,
    0,
    0,
    1,
    0,
    0
  ],
  "transform": null,
  "_user": {},
  "width": "100%",
  "onCursorEnter": [],
  "onCursorExit": [],
  "_laidOut": true,
  "_visualHash": "[HASH]"
})";

TEST_F(SerializeTest, SerializeAll)
{
    metrics.size(1280, 800);
    loadDocument(SERIALIZE_ALL);
    ASSERT_TRUE(component);

    auto visualHash = component->getCalculated(kPropertyVisualHash).getString();

    rapidjson::Document doc;
    auto json = component->serializeAll(doc.GetAllocator());

    // Need to remove the "id" element - it changes depending on the number of unit tests executed.
    json.RemoveMember("id");

    // Load the JSON result above
    rapidjson::Document result;
    rapidjson::ParseResult ok = result.Parse(SERIALIZE_ALL_RESULT);
    ASSERT_TRUE(ok);

    result["_visualHash"].SetString(visualHash.c_str(), doc.GetAllocator());

    // Compare the output - they should be the same
    ASSERT_TRUE(json == result);
}

static const char *CHILDREN_UPDATE = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "item": {
        "type": "Text",
        "text": "${data} ${index} ${dataIndex} ${length}"
      }
    }
  }
})";

TEST_F(SerializeTest, ChildrenUpdateNotification)
{
    auto myArray = LiveArray::create(ObjectArray{"A", "B"});
    config->liveData("TestArray", myArray);

    loadDocument(CHILDREN_UPDATE);

    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());

    auto removedId = component->getChildAt(1)->getUniqueId();

    myArray->insert(0, "Z");  // Z, A, B
    myArray->push_back("C");  // Z, A, B, C
    myArray->remove(2);       // Z, A, C
    root->clearPending();

    ASSERT_EQ(3, component->getChildCount());

    rapidjson::Document doc;

    auto json = component->serializeDirty(doc.GetAllocator());

    ASSERT_EQ(2, json.MemberCount());

    auto& notify = json["_notify_childrenChanged"];

    ASSERT_EQ(3, notify.Size());
    ASSERT_EQ(0.0, notify[0]["index"].GetDouble());
    ASSERT_EQ(component->getChildAt(0)->getUniqueId(), notify[0]["uid"].GetString());
    ASSERT_STREQ("insert", notify[0]["action"].GetString());
    ASSERT_EQ(2.0, notify[1]["index"].GetDouble());
    ASSERT_EQ(component->getChildAt(2)->getUniqueId(), notify[1]["uid"].GetString());
    ASSERT_STREQ("insert", notify[1]["action"].GetString());
    ASSERT_EQ(3.0, notify[2]["index"].GetDouble());
    ASSERT_EQ(removedId, notify[2]["uid"].GetString());
    ASSERT_STREQ("remove", notify[2]["action"].GetString());
}

static const char *SEQUENCE_CHILDREN_UPDATE = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "data": "${TestArray}",
      "item": {
        "type": "Text",
        "height": 100,
        "width": "100%",
        "text": "${data} ${index} ${dataIndex} ${length}"
      }
    }
  }
})";

TEST_F(SerializeTest, SequencePositionChildrenUpdate)
{
    auto myArray = LiveArray::create(ObjectArray{"A", "B"});
    config->liveData("TestArray", myArray);

    loadDocument(SEQUENCE_CHILDREN_UPDATE);

    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());

    myArray->insert(0, "Z");  // Z, A, B
    myArray->remove(2);       // Z, A
    root->clearPending();

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_EQ(100, component->getCalculated(kPropertyScrollPosition).asNumber());

    rapidjson::Document doc;

    auto json = component->serializeDirty(doc.GetAllocator());

    ASSERT_EQ(3, json.MemberCount());
    ASSERT_EQ(2, json["_notify_childrenChanged"].Size());
    ASSERT_EQ(component->getCalculated(kPropertyScrollPosition).asNumber(), json["_scrollPosition"].GetDouble());
}

static const char *PAGER_CHILDREN_UPDATE = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "data": "${TestArray}",
      "item": {
        "type": "Text",
        "height": 100,
        "width": "100%",
        "text": "${data} ${index} ${dataIndex} ${length}"
      }
    }
  }
})";

TEST_F(SerializeTest, PagerPositionChildrenUpdate)
{
    auto myArray = LiveArray::create(ObjectArray{"A", "B"});
    config->liveData("TestArray", myArray);

    loadDocument(PAGER_CHILDREN_UPDATE);

    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());

    ASSERT_EQ(0, component->getCalculated(kPropertyCurrentPage).getInteger());

    myArray->insert(0, "Z");  // Z, A, B
    myArray->remove(2);       // Z, A
    root->clearPending();

    ASSERT_EQ(2, component->getChildCount());

    rapidjson::Document doc;

    auto json = component->serializeDirty(doc.GetAllocator());

    ASSERT_EQ(3, json.MemberCount());
    ASSERT_EQ(2, json["_notify_childrenChanged"].Size());
    ASSERT_EQ(1, component->getCalculated(kPropertyCurrentPage).getInteger());
    ASSERT_EQ(component->getCalculated(kPropertyCurrentPage).getInteger(), json["_currentPage"].GetDouble());
}

static const char *SERIALIE_VG = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "height": 100,
      "width": 100,
      "source": "box"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "resources": [
        {
          "gradients": {
            "strokeGradient": {
              "type": "linear",
              "colorRange": [ "blue", "white" ],
              "inputRange": [0, 1],
              "x1": 0.1,
              "y1": 0.2,
              "x2": 0.3,
              "y2": 0.4
            }
          },
          "patterns": {
            "fillPattern": {
              "height": 18,
              "width": 18,
              "item": {
                "type": "path",
                "pathData": "M0,9 a9,9 0 1 1 18,0 a9,9 0 1 1 -18,0",
                "fill": "red"
              }
            }
          }
        }
      ],
      "items": {
        "type": "group",
        "clipPath": "M 0,0",
        "opacity": 0.7,
        "transform": "translate(1 1) ",
        "items": [
          {
            "type": "path",
            "fill": "@fillPattern",
            "fillOpacity": 0.1,
            "fillTransform": "skewX(7) ",
            "pathData": "M 1,1",
            "pathLength": 5,
            "stroke": "@strokeGradient",
            "strokeDashArray": [1, 2, 3],
            "strokeDashOffset": 1,
            "strokeLineCap": "butt",
            "strokeLineJoin": "bevel",
            "strokeMiterLimit": 2,
            "strokeOpacity": 0.9,
            "strokeTransform": "skewY(8) ",
            "strokeWidth": 2
          },
          {
            "type": "text",
            "fill": "red",
            "fillOpacity": 0.1,
            "fillTransform": "skewX(7) ",
            "fontFamily": "Violet",
            "fontSize": 50,
            "fontStyle": "italic",
            "fontWeight": "bold",
            "letterSpacing": 3,
            "stroke": "green",
            "strokeOpacity": 0.9,
            "strokeTransform": "skewY(8) ",
            "strokeWidth": 2,
            "text": "Text",
            "textAnchor": "middle",
            "x": 5,
            "y": 6
          }
        ]
      }
    }
  }
})";



TEST_F(SerializeTest, AVG)
{
    loadDocument(SERIALIE_VG);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeVectorGraphic, component->getType());

    rapidjson::Document doc;
    auto json = component->serialize(doc.GetAllocator());

    checkCommonProperties(component, json);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    auto& graphicJson = json["graphic"];

    ASSERT_EQ(graphicJson["isValid"].GetBool(), true);
    ASSERT_EQ(graphicJson["intrinsicWidth"].GetDouble(), graphic->getIntrinsicWidth());
    ASSERT_EQ(graphicJson["intrinsicHeight"].GetDouble(), graphic->getIntrinsicHeight());
    ASSERT_EQ(graphicJson["viewportWidth"].GetDouble(), graphic->getViewportWidth());
    ASSERT_EQ(graphicJson["viewportHeight"].GetDouble(), graphic->getViewportHeight());


    auto graphicRoot = graphic->getRoot();
    auto& graphicRootJson = graphicJson["root"];

    ASSERT_EQ(graphicRootJson["id"].GetInt(), graphicRoot->getId());
    ASSERT_EQ(graphicRootJson["props"]["height_actual"].GetDouble(), graphicRoot->getValue(kGraphicPropertyHeightActual).getAbsoluteDimension());
    ASSERT_EQ(graphicRootJson["props"]["width_actual"].GetDouble(), graphicRoot->getValue(kGraphicPropertyWidthActual).getAbsoluteDimension());
    ASSERT_EQ(graphicRootJson["props"]["viewportHeight_actual"].GetDouble(), graphicRoot->getValue(kGraphicPropertyViewportHeightActual).getDouble());
    ASSERT_EQ(graphicRootJson["props"]["viewportWidth_actual"].GetDouble(), graphicRoot->getValue(kGraphicPropertyViewportWidthActual).getDouble());


    auto group = graphicRoot->getChildAt(0);
    auto& groupJson = graphicRootJson["children"][0];

    ASSERT_EQ(groupJson["id"].GetInt(), group->getId());
    ASSERT_EQ(groupJson["type"].GetInt(), group->getType());
    ASSERT_EQ(groupJson["props"]["clipPath"].GetString(), group->getValue(kGraphicPropertyClipPath).getString());
    ASSERT_EQ(groupJson["props"]["opacity"].GetDouble(), group->getValue(kGraphicPropertyOpacity).getDouble());
    ASSERT_TRUE(groupJson["props"]["_transform"].IsArray());


    auto path = group->getChildAt(0);
    auto& pathJson = groupJson["children"][0];

    ASSERT_EQ(pathJson["id"].GetInt(), path->getId());
    ASSERT_EQ(pathJson["type"].GetInt(), path->getType());
    ASSERT_EQ(pathJson["props"]["fillOpacity"].GetDouble(), path->getValue(kGraphicPropertyFillOpacity).getDouble());
    ASSERT_TRUE(pathJson["props"]["_fillTransform"].IsArray());
    ASSERT_EQ(pathJson["props"]["pathData"].GetString(), path->getValue(kGraphicPropertyPathData).getString());
    ASSERT_EQ(pathJson["props"]["pathLength"].GetDouble(), path->getValue(kGraphicPropertyPathLength).getDouble());
    ASSERT_TRUE(pathJson["props"]["strokeDashArray"].IsArray());
    ASSERT_EQ(pathJson["props"]["strokeDashOffset"].GetDouble(), path->getValue(kGraphicPropertyStrokeDashOffset).getDouble());
    ASSERT_EQ(pathJson["props"]["strokeLineCap"].GetDouble(), path->getValue(kGraphicPropertyStrokeLineCap).getInteger());
    ASSERT_EQ(pathJson["props"]["strokeLineJoin"].GetDouble(), path->getValue(kGraphicPropertyStrokeLineJoin).getInteger());
    ASSERT_EQ(pathJson["props"]["strokeMiterLimit"].GetDouble(), path->getValue(kGraphicPropertyStrokeMiterLimit).getInteger());
    ASSERT_EQ(pathJson["props"]["strokeOpacity"].GetDouble(), path->getValue(kGraphicPropertyStrokeOpacity).getDouble());
    ASSERT_TRUE(pathJson["props"]["_strokeTransform"].IsArray());
    ASSERT_EQ(pathJson["props"]["strokeWidth"].GetDouble(), path->getValue(kGraphicPropertyStrokeWidth).getDouble());

    auto gradient = path->getValue(kGraphicPropertyStroke).getGradient();
    auto& gradientJson = pathJson["props"]["stroke"];

    ASSERT_EQ(gradientJson["type"].GetDouble(), gradient.getType());
    ASSERT_TRUE(gradientJson["colorRange"].IsArray());
    ASSERT_TRUE(gradientJson["inputRange"].IsArray());
    ASSERT_EQ(gradientJson["spreadMethod"].GetDouble(), gradient.getProperty(kGradientPropertySpreadMethod).getInteger());
    ASSERT_EQ(gradientJson["x1"].GetDouble(), gradient.getProperty(kGradientPropertyX1).getDouble());
    ASSERT_EQ(gradientJson["y1"].GetDouble(), gradient.getProperty(kGradientPropertyY1).getDouble());
    ASSERT_EQ(gradientJson["x2"].GetDouble(), gradient.getProperty(kGradientPropertyX2).getDouble());
    ASSERT_EQ(gradientJson["y2"].GetDouble(), gradient.getProperty(kGradientPropertyY2).getDouble());


    auto pattern = path->getValue(kGraphicPropertyFill).getGraphicPattern();
    auto& patternJson = pathJson["props"]["fill"];

    ASSERT_EQ(patternJson["id"].GetString(), pattern->getId());
    ASSERT_EQ(patternJson["description"].GetString(), pattern->getDescription());
    ASSERT_EQ(patternJson["width"].GetDouble(), pattern->getWidth());
    ASSERT_EQ(patternJson["height"].GetDouble(), pattern->getHeight());

    auto patternPath = pattern->getItems().at(0);
    auto& patternPathJson = patternJson["items"][0];
    // Just check type and ID. It's just regular Path.
    ASSERT_EQ(patternPathJson["id"].GetInt(), patternPath->getId());
    ASSERT_EQ(patternPathJson["type"].GetInt(), patternPath->getType());


    auto text = group->getChildAt(1);
    auto& textJson = groupJson["children"][1];

    ASSERT_EQ(textJson["id"].GetInt(), text->getId());
    ASSERT_EQ(textJson["type"].GetInt(), text->getType());
    ASSERT_EQ(textJson["props"]["x"].GetDouble(), text->getValue(kGraphicPropertyCoordinateX).getDouble());
    ASSERT_EQ(textJson["props"]["y"].GetDouble(), text->getValue(kGraphicPropertyCoordinateY).getDouble());
    ASSERT_EQ(textJson["props"]["fill"].GetString(), text->getValue(kGraphicPropertyFill).asString());
    ASSERT_EQ(textJson["props"]["fillOpacity"].GetDouble(), text->getValue(kGraphicPropertyFillOpacity).getDouble());
    ASSERT_EQ(textJson["props"]["fontFamily"].GetString(), text->getValue(kGraphicPropertyFontFamily).getString());
    ASSERT_EQ(textJson["props"]["fontSize"].GetDouble(), text->getValue(kGraphicPropertyFontSize).getDouble());
    ASSERT_EQ(textJson["props"]["fontStyle"].GetDouble(), text->getValue(kGraphicPropertyFontStyle).getInteger());
    ASSERT_EQ(textJson["props"]["fontWeight"].GetDouble(), text->getValue(kGraphicPropertyFontWeight).getDouble());
    ASSERT_EQ(textJson["props"]["letterSpacing"].GetDouble(), text->getValue(kGraphicPropertyLetterSpacing).getDouble());
    ASSERT_EQ(textJson["props"]["stroke"].GetString(), text->getValue(kGraphicPropertyStroke).asString());
    ASSERT_EQ(textJson["props"]["strokeOpacity"].GetDouble(), text->getValue(kGraphicPropertyStrokeOpacity).getDouble());
    ASSERT_EQ(textJson["props"]["strokeWidth"].GetDouble(), text->getValue(kGraphicPropertyStrokeWidth).getDouble());
    ASSERT_EQ(textJson["props"]["text"].GetString(), text->getValue(kGraphicPropertyText).getString());
    ASSERT_EQ(textJson["props"]["textAnchor"].GetDouble(), text->getValue(kGraphicPropertyTextAnchor).getInteger());
}

static const char *MUSIC_DOC = R"({
    "type": "APL",
    "version": "1.5",
    "mainTemplate": {
        "items": [
            {
                "type": "Container",
                "height": "100%",
                "width": "100%",
                "id": "document",
                "items": [
                    {
                        "type": "Container",
                        "position": "relative",
                        "id": "view",
                        "height": "16vh",
                        "display": "none",
                        "grow": 1,
                        "items": [
                            {
                                "type": "Sequence",
                                "height": "16vh",
                                "alignSelf": "center",
                                "position": "absolute",
                                "id": "sequence",
                                "numbered": true,
                                "data": [
                                    "first",
                                    "second"
                                ],
                                "grow": 1,
                                "item": {
                                    "type": "VectorGraphic",
                                    "source": "diamond",
                                    "scale": "best-fit",
                                    "width": "100%",
                                    "align": "center",
                                    "Tick": "${elapsedTime}"
                                }
                            }
                        ]
                    }
                ]
            }
        ]
    },
    "graphics": {
        "diamond": {
            "type": "AVG",
            "version": "1.1",
            "parameters": [
                {
                    "name": "Tick",
                    "type": "number",
                    "default": 0
                },
                {
                    "name": "Colors",
                    "type": "array",
                    "default": ["yellow", "orange", "red"]
                }
            ],
            "width": 48,
            "height": 48,
            "items": {
                "type": "path",
                "fill": "${Colors[Tick % Colors.length]}",
                "stroke": "${Colors[(Tick+1) % Colors.length]}",
                "strokeWidth": 3,
                "pathData": "M 24 0 L 48 24 L 24 48 L 0 24 z"
            }
        }
    }
})";

TEST_F(SerializeTest, AVGInSequence) {
    loadDocument(MUSIC_DOC);
    ASSERT_TRUE(component);

    advanceTime(5);

    ASSERT_TRUE(root->isDirty());
    ASSERT_EQ(2, root->getDirty().size());
    for (auto& c : root->getDirty()) {
        rapidjson::Document doc;
        auto json = c->serializeDirty(doc.GetAllocator());

        ASSERT_TRUE(json.HasMember("graphic"));
        ASSERT_TRUE(json["graphic"]["isValid"].GetBool());
        ASSERT_EQ(json["graphic"]["intrinsicWidth"].GetDouble(), 48.0);
        ASSERT_EQ(json["graphic"]["intrinsicHeight"].GetDouble(), 48.0);
        ASSERT_EQ(json["graphic"]["viewportWidth"].GetDouble(), 48.0);
        ASSERT_EQ(json["graphic"]["viewportHeight"].GetDouble(), 48.0);
        ASSERT_EQ(json["graphic"]["root"]["props"]["width_actual"].GetDouble(), 48.0);
        ASSERT_EQ(json["graphic"]["root"]["props"]["height_actual"].GetDouble(), 48.0);
        ASSERT_EQ(json["graphic"]["root"]["props"]["viewportWidth_actual"].GetDouble(), 48.0);
        ASSERT_EQ(json["graphic"]["root"]["props"]["viewportHeight_actual"].GetDouble(), 48.0);

        ASSERT_STREQ(json["graphic"]["root"]["children"][0]["props"]["fill"].GetString(), "#ff0000ff");
        ASSERT_STREQ(json["graphic"]["root"]["children"][0]["props"]["stroke"].GetString(), "#ffff00ff");
        ASSERT_EQ(json["graphic"]["root"]["children"][0]["props"]["strokeWidth"].GetDouble(), 3.0);
    }
}

static const char *SINGULAR_TRANSFORM = R"({
    "type": "APL",
    "version": "1.5",
    "mainTemplate": {
        "items": [
            {
                "type": "Container",
                "height": "100%",
                "width": "100%",
                "id": "document",
                "items": [
                    {
                        "type": "Text",
                        "id": "text",
                        "transform": [
                            {"scale": "${1/0}"}
                        ],
                        "text": "Lorem Ipsum"
                    }
                ]
            }
        ]
    }
})";

TEST_F(SerializeTest, SingularTransform) {
    loadDocument(SINGULAR_TRANSFORM);
    ASSERT_TRUE(component);

    rapidjson::Document doc;
    auto json = component->serialize(doc.GetAllocator());

    auto &transformJson = json["children"][0]["_transform"];
    ASSERT_EQ(0.0f, transformJson[0].GetFloat());
    ASSERT_EQ(0.0f, transformJson[1].GetFloat());
    ASSERT_EQ(0.0f, transformJson[2].GetFloat());
    ASSERT_EQ(0.0f, transformJson[3].GetFloat());
    ASSERT_EQ(0.0f, transformJson[4].GetFloat());
    ASSERT_EQ(0.0f, transformJson[5].GetFloat());
}
