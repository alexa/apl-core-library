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

class BuilderTestSequence : public DocumentWrapper {};

const char *SIMPLE_SEQUENCE = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "height": 100,
      "items": [
        {
          "type": "Text",
          "height": 100
        },
        {
          "type": "Text",
          "height": 100
        }
      ]
    }
  }
})";

TEST_F(BuilderTestSequence, Simple)
{
    loadDocument(SIMPLE_SEQUENCE);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // Standard properties
    ASSERT_EQ("", component->getCalculated(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(2, component->getCalculated(kPropertyAccessibilityActions).size());
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(Object(Dimension(100)), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(1.0, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingBottom)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingLeft)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingRight)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingTop)));
    ASSERT_TRUE(IsEqual(Object(ObjectArray{}), component->getCalculated(kPropertyPadding)));
    ASSERT_EQ(Object(kSnapNone), component->getCalculated(kPropertySnap));
    ASSERT_EQ(Object(1.0), component->getCalculated(kPropertyFastScrollScale));
    ASSERT_EQ(Object(kScrollAnimationDefault), component->getCalculated(kPropertyScrollAnimation));
    ASSERT_EQ(Object(Dimension()), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Sequence properties
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).getInteger());
    ASSERT_EQ(false, component->getCalculated(kPropertyNumbered).getBoolean());

    // Children
    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 1), true));

    auto scrollPosition = component->getCalculated(kPropertyScrollPosition);
    ASSERT_TRUE(scrollPosition.isDimension());
    ASSERT_EQ(0, scrollPosition.asNumber());
}

TEST_F(BuilderTestSequence, SimpleScrolled)
{
    loadDocument(SIMPLE_SEQUENCE);

    ASSERT_EQ(ScrollType::kScrollTypeVertical, component->scrollType());
    ASSERT_TRUE(component->allowForward());
    ASSERT_FALSE(component->allowBackwards());

    component->update(kUpdateScrollPosition, 1000);
    root->clearPending();
    ASSERT_FALSE(component->allowForward());
    ASSERT_TRUE(component->allowBackwards());
}

const static char *SIMPLE_HORIZONTAL_SEQUENCE_RTL = R"(
    {
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "items": {
          "type": "Sequence",
          "layoutDirection": "RTL",
          "width": "100%",
          "scrollDirection": "horizontal",
          "items": {
            "type": "Frame",
            "width": "400",
            "height": "100%"
          },
          "data": [
            1,
            2,
            3,
            4
          ]
        }
      }
    }
)";

TEST_F(BuilderTestSequence, SimpleHorizontalSequenceRTL)
{
    loadDocument(SIMPLE_HORIZONTAL_SEQUENCE_RTL);
    advanceTime(10);
    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(kScrollDirectionHorizontal, component->getCalculated(kPropertyScrollDirection).asInt());

    for (int i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(Rect(624-400*i, 0, 400, 800), child->getCalculated(kPropertyBounds).get<Rect>());
    }

    // Children
    ASSERT_EQ(4, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 3), true));

    auto scrollPosition = component->getCalculated(kPropertyScrollPosition);
    ASSERT_TRUE(scrollPosition.isDimension());
    ASSERT_EQ(0, scrollPosition.asNumber());

    ASSERT_EQ(ScrollType::kScrollTypeHorizontal, component->scrollType());
    ASSERT_TRUE(component->allowForward());
    ASSERT_FALSE(component->allowBackwards());

    component->update(kUpdateScrollPosition, -1000);
    root->clearPending();
    ASSERT_FALSE(component->allowForward());
    ASSERT_TRUE(component->allowBackwards());
}

const char *EMPTY_SEQUENCE = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "height": 100
    }
  }
})";

TEST_F(BuilderTestSequence, Empty)
{
    loadDocument(EMPTY_SEQUENCE);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // Standard properties
    ASSERT_EQ("", component->getCalculated(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(2, component->getCalculated(kPropertyAccessibilityActions).size());
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(Object(Dimension(100)), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(1.0, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingBottom)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingLeft)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingRight)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingTop)));
    ASSERT_TRUE(IsEqual(Object(ObjectArray{}), component->getCalculated(kPropertyPadding)));
    ASSERT_EQ(Object(Dimension()), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Sequence properties
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).getInteger());
    ASSERT_EQ(false, component->getCalculated(kPropertyNumbered).getBoolean());

    // Children
    ASSERT_EQ(0, component->getChildCount());
}

