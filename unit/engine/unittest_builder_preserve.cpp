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

using namespace apl;

class BuilderPreserveTest : public DocumentWrapper {};

static const char * BASIC = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "Text",
          "id": "TEST1",
          "text": "Hello",
          "preserve": [
            "text"
          ]
        }
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";

TEST_F(BuilderPreserveTest, Basic)
{
    loadDocument(BASIC);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual("Hello", component->getCalculated(kPropertyText).asString()));

    // Change the text
    executeCommand("SetValue", {{"componentId", "TEST1"},
                                {"property",    "text"},
                                {"value",       "Woof"}}, false);
    ASSERT_TRUE(IsEqual("Woof", component->getCalculated(kPropertyText).asString()));

    // Re-inflate the document using the same RootContext.
    auto oldComponent = component;
    configChangeReinflate(ConfigurationChange(metrics.getPixelWidth(), metrics.getPixelHeight()));

    ASSERT_TRUE(component);
    ASSERT_NE(component->getUniqueId(), oldComponent->getUniqueId()); // The component should have changed

    ASSERT_TRUE(IsEqual("Woof", component->getCalculated(kPropertyText).asString()));
}

static const char *CHECKED_DISABLED = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "id": "TEST",
          "preserve": [
            "checked",
            "disabled"
          ]
        }
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";

/**
 * Verify that state preservation works for "checked" and "disabled" states
 */
TEST_F(BuilderPreserveTest, CheckedDisabled)
{
    loadDocument(CHECKED_DISABLED);
    ASSERT_TRUE(component);
    ASSERT_TRUE(CheckState(component));  // No state set

    // Change the text
    executeCommand("SetValue", {{"componentId", "TEST"},
                                {"property",    "checked"},
                                {"value",       true}}, false);
    executeCommand("SetValue", {{"componentId", "TEST"},
                                {"property",    "disabled"},
                                {"value",       true}}, false);

    ASSERT_TRUE(CheckState(component, kStateChecked, kStateDisabled));

    // Re-inflate the document using the same RootContext.
    auto oldComponent = component;
    configChangeReinflate(ConfigurationChange(metrics.getPixelWidth(), metrics.getPixelHeight()));

    ASSERT_TRUE(component);
    ASSERT_NE(component->getUniqueId(), oldComponent->getUniqueId()); // The component id should have changed

    ASSERT_TRUE(CheckState(component, kStateChecked, kStateDisabled));
}


static const char *CHECKED_DISABLED_INHERIT_PARENT_STATE =R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "TouchWrapper",
              "id": "WRAPPER1",
              "description": "This touch wrapper does not save state",
              "item": {
                "type": "Frame",
                "id": "FRAME1",
                "inheritParentState": true,
                "preserve": [
                  "checked",
                  "disabled"
                ]
              }
            },
            {
              "type": "TouchWrapper",
              "id": "WRAPPER2",
              "description": "This touch wrapper saves state",
              "preserve": [
                "checked",
                "disabled"
              ],
              "item": {
                "type": "Frame",
                "id": "FRAME2",
                "inheritParentState": true
              }
            }
          ]
        }
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";

/**
 * Ensure state preservation for "checked" and "disabled" don't work if parent state is inherited
 */