const char *CHILDREN_TEST = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "scrollDirection": "horizontal",
      "width": "1000",
      "snap": "center",
      "-fastScrollScale": 0.5,
      "-scrollAnimation": "smoothInOut",
      "numbered": true,
      "data": [
        "One",
        "Two",
        "Three",
        "Four",
        "Five"
      ],
      "items": [
        {
          "when": "${data == 'Two'}",
          "type": "Text",
          "text": "A ${index}-${ordinal}-${length}",
          "numbering": "reset"
        },
        {
          "when": "${data == 'Four'}",
          "type": "Text",
          "text": "B ${index}-${ordinal}-${length}",
          "numbering": "skip",
          "spacing": 20
        },
        {
          "type": "Text",
          "text": "C ${index}-${ordinal}-${length}"
        }
      ]
    }
  }
})";

TEST_F(BuilderTestSequence, Children)
{
    loadDocument(CHILDREN_TEST);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(kScrollDirectionHorizontal, component->getCalculated(kPropertyScrollDirection).getInteger());
    ASSERT_EQ(kSnapCenter, component->getCalculated(kPropertySnap).getInteger());
    ASSERT_EQ(0.5, component->getCalculated(kPropertyFastScrollScale).getDouble());
    ASSERT_TRUE(IsEqual(Dimension(1000), component->getCalculated(kPropertyWidth)));
    ASSERT_EQ(kScrollAnimationSmoothInOut, component->getCalculated(kPropertyScrollAnimation).getInteger());
    ASSERT_TRUE(IsEqual(Dimension(), component->getCalculated(kPropertyHeight)));

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 3), true));

    auto child = component->getChildAt(0)->getCalculated();
    ASSERT_EQ(Object("C 0-1-5"), child.get(kPropertyText).asString());
    ASSERT_EQ(Object(Dimension(0)), child.get(kPropertySpacing));

    child = component->getChildAt(1)->getCalculated();
    ASSERT_EQ(Object("A 1-2-5"), child.get(kPropertyText).asString());
    ASSERT_EQ(Object(Dimension(0)), child.get(kPropertySpacing));

    child = component->getChildAt(2)->getCalculated();
    ASSERT_EQ(Object("C 2-1-5"), child.get(kPropertyText).asString());
    ASSERT_EQ(Object(Dimension(0)), child.get(kPropertySpacing));

    child = component->getChildAt(3)->getCalculated();
    ASSERT_EQ(Object("B 3-2-5"), child.get(kPropertyText).asString());
    ASSERT_EQ(Object(Dimension(20)), child.get(kPropertySpacing));

    child = component->getChildAt(4)->getCalculated();
    ASSERT_EQ(Object("C 4-2-5"), child.get(kPropertyText).asString());
    ASSERT_EQ(Object(Dimension(0)), child.get(kPropertySpacing));
}

const char *LAYOUT_CACHE_TEST = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "height": 100,
      "width": "auto",
      "data": [0, 1, 2, 3, 4, 5, 6],
      "items": [
        {
          "type": "Text",
          "height": 50,
          "text": "${data}"
        }
      ]
    }
  }
})";

TEST_F(BuilderTestSequence, LayoutCache)
{
    loadDocument(LAYOUT_CACHE_TEST);
    advanceTime(10);
    auto map = component->getCalculated();

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(7, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 4), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(5, 6), false));
}

const char *LAYOUT_CACHE_TEST_HORIZONTAL = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "width": 200,
      "scrollDirection": "horizontal",
      "height": "auto",
      "data": [0, 1, 2, 3, 4, 5],
      "items": [
        {
          "type": "Text",
          "width": 100,
          "text": "${data}"
        }
      ]
    }
  }
})";

TEST_F(BuilderTestSequence, LayoutCacheHorizontal)
{
    loadDocument(LAYOUT_CACHE_TEST_HORIZONTAL);
    advanceTime(10);
    ASSERT_EQ(kScrollDirectionHorizontal, component->getCalculated(kPropertyScrollDirection).getInteger());

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(6, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 4), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(5, 5), false));

    component->update(kUpdateScrollPosition, 100);
    advanceTime(10);
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 5), true));
}

TEST_F(BuilderTestSequence, LayoutCacheHorizontalRTL)
{
    loadDocument(LAYOUT_CACHE_TEST_HORIZONTAL);
    advanceTime(10);
    component->update(kUpdateScrollPosition, 50);
    advanceTime(10);
    ASSERT_EQ(Point(50, 0), component->scrollPosition());
    ASSERT_EQ(6, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 4), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(5, 5), false));

    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    ASSERT_EQ(kLayoutDirectionRTL, component->getCalculated(kPropertyLayoutDirection).getInteger());
    ASSERT_EQ(kScrollDirectionHorizontal, component->getCalculated(kPropertyScrollDirection).getInteger());

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(6, component->getChildCount());
    ASSERT_EQ(Point(-50, 0), component->scrollPosition());
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 4), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(5, 5), false));

    component->update(kUpdateScrollPosition, -100);
    advanceTime(10);
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 5), true));
    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(Rect(100 - 100*i, 0, 100, 800), child->getCalculated(kPropertyBounds).get<Rect>());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(100 - 100*i + 100, 0, 100, 800), rect);
    }

    component->setProperty(kPropertyLayoutDirectionAssigned, "LTR");
    root->clearPending();

    ASSERT_EQ(6, component->getChildCount());
    ASSERT_EQ(Point(100, 0), component->scrollPosition());
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 5), true));
}

static const char* MULTISEQUENCE = R"apl({
  "type": "APL",
  "version": "1.5",
  "layouts": {
    "ScrollyRow": {
      "parameters": [
        "parent"
      ],
      "item": {
        "type": "Sequence",
        "scrollDirection": "horizontal",
        "id": "${parent}",
        "width": 200,
        "height": 100,
        "data": [0, 1],
        "item": {
          "type": "TouchWrapper",
          "id": "${parent}.${data}",
          "width": 100,
          "height": 100,
          "entities": ["entity"],
            "item": {
              "type": "Text",
              "width": 100,
              "height": 100,
              "text": "${parent}.${data}",
              "color": "white"
            }
        }
      }
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "id": "root",
      "width": 200,
      "height": 100,
      "item": {
        "type": "ScrollyRow",
        "parent": "${data}"
      },
      "data": [0, 1]
    }
  }
})apl";

TEST_F(BuilderTestSequence, Multisequence) {
    loadDocument(MULTISEQUENCE);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // Should stay at 0.
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
}

const char *INVALID_SCROLL_ANIMATION = R"({
  "type": "APL",
  "version": "1.5",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "-scrollAnimation": "foo",
      "height": 100,
      "items": [
        {
          "type": "Text"
        },
        {
          "type": "Text"
        }
      ]
    }
  }
})";

TEST_F(BuilderTestSequence, InvalidScrollAnimation)
{
    loadDocument(INVALID_SCROLL_ANIMATION);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // inavlid value replaced by default
    ASSERT_EQ(kScrollAnimationDefault, component->getCalculated(kPropertyScrollAnimation).getInteger());

    // Children
    ASSERT_EQ(2, component->getChildCount());
}

const char *NONE_SEQUENCE = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "items": {
        "type": "Sequence",
        "display": "none",
        "items": {
          "type": "Text",
          "text": "${data}"
        },
        "data": "${Array.range(50)}"
      }
    }
  }
})";

TEST_F(BuilderTestSequence, DisplayNone)
{
    loadDocument(NONE_SEQUENCE);

    auto measurement = std::static_pointer_cast<MyTestMeasurement>(config->getMeasure());

    // Nothing should try laying out
    ASSERT_EQ(0, measurement->getLayoutCount());

    auto sequence = component->getCoreChildAt(0);

    // Force trimScroll
    sequence->update(kUpdateScrollPosition, 100);

    // Nothing should have happened
    ASSERT_EQ(0, measurement->getLayoutCount());
    ASSERT_EQ(0, sequence->scrollPosition().getY());

    // Now make it appear
    sequence->setProperty(kPropertyDisplay, kDisplayNormal);
    root->clearPending();

    // We now require some measures.
    ASSERT_EQ(21, measurement->getLayoutCount());

    // And scrolling works
    sequence->update(kUpdateScrollPosition, 100);
    ASSERT_EQ(100, sequence->scrollPosition().getY());
}

const char *NONE_NESTED_SEQUENCE = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "items": {
        "type": "Container",
        "width": "100%",
        "height": "100%",
        "display": "none",
        "items": {
          "type": "Sequence",
          "items": {
            "type": "Text",
            "text": "${data}"
          },
          "data": "${Array.range(50)}"
        }
      }
    }
  }
})";