TEST_F(BuilderPreserveTest, CheckedDisabledInheritParentState)
{
    loadDocument(CHECKED_DISABLED_INHERIT_PARENT_STATE);
    ASSERT_TRUE(component);

    auto wrapper1 = component->getChildAt(0);
    auto frame1 = wrapper1->getChildAt(0);
    auto wrapper2 = component->getChildAt(1);
    auto frame2 = wrapper2->getChildAt(0);

    // Change the state of both touch wrappers
    executeCommand("SetValue", {{"componentId", "WRAPPER1"},
                                {"property",    "checked"},
                                {"value",       true}}, false);
    executeCommand("SetValue", {{"componentId", "WRAPPER1"},
                                {"property",    "disabled"},
                                {"value",       true}}, false);
    executeCommand("SetValue", {{"componentId", "WRAPPER2"},
                                {"property",    "checked"},
                                {"value",       true}}, false);
    executeCommand("SetValue", {{"componentId", "WRAPPER2"},
                                {"property",    "disabled"},
                                {"value",       true}}, false);

    // Everyone should be checked and disabled
    ASSERT_TRUE(CheckState(wrapper1, kStateChecked, kStateDisabled));
    ASSERT_TRUE(CheckState(frame1, kStateChecked, kStateDisabled));
    ASSERT_TRUE(CheckState(wrapper2, kStateChecked, kStateDisabled));
    ASSERT_TRUE(CheckState(frame2, kStateChecked, kStateDisabled));

    // Re-inflate the document using the same RootContext.
    configChangeReinflate(ConfigurationChange(metrics.getPixelWidth(), metrics.getPixelHeight()));
    ASSERT_TRUE(component);

    wrapper1 = component->getChildAt(0);
    frame1 = wrapper1->getChildAt(0);
    wrapper2 = component->getChildAt(1);
    frame2 = wrapper2->getChildAt(0);

    ASSERT_TRUE(CheckState(wrapper1));   // Wrapper 1 did not save state
    ASSERT_TRUE(CheckState(frame1));     // Frame 1 tried to save state, but fails because it inherits state
    ASSERT_TRUE(CheckState(wrapper2, kStateChecked, kStateDisabled));  // Wrapper 2 saves state
    ASSERT_TRUE(CheckState(frame2, kStateChecked, kStateDisabled));  // Frame 2 gets that inherited state
}


static const char *DYNAMIC_PROPERTIES = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "Container",
              "id": "CONTAINER",
              "preserve": ["accessibilityLabel", "display", "opacity"]
            },
            {
              "type": "EditText",
              "id": "EDIT_TEXT",
              "borderWidth": 3,
              "preserve": ["borderColor", "color", "secureInput", "text", "borderStrokeWidth" ]
            },
            {
              "type": "Frame",
              "id": "FRAME",
              "borderWidth": 3,
              "preserve": ["backgroundColor", "borderColor", "borderStrokeWidth"]
            },
            {
              "type": "Image",
              "id": "IMAGE",
              "preserve": ["overlayColor", "source"]
            },
            {
              "type": "Text",
              "id": "TEXT",
              "preserve": ["color", "text"]
            },
            {
              "type": "Video",
              "id": "VIDEO",
              "preserve": ["source"]
            }
          ]
        }
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";


/**
 * Verify that all dynamic properties can be preserved
 */