TEST_F(BuilderTestSequence, DisplayNoneNested)
{
    loadDocument(NONE_NESTED_SEQUENCE);
    auto measurement = std::static_pointer_cast<MyTestMeasurement>(config->getMeasure());

    // Nothing should try laying out
    ASSERT_EQ(0, measurement->getLayoutCount());

    auto sequence = component->getCoreChildAt(0)->getCoreChildAt(0);

    // Force trimScroll
    sequence->update(kUpdateScrollPosition, 100);

    // Nothing should have happened
    ASSERT_EQ(0, measurement->getLayoutCount());
    ASSERT_EQ(0, sequence->scrollPosition().getY());

    // Now make it appear
    component->getCoreChildAt(0)->setProperty(kPropertyDisplay, kDisplayNormal);
    root->clearPending();

    // We now require some measures.
    ASSERT_EQ(21, measurement->getLayoutCount());

    // And scrolling works
    sequence->update(kUpdateScrollPosition, 100);
    ASSERT_EQ(100, sequence->scrollPosition().getY());
}

const char *AUTO_SEQUENCE_SIZING = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "height": "100%",
      "width": "100%",
      "items": {
        "type": "Frame",
        "borderColor": "red",
        "borderWidth": "2dp",
        "height": "auto",
        "width": "100%",
        "items": {
          "type": "Container",
          "height": "auto",
          "width": "100%",
          "items": {
            "type": "Sequence",
            "height": "auto",
            "width": "100%",
            "data": ["green","blue","purple","white"],
            "scrollDirection": "horizontal",
            "items": [
              {
                "type": "Frame",
                "borderColor": "${data}",
                "borderWidth": "2dp",
                "height": "100",
                "width": "100"
              }
            ]
          }
        }
      }
    }
  }
})";

TEST_F(BuilderTestSequence, AutoSequenceSizing)
{
    loadDocument(AUTO_SEQUENCE_SIZING);

    auto frame = component->getCoreChildAt(0);
    ASSERT_EQ(Object(Rect(0, 0, 1024, 104)), frame->getCalculated(kPropertyBounds));
}

const char *RTL_SEQUENCE_VERTICAL_LOAD_TEST = R"(
{
  "type": "APL",
  "version": "1.9",
  "mainTemplate": {
    "parameters": [
      "layoutDir",
      "scrollDir"
    ],
    "items": {
      "type": "Sequence",
      "scrollDirection": "${scrollDir}",
      "layoutDirection": "${layoutDir}",
      "items": {
        "type": "Text",
        "id": "${data}",
        "text": "${data}"
      },
      "data": "${TestArray}"
    }
  }
}
)";

// Test that the correct number of children are inflated as a sequence scrolls regardless of layoutDirection
TEST_F(BuilderTestSequence, SequenceInflationTestVerticalRTL)
{
    std::vector<Object> v;
    for( int i = 0; i < 50; i++ )
        v.push_back( i );

    auto myArray = LiveArray::create(std::move(v));
    config->liveData("TestArray", myArray);

    config->set(RootProperty::kSequenceChildCache, 5);

    loadDocument(RTL_SEQUENCE_VERTICAL_LOAD_TEST, "{\"layoutDir\": \"RTL\", \"scrollDir\": \"vertical\"}");

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 10), true));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(11, 49), false));

    component->update(kUpdateScrollPosition, 50);
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 14), true));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(15, 49), false));

    for (int i = 0; i < 50; i++)
        myArray->insert(0, -i);

    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 3), false));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(4, 99), true));
}

// Test that the correct number of children are inflated as a sequence scrolls regardless of layoutDirection
TEST_F(BuilderTestSequence, SequenceInflationTestVerticalLTR)
{
    std::vector<Object> v;
    for( int i = 0; i < 50; i++ )
        v.push_back( i );

    auto myArray = LiveArray::create(std::move(v));
    config->liveData("TestArray", myArray);
    config->set(RootProperty::kSequenceChildCache, 5);

    loadDocument(RTL_SEQUENCE_VERTICAL_LOAD_TEST, "{\"layoutDir\": \"LTR\", \"scrollDir\": \"vertical\"}");

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 10), true));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(11, 49), false));

    component->update(kUpdateScrollPosition, 50);
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 14), true));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(15, 49), false));

    for (int i = 0; i < 50; i++)
        myArray->insert(0, -i);

    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 3), false));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(4, 99), true));
}

// Test that the correct number of children are inflated as a sequence scrolls regardless of layoutDirection
TEST_F(BuilderTestSequence, SequenceInflationTestHorizontalRTL)
{
    std::vector<Object> v;
    for( int i = 0; i < 50; i++ )
        v.push_back( i );

    auto myArray = LiveArray::create(std::move(v));
    config->liveData("TestArray", myArray);

    config->set(RootProperty::kSequenceChildCache, 5);

    loadDocument(RTL_SEQUENCE_VERTICAL_LOAD_TEST, "{\"layoutDir\": \"RTL\", \"scrollDir\": \"horizontal\"}");

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 10), true));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(11, 49), false));

    component->update(kUpdateScrollPosition, -100);
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 14), true));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(15, 49), false));

    for (int i = 0; i < 50; i++)
        myArray->insert(0, -i);

    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 31), false));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(32, 90), true));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(91, 99), false));
}

// Test that the correct number of children are inflated as a sequence scrolls regardless of layoutDirection
TEST_F(BuilderTestSequence, SequenceInflationTestHorizontalLTR)
{
    std::vector<Object> v;
    for( int i = 0; i < 50; i++ )
        v.push_back( i );

    auto myArray = LiveArray::create(std::move(v));
    config->liveData("TestArray", myArray);

    config->set(RootProperty::kSequenceChildCache, 5);

    loadDocument(RTL_SEQUENCE_VERTICAL_LOAD_TEST, "{\"layoutDir\": \"LTR\", \"scrollDir\": \"horizontal\"}");

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 10), true));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(11, 49), false));

    component->update(kUpdateScrollPosition, 100);
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 14), true));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(15, 49), false));

    for (int i = 0; i < 50; i++)
        myArray->insert(0, -i);

    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 31), false));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(32, 90), true));

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(91, 99), false));
}

const char *AUTO_SIZE_TEXT_CHILD = R"({
  "type": "APL",
  "version": "2023.2",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "height": 800,
      "width": 800,
      "items": [
        {
          "type": "Sequence",
          "height": 20,
          "width": 50,
          "items": {
            "type": "Text",
            "width": "auto",
            "height": "auto",
            "text": "text text text text text text text text"
          }
        },
        {
          "type": "ScrollView",
          "height": 20,
          "width": 50,
          "items": {
            "type": "Text",
            "width": "auto",
            "height": "auto",
            "text": "text text text text text text text text"
          }
        }
      ]
    }
  }
})";

TEST_F(BuilderTestSequence, AutoSizeTextChild)
{
    loadDocument(AUTO_SIZE_TEXT_CHILD);

    ASSERT_TRUE(component);

    ASSERT_EQ(Rect(0, 0, 50, 20), component->getChildAt(0)->getCalculated(apl::kPropertyBounds).get<Rect>());
    auto textBounds = component->getChildAt(0)->getChildAt(0)->getCalculated(apl::kPropertyBounds).get<Rect>();
    ASSERT_EQ(Rect(0, 0, 50, 80), textBounds);

    ASSERT_EQ(Rect(0, 20, 50, 20), component->getChildAt(1)->getCalculated(apl::kPropertyBounds).get<Rect>());
    textBounds = component->getChildAt(1)->getChildAt(0)->getCalculated(apl::kPropertyBounds).get<Rect>();
    ASSERT_EQ(Rect(0, 0, 50, 80), textBounds);
}

// Test that kPropertyScrollPosition is 0 after the first item is removed
TEST_F(BuilderTestSequence, SequenceRebuildLiveDataFirstChildRemovedVerticalLTR)
{
    std::vector<Object> v;
    for( int i = 0; i < 50; i++ )
        v.push_back( i );

    auto myArray = LiveArray::create(std::move(v));
    config->liveData("TestArray", myArray);
    config->set(RootProperty::kSequenceChildCache, 5);

    loadDocument(RTL_SEQUENCE_VERTICAL_LOAD_TEST, "{\"layoutDir\": \"LTR\", \"scrollDir\": \"vertical\"}");

    ASSERT_EQ(component->getCalculated(apl::kPropertyScrollPosition).asNumber(), 0);
    ASSERT_EQ(component->getChildCount(), 50);

    myArray->remove(0, 1);
    root->clearPending();

    ASSERT_EQ(component->getCalculated(apl::kPropertyScrollPosition).asNumber(), 0);
    ASSERT_EQ(component->getChildCount(), 49);
}