TEST_F(BuilderPreserveTest, SetDynamicProperties)
{
    loadDocument(DYNAMIC_PROPERTIES);
    ASSERT_TRUE(component);

    struct TestCase {
        std::string id;
        std::string property;
        Object value;
        std::function<bool(const Object&, const Object&)> compare;
    };

    std::vector<TestCase> TEST_CASES = {
        {"CONTAINER", "accessibilityLabel", "Test Label",                           nullptr},
        {"CONTAINER", "display",            "none",                                 [](const Object& target,
                                                                                       const Object& x) {
            return x.asInt() == sDisplayMap.at(target.asString());
        }},
        {"CONTAINER", "opacity",            0.5,                                    nullptr},
        // { "CONTAINER", "transform", ObjectArray{{ObjectMap{{"rotate", 45}}}}, nullptr },
        {"EDIT_TEXT", "borderColor",        Color(Color::SILVER),                   nullptr},
        {"EDIT_TEXT", "secureInput",        true,                                   nullptr},
        {"EDIT_TEXT", "text",               "This is a test",                       nullptr},
        {"EDIT_TEXT", "borderStrokeWidth",  2,                                      [](const Object& target,
                                                                                       const Object& x) {
            return x.isAbsoluteDimension() && x.getAbsoluteDimension() == target.asNumber();
        }},
        {"FRAME",     "backgroundColor",    Color(Color::PURPLE),                   nullptr},
        {"FRAME",     "borderColor",        Color(Color::FUCHSIA),                  nullptr},
        {"FRAME",     "borderStrokeWidth",  1.5,                                    [](const Object& target,
                                                                                       const Object& x) {
            return x.isAbsoluteDimension() && x.getAbsoluteDimension() == target.asNumber();
        }},
        {"IMAGE",     "overlayColor",       Color(Color::RED),                      nullptr},
        {"IMAGE",     "source",             "http://www.picturestuff.fake/dog.png", nullptr},
        {"TEXT",      "color",              Color(Color::LIME),                     nullptr},
        {"TEXT",      "text",               "This is another test",                 [](const Object& target,
                                                                                       const Object& x) {
            return x.isStyledText() && x.getStyledText().asString() == target.asString();
        }},
        {"VIDEO",     "source",             "http://www.videostuff.fake/dog.mp3",   [](const Object& target,
                                                                                       const Object& x) {
            return x.isArray() && x.size() == 1 && x.at(0).isMediaSource() &&
                   x.at(0).getMediaSource().getUrl() == target.asString();
        }},
    };

    // Dynamically set all of the properties and verify that they have been set
    for (const auto& m : TEST_CASES) {
        auto c = root->findComponentById(m.id);
        ASSERT_TRUE(c) << m.id;
        executeCommand("SetValue", {{"componentId", m.id},
                                    {"property", m.property},
                                    {"value", m.value}}, false);
        auto key = static_cast<PropertyKey>(sComponentPropertyBimap.at(m.property));
        const auto& value = c->getCalculated(key);
        if (m.compare)
            ASSERT_TRUE(m.compare(m.value, value)) << m.id << " " << m.property;
        else
            ASSERT_TRUE(IsEqual(m.value, value)) << m.id << " " << m.property;
    }

    // Re-inflate the document using the same RootContext.
    configChangeReinflate(ConfigurationChange(metrics.getPixelWidth(), metrics.getPixelHeight()));

    // Verify that all of the properties are still set
    for (const auto& m : TEST_CASES) {
        auto c = root->findComponentById(m.id);
        ASSERT_TRUE(c) << m.id;
        auto key = static_cast<PropertyKey>(sComponentPropertyBimap.at(m.property));
        const auto& value = c->getCalculated(key);
        if (m.compare)
            ASSERT_TRUE(m.compare(m.value, value)) << m.id << " " << m.property;
        else
            ASSERT_TRUE(IsEqual(m.value, value)) << m.id << " " << m.property;
    }
}


static const char *BUTTON = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "layouts": {
        "Button": {
          "parameters": [ "LABEL" ],
          "items": {
            "type": "TouchWrapper",
            "items": {
              "type": "Text",
              "text": "${LABEL}",
              "inheritParentState": true
            },
            "onPress": {
              "type": "SetValue",
              "property": "checked",
              "value": "${!event.source.value}"
            },
            "preserve": ["checked", "LABEL"]
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Button",
          "id": "MyButton",
          "LABEL": "Big Button"
        }
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";

/**
 * Pass the id through to a layout and use it to preserve the checked state.
 * Then change the label and see what happens
 */
TEST_F(BuilderPreserveTest, Button)
{
    loadDocument(BUTTON);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(0);

    ASSERT_TRUE(IsEqual("Big Button", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckState(text));

    // Toggle the checked state
    component->update(kUpdatePressed, 1);
    ASSERT_TRUE(CheckState(text, kStateChecked));

    // Change the LABEL property
    executeCommand("SetValue", {{"componentId", "MyButton"},
                                {"property", "LABEL"},
                                {"value", "Small Button"}}, false);
    ASSERT_TRUE(IsEqual("Small Button", text->getCalculated(kPropertyText).asString()));

    // Rebuild the world
    auto oldComponent = component;
    configChangeReinflate(ConfigurationChange(100, 100));
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getId(), oldComponent->getId());
    ASSERT_NE(component->getUniqueId(), oldComponent->getUniqueId());

    text = component->getChildAt(0);
    ASSERT_TRUE(text);

    // Verify that we are still checked and that the label was preserved
    ASSERT_TRUE(CheckState(component, kStateChecked));
    ASSERT_TRUE(CheckState(text, kStateChecked));
    ASSERT_TRUE(IsEqual("Small Button", text->getCalculated(kPropertyText).asString()));

    // Toggle again and verify that the checked state is removed
    component->update(kUpdatePressed, 1);
    ASSERT_TRUE(CheckState(component));
    ASSERT_TRUE(CheckState(text));
}

static const char *TWO_BUTTON_VARIATIONS = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "layouts": {
        "TextButton": {
          "parameters": "LABEL",
          "items": {
            "type": "TouchWrapper",
            "items": {
              "type": "Text",
              "text": "${LABEL}"
            },
            "preserve": [
              "checked"
            ]
          }
        },
        "ImageButton": {
          "parameters": "URL",
          "items": {
            "type": "TouchWrapper",
            "items": {
              "type": "Image",
              "source": "${URL}"
            },
            "preserve": [
              "checked"
            ]
          }
        }
      },
      "resources": [
        {
          "boolean": {
            "PortraitMode": "${viewport.width < viewport.height}"
          }
        }
      ],
      "mainTemplate": {
        "items": [
          {
            "when": "@PortraitMode",
            "type": "TextButton",
            "id": "MyButton",
            "LABEL": "Big Button"
          },
          {
            "type": "ImageButton",
            "id": "MyButton",
            "URL": "http://images.company.fake/foo.png"
          }
        ]
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";

TEST_F(BuilderPreserveTest, TwoButtonVariations)
{
    metrics.size(1000, 500);
    loadDocument(TWO_BUTTON_VARIATIONS);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeImage, component->getChildAt(0)->getType());

    // Toggle the check mark
    executeCommand("SetValue", {{"componentId", "MyButton"},
                                {"property", "checked"},
                                {"value", true}}, false);
    ASSERT_TRUE(CheckState(component, kStateChecked));

    // Reinflate
    configChangeReinflate(ConfigurationChange(500,1000));
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeText, component->getChildAt(0)->getType());

    // Should still be checked
    ASSERT_TRUE(CheckState(component, kStateChecked));
}


static const char *PAGER_PRESERVE_INDEX = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Pager",
          "id": "DogPager",
          "bind": {
            "name": "DOG_LIST",
            "value": [
              "Briard",
              "Chinook",
              "Golden Retriever",
              "Belgian Sheepdog"
            ]
          },
          "preserve": [
            "pageIndex"
          ],
          "item": {
            "type": "Text",
            "text": "${data}=${index}"
          },
          "data": "${Array.slice(DOG_LIST, environment.reason == 'initial' ? 0 : 1)}"
        }
      },
      "onConfigChange": [
        {
          "type": "Reinflate"
        }
      ]
    }
)apl";

/**
 * Preserve the current index of the pager, even though a reinflation changes which pages
 * are included.
 */
TEST_F(BuilderPreserveTest, PagerPreserveIndex)
{
    metrics.size(1000,500);
    loadDocument(PAGER_PRESERVE_INDEX);
    ASSERT_TRUE(component);

    ASSERT_EQ(4, component->getChildCount());
    auto currentPage = component->getCalculated(kPropertyCurrentPage).asInt();
    ASSERT_EQ(0, currentPage);
    ASSERT_TRUE(IsEqual("Briard=0", component->getChildAt(currentPage)->getCalculated(kPropertyText).asString()));

    component->update(kUpdatePagerPosition, 2);
    currentPage = component->getCalculated(kPropertyCurrentPage).asInt();
    ASSERT_EQ(2, currentPage);
    ASSERT_TRUE(IsEqual("Golden Retriever=2", component->getChildAt(currentPage)->getCalculated(kPropertyText).asString()));

    configChangeReinflate(ConfigurationChange().theme("blue"));
    ASSERT_EQ(3, component->getChildCount());
    currentPage = component->getCalculated(kPropertyCurrentPage).asInt();
    ASSERT_EQ(2, currentPage);
    ASSERT_TRUE(IsEqual("Belgian Sheepdog=2", component->getChildAt(currentPage)->getCalculated(kPropertyText).asString()));
}