// Test that kPropertyScrollPosition is 0 after the first item is removed
TEST_F(BuilderTestSequence, SequenceRebuildLiveDataFirstChildRemovedVerticalRTL)
{
    std::vector<Object> v;
    for( int i = 0; i < 50; i++ )
        v.push_back( i );

    auto myArray = LiveArray::create(std::move(v));
    config->liveData("TestArray", myArray);
    config->set(RootProperty::kSequenceChildCache, 5);

    loadDocument(RTL_SEQUENCE_VERTICAL_LOAD_TEST, "{\"layoutDir\": \"RTL\", \"scrollDir\": \"vertical\"}");

    ASSERT_EQ(component->getCalculated(apl::kPropertyScrollPosition).asNumber(), 0);
    ASSERT_EQ(component->getChildCount(), 50);

    myArray->remove(0, 1);
    root->clearPending();

    ASSERT_EQ(component->getCalculated(apl::kPropertyScrollPosition).asNumber(), 0);
    ASSERT_EQ(component->getChildCount(), 49);
}

// Test that kPropertyScrollPosition is 0 after the first item is removed
TEST_F(BuilderTestSequence, SequenceRebuildLiveDataFirstChildRemovedHorizontalLTR)
{
    std::vector<Object> v;
    for( int i = 0; i < 50; i++ )
        v.push_back( i );

    auto myArray = LiveArray::create(std::move(v));
    config->liveData("TestArray", myArray);
    config->set(RootProperty::kSequenceChildCache, 5);

    loadDocument(RTL_SEQUENCE_VERTICAL_LOAD_TEST, "{\"layoutDir\": \"LTR\", \"scrollDir\": \"horizontal\"}");

    ASSERT_EQ(component->getCalculated(apl::kPropertyScrollPosition).asNumber(), 0);
    ASSERT_EQ(component->getChildCount(), 50);

    myArray->remove(0, 1);
    root->clearPending();

    ASSERT_EQ(component->getCalculated(apl::kPropertyScrollPosition).asNumber(), 0);
    ASSERT_EQ(component->getChildCount(), 49);
}

// Test that kPropertyScrollPosition is 0 after the first item is removed
TEST_F(BuilderTestSequence, SequenceRebuildLiveDataFirstChildRemovedHorizontalRTL)
{
    std::vector<Object> v;
    for( int i = 0; i < 50; i++ )
        v.push_back( i );

    auto myArray = LiveArray::create(std::move(v));
    config->liveData("TestArray", myArray);
    config->set(RootProperty::kSequenceChildCache, 5);

    loadDocument(RTL_SEQUENCE_VERTICAL_LOAD_TEST, "{\"layoutDir\": \"RTL\", \"scrollDir\": \"horizontal\"}");

    ASSERT_EQ(component->getCalculated(apl::kPropertyScrollPosition).asNumber(), 0);
    ASSERT_EQ(component->getChildCount(), 50);

    myArray->remove(0, 1);
    root->clearPending();

    ASSERT_EQ(component->getCalculated(apl::kPropertyScrollPosition).asNumber(), 0);
    ASSERT_EQ(component->getChildCount(), 49);
}

static const char *SEQUENCE_SCROLL_OFFSET = R"({
  "type": "APL",
  "version": "1.7",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "id": "testSequence",
      "width": 100,
      "height": 100,
      "preserve": [
         "scrollOffset"
      ],
      "data": "${TestArray}",
      "item": {
        "type": "Frame",
        "width": "100%",
        "height": 60
      }
    }
  }
})";

TEST_F(BuilderTestSequence, ScrollOffsetReinflate) {
    auto myArray = LiveArray::create(ObjectArray{0, 1, 2, 3, 4, 5});
    config->liveData("TestArray", myArray);

    metrics.size(200,200);
    loadDocument(SEQUENCE_SCROLL_OFFSET);
    ASSERT_TRUE(component);
    ASSERT_EQ(6, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0,1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2,3}, false));
    root->clearDirty();

    advanceTime(10);
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0,3}, true));

    // Trigger reinflate
    configChangeReinflate(ConfigurationChange(200,200));
    ASSERT_TRUE(component);

    ASSERT_EQ(6, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0,1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2,3}, false));
    root->clearDirty();

    // Validate second layout pass when scroll position need not to be adjusted after re-inflation
    advanceTime(10);
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0,3}, true));
}