static const char *PAGER_PRESERVE_ID = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Pager",
          "id": "DogPager",
          "bind": {
            "name": "DOG_LIST",
            "value": [
              {
                "name": "Briard",
                "id": "DOG_101"
              },
              {
                "name": "Chinook",
                "id": "DOG_102"
              },
              {
                "name": "Golden Retriever",
                "id": "DOG_107"
              },
              {
                "name": "Belgian Sheepdog",
                "id": "DOG_121"
              }
            ]
          },
          "preserve": [
            "pageId"
          ],
          "item": {
            "type": "Text",
            "id": "${data.id}",
            "text": "${data.name}=${index}"
          },
          "data": "${Array.slice(DOG_LIST, environment.reason == 'initial' ? 0 : 1)}"
        }
      },
      "onConfigChange": [
        {
          "type": "Reinflate"
        }
      ]
    }
)apl";


/**
 * Preserve the current ID of a page in the pager, even though a reinflation changes which pages
 * are included.
 */
TEST_F(BuilderPreserveTest, PagerPreserveId)
{
    config->pagerChildCache(10);  // Cache all pages (simplifies dirty)
    metrics.size(1000,500);
    loadDocument(PAGER_PRESERVE_ID);
    ASSERT_TRUE(component);
    advanceTime(10);
    root->clearDirty();

    ASSERT_EQ(4, component->getChildCount());
    auto currentPage = component->getCalculated(kPropertyCurrentPage).asInt();
    ASSERT_EQ(0, currentPage);
    ASSERT_TRUE(IsEqual("Briard=0", component->getChildAt(currentPage)->getCalculated(kPropertyText).asString()));

    component->update(kUpdatePagerPosition, 2);
    currentPage = component->getCalculated(kPropertyCurrentPage).asInt();
    ASSERT_EQ(2, currentPage);
    ASSERT_TRUE(IsEqual("Golden Retriever=2", component->getChildAt(currentPage)->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage)); // Update just asks to move, we control when this happens
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(CheckDirtyVisualContext(root, component));  // Visual context has changed

    configChangeReinflate(ConfigurationChange().theme("blue"));
    ASSERT_EQ(3, component->getChildCount());
    currentPage = component->getCalculated(kPropertyCurrentPage).asInt();
    ASSERT_EQ(1, currentPage);
    ASSERT_TRUE(IsEqual("Golden Retriever=1", component->getChildAt(currentPage)->getCalculated(kPropertyText).asString()));

    ASSERT_TRUE(CheckDirty(root));
    ASSERT_TRUE(CheckDirtyVisualContext(root));
}


static const char *PAGER_SET_VALUE = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Pager",
          "id": "DogPager",
          "bind": {
            "name": "DOG_LIST",
            "value": [
              {
                "name": "Briard",
                "id": "DOG_101"
              },
              {
                "name": "Chinook",
                "id": "DOG_102"
              },
              {
                "name": "Golden Retriever",
                "id": "DOG_107"
              },
              {
                "name": "Belgian Sheepdog",
                "id": "DOG_121"
              }
            ]
          },
          "item": {
            "type": "Text",
            "id": "${data.id}",
            "text": "${data.name}=${index}"
          },
          "data": "${Array.slice(DOG_LIST, environment.reason == 'initial' ? 0 : 1)}"
        }
      }
    }
)apl";

/**
 * Introducing the "pageId" and "pageIndex" properties also allows us to change the pager by calling SetValue
 */
TEST_F(BuilderPreserveTest, PagerChangePages)
{
    config->pagerChildCache(10);   // Set the cache so that all pages will be laid out immediately
    metrics.size(1000,500);
    loadDocument(PAGER_SET_VALUE);
    ASSERT_TRUE(component);
    advanceTime(10);
    root->clearDirty();

    ASSERT_EQ(4, component->getChildCount());
    auto currentPage = component->getCalculated(kPropertyCurrentPage).asInt();
    ASSERT_EQ(0, currentPage);
    ASSERT_TRUE(IsEqual("Briard=0", component->getChildAt(currentPage)->getCalculated(kPropertyText).asString()));

    // SetValue to pageId="DOG_121
    executeCommand("SetValue", {{"componentId", "DogPager"}, {"property", "pageId"}, {"value", "DOG_121"}}, true);
    currentPage = component->getCalculated(kPropertyCurrentPage).asInt();
    ASSERT_EQ(3, currentPage);
    ASSERT_TRUE(IsEqual("Belgian Sheepdog=3", component->getChildAt(currentPage)->getCalculated(kPropertyText).asString()));

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(root->isVisualContextDirty());
    root->clearVisualContextDirty();

    // SetValue to pageId=Unique ID of one of the components
    auto uid = component->getChildAt(2)->getUniqueId();
    executeCommand("SetValue", {{"componentId", "DogPager"}, {"property", "pageId"}, {"value", uid}}, true);
    currentPage = component->getCalculated(kPropertyCurrentPage).asInt();
    ASSERT_EQ(2, currentPage);
    ASSERT_TRUE(IsEqual("Golden Retriever=2", component->getChildAt(currentPage)->getCalculated(kPropertyText).asString()));

    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(root->isVisualContextDirty());
    root->clearVisualContextDirty();

    // SetValue to pageIndex=1
    executeCommand("SetValue", {{"componentId", "DogPager"}, {"property", "pageIndex"}, {"value", 1}}, true);
    currentPage = component->getCalculated(kPropertyCurrentPage).asInt();
    ASSERT_EQ(1, currentPage);
    ASSERT_TRUE(IsEqual("Chinook=1", component->getChildAt(currentPage)->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component, kPropertyCurrentPage));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(root->isVisualContextDirty());
}


static const char *PAGER_EVENT_HANDLERS_IN_REINFLATE = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "item": {
          "type": "Container",
          "items": [
            {
              "type": "Text",
              "id": "TEXT"
            },
            {
              "type": "Pager",
              "id": "PAGER",
              "preserve": [
                "pageIndex"
              ],
              "data": [
                "Snuffles",
                "Rex",
                "Spot",
                "Clifford",
                "Mudge"
              ],
              "item": {
                "type": "Text",
                "text": "${data}"
              },
              "onPageChanged": {
                "type": "SetValue",
                "componentId": "TEXT",
                "property": "text",
                "value": "Page: ${event.source.page}"
              }
            }
          ]
        }
      }
    }
)apl";

TEST_F(BuilderPreserveTest, PagerEventHandlersInRefinflate)
{
    metrics.size(400,400);
    loadDocument(PAGER_EVENT_HANDLERS_IN_REINFLATE);
    ASSERT_TRUE(component);
    auto text = root->findComponentById("TEXT");
    ASSERT_TRUE(text);
    auto pager = root->findComponentById("PAGER");
    ASSERT_TRUE(pager);
    ASSERT_TRUE(IsEqual("", text->getCalculated(kPropertyText).asString()));  // No text to start with

    // SetValue to pageId=2
    executeCommand("SetValue", {{"componentId", "PAGER"}, {"property", "pageIndex"}, {"value", 2}}, true);
    ASSERT_EQ(2, pager->getCalculated(kPropertyCurrentPage).asInt());
    ASSERT_TRUE(IsEqual("Page: 2", text->getCalculated(kPropertyText).asString()));

    // Reinflate
    configChangeReinflate(ConfigurationChange(100,100));
    text = root->findComponentById("TEXT");
    pager = root->findComponentById("PAGER");
    ASSERT_EQ(2, pager->getCalculated(kPropertyCurrentPage).asInt());  // The old page index is maintained
    ASSERT_TRUE(IsEqual("", text->getCalculated(kPropertyText).asString()));  // The old text label is gone

    // SetValue to pageID=3
    executeCommand("SetValue", {{"componentId", "PAGER"}, {"property", "pageIndex"}, {"value", 3}}, true);
    ASSERT_EQ(3, pager->getCalculated(kPropertyCurrentPage).asInt());
    ASSERT_TRUE(IsEqual("Page: 3", text->getCalculated(kPropertyText).asString()));
}


const static char *PAGER_SET_VALUE_CANCELS_AUTOPAGE = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Pager",
          "id": "PAGER",
          "data": [
            "Snuffles",
            "Rex",
            "Spot",
            "Clifford",
            "Mudge"
          ],
          "item": {
            "type": "Text",
            "text": "${data}"
          }
        }
      }
    }
)apl";

// Verify commands like AUTO-PAGE are cancelled when you set the page directly using an index or ID
TEST_F(BuilderPreserveTest, PagerSetValueCancelsAutoPage)
{
    loadDocument(PAGER_SET_VALUE_CANCELS_AUTOPAGE);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(0, component->getCalculated(kPropertyCurrentPage)));

    // Start an auto page command
    auto action = executeCommand("AutoPage", {{"componentId", "PAGER"}}, false);
    ASSERT_TRUE(IsEqual(0, component->getCalculated(kPropertyCurrentPage)));

    // Move forward in time and resolve the first auto page
    advanceTime(600);
    ASSERT_TRUE(action->isPending());
    ASSERT_TRUE(IsEqual(1, component->getCalculated(kPropertyCurrentPage)));

    // There should be another auto page waiting
    root->updateTime(250);

    // Now we set a page directly
    executeCommand("SetValue", {{"componentId", "PAGER"}, {"property", "pageIndex"}, {"value", 3}}, false);
    ASSERT_TRUE(action->isTerminated());   // The AutoPage action should be terminated
    ASSERT_TRUE(IsEqual(3, component->getCalculated(kPropertyCurrentPage)));   // We've jumped to page #3
}


static const char *VIDEO_COMPONENT_PLAY_STATE = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "Video",
          "id": "MY_VIDEO",
          "width": "100%",
          "height": "100%",
          "source": [
            "URL1",
            "URL2",
            "URL3"
          ],
          "preserve": [
            "playingState"
          ]
        }
      }
    }
)apl";

/**
 * Verify that the Video "playingState" property saves the current media state
 * over reinflation.
 */
TEST_F(BuilderPreserveTest, VideoComponentPlayState)
{
    metrics.size(300,300);
    loadDocument(VIDEO_COMPONENT_PLAY_STATE);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0,0,300,300), component->getCalculated(kPropertyBounds)));
    auto oldId = component->getUniqueId();

    MediaState ms{
        1,      // Track index
        3,      // Track count
        1003,   // Current time
        3003,   // Duration
        false,  // Paused
        false,  // Ended
    };
    component->updateMediaState(ms, false);
    ASSERT_TRUE(IsEqual(1, component->getCalculated(kPropertyTrackIndex)));
    ASSERT_TRUE(IsEqual(3, component->getCalculated(kPropertyTrackCount)));
    ASSERT_TRUE(IsEqual(1003, component->getCalculated(kPropertyTrackCurrentTime)));
    ASSERT_TRUE(IsEqual(3003, component->getCalculated(kPropertyTrackDuration)));
    ASSERT_TRUE(IsEqual(false, component->getCalculated(kPropertyTrackPaused)));
    ASSERT_TRUE(IsEqual(false, component->getCalculated(kPropertyTrackEnded)));

    configChangeReinflate(ConfigurationChange(200,200));
    // Verify that the component changed on the reinflation
    ASSERT_TRUE(IsEqual(Rect(0,0,200,200), component->getCalculated(kPropertyBounds)));
    ASSERT_FALSE(IsEqual(oldId, component->getUniqueId()));
    // Verify that the media state properties were restored
    ASSERT_TRUE(IsEqual(1, component->getCalculated(kPropertyTrackIndex)));
    ASSERT_TRUE(IsEqual(3, component->getCalculated(kPropertyTrackCount)));
    ASSERT_TRUE(IsEqual(1003, component->getCalculated(kPropertyTrackCurrentTime)));
    ASSERT_TRUE(IsEqual(3003, component->getCalculated(kPropertyTrackDuration)));
    ASSERT_TRUE(IsEqual(false, component->getCalculated(kPropertyTrackPaused)));
    ASSERT_TRUE(IsEqual(false, component->getCalculated(kPropertyTrackEnded)));
}


static const char *VIDEO_COMPONENT_SOURCE = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "Video",
          "id": "MY_VIDEO",
          "width": "100%",
          "height": "100%",
          "source": [
            "URL1",
            "URL2",
            "URL3"
          ],
          "preserve": [
            "source"
          ]
        }
      }
    }
)apl";

/**
 * Verify that the Video component "source" property saves the list of source material
 * over a reinflation.
 */
TEST_F(BuilderPreserveTest, VideoComponentSource)
{
    metrics.size(200,200);
    loadDocument(VIDEO_COMPONENT_SOURCE);
    ASSERT_TRUE(component);

    // Change the sources
    executeCommand("SetValue", {{"componentId", "MY_VIDEO"},
                                {"property", "source"},
                                {"value", ObjectArray{"FOO1", "FOO2"}}}, true);
    ASSERT_TRUE(CheckDirty(component, kPropertySource, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));
    
    // Reinflate
    configChangeReinflate(ConfigurationChange(300,300));
    auto sources = component->getCalculated(kPropertySource);
    ASSERT_TRUE(sources.isArray());
    ASSERT_EQ(2, sources.size());
    ASSERT_TRUE(IsEqual("FOO1", sources.at(0).getMediaSource().getUrl()));
    ASSERT_TRUE(IsEqual("FOO2", sources.at(1).getMediaSource().getUrl()));
}

static const char *PRESERVE_BOUND_VALUES = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": [
        {
          "type": "SetValue",
          "componentId": "MAIN",
          "property": "X",
          "value": 2
        },
        {
          "type": "Reinflate"
        }
      ],
      "mainTemplate": {
        "items": {
          "type": "Container",
          "id": "MAIN",
          "bind": {
            "name": "X",
            "value": 1
          },
          "preserve": "X",
          "items": [
            {
              "type": "Text",
              "when": "${X == 1}",
              "text": "X is one"
            },
            {
              "type": "Text",
              "when": "${X == 2}",
              "text": "X is two"
            }
          ]
        }
      }
    }
)apl";

TEST_F(BuilderPreserveTest, PreserveBoundValues)
{
    loadDocument(PRESERVE_BOUND_VALUES);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    auto child = component->getChildAt(0);
    ASSERT_TRUE(IsEqual("X is one", child->getCalculated(kPropertyText).asString()));

    // Reinflate
    configChangeReinflate(ConfigurationChange(233,344));
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    child = component->getChildAt(0);
    ASSERT_TRUE(IsEqual("X is two", child->getCalculated(kPropertyText).asString()));
}

// TODO: Check that TransformAssigned works - this is trickier to copy and compare
// TODO: What about graphic elements? These need to have an ID assigned to them to store state AND match at the Graphic level

// TODO: Verify that the path of where the component is inflated doesn't matter
// TODO: "focused"

// TODO: Verify that "hover" works after a re-layout

// TODO: Ensure that the layout pass is completed when necessary

// TODO: Verify that elapsedTime, UTC time, and UTC time adjustment stay the same
// TODO: Verify that changing the screen metrics results in an appropriate new layout (resize)

// TODO: Verify that no components are dirty after a resize.  The visual context should be marked the same way
