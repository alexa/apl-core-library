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

class BuilderTestPager : public DocumentWrapper {
public:
    ::testing::AssertionResult
    CheckChild(size_t idx, const std::string& id, const Rect& bounds) {
        auto child = component->getChildAt(idx);

        auto actualId = child->getId();
        if (id != actualId) {
            return ::testing::AssertionFailure()
                    << "child " << idx
                    << " id is wrong. Expected: " << id
                    << ", actual: " << actualId;
        }

        auto actualBounds = child->getCalculated(kPropertyBounds).getRect();
        if (bounds != actualBounds) {
            return ::testing::AssertionFailure()
                    << "child " << idx
                    << " bounds is wrong. Expected: " << bounds.toString()
                    << ", actual: " << actualBounds.toString();
        }

        return ::testing::AssertionSuccess();
    }
};

static const char *SIMPLE_PAGER = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "width": 100,
      "height": 200,
      "items": [
        { "type": "Text" },
        { "type": "Text" },
        { "type": "Text" }
      ]
    }
  }
})";

TEST_F(BuilderTestPager, SimplePager)
{
    loadDocument(SIMPLE_PAGER);
    advanceTime(10);
    root->clearDirty();

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypePager, component->getType());

    // Standard properties
    ASSERT_TRUE(IsEqual("", component->getCalculated(kPropertyAccessibilityLabel)));
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyAccessibilityActions));
    ASSERT_TRUE(IsEqual(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled)));
    ASSERT_TRUE(IsEqual(Dimension(200), component->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxHeight)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxWidth)));
    ASSERT_TRUE(IsEqual(Dimension(0), component->getCalculated(kPropertyMinHeight)));
    ASSERT_TRUE(IsEqual(Dimension(0), component->getCalculated(kPropertyMinWidth)));
    ASSERT_TRUE(IsEqual(1.0, component->getCalculated(kPropertyOpacity).getDouble()));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingBottom)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingLeft)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingRight)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingTop)));
    ASSERT_TRUE(IsEqual(Object(ObjectArray{}), component->getCalculated(kPropertyPadding)));
    ASSERT_TRUE(IsEqual(Dimension(100), component->getCalculated(kPropertyWidth)));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Pager properties
    ASSERT_EQ(0, component->getCalculated(kPropertyInitialPage).getInteger());
    ASSERT_EQ(0, component->getCalculated(kPropertyCurrentPage).getInteger());
    ASSERT_EQ(kNavigationWrap, component->getCalculated(kPropertyNavigation).getInteger());

    ASSERT_TRUE(IsEqual(Rect(0, 0, 100, 200), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 2}, true));

    // Children
    ASSERT_EQ(3, component->getChildCount());

    for (int i = 0 ; i < component->getChildCount() ; i++) {
        auto text = component->getChildAt(i);

        ASSERT_TRUE(IsEqual("", text->getCalculated(kPropertyText).asString()));
        ASSERT_TRUE(IsEqual(Rect(0, 0, 100, 200), text->getCalculated(kPropertyBounds)));
    }
}

static const char *SIMPLE_PAGER_RTL = R"apl(
{
    "type": "APL",
        "version": "1.7",
        "mainTemplate": {
        "item": {
            "type": "Pager",
            "layoutDirection": "RTL",
            "width": 100,
            "height": 200,
            "items": [
            {
                "id": 1,
                "type": "Text"
            },
            {
                "id": 2,
                "type": "Text"
            },
            {
                "id": 3,
                "type": "Text"
            }
            ]
        }
    }
})apl";

TEST_F(BuilderTestPager, SimplePagerRTL)
{
    loadDocument(SIMPLE_PAGER_RTL);
    advanceTime(10);
    root->clearDirty();

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypePager, component->getType());

    // Pager properties
    ASSERT_EQ(0, component->getCalculated(kPropertyInitialPage).getInteger());
    ASSERT_EQ(0, component->getCalculated(kPropertyCurrentPage).getInteger());
    ASSERT_EQ(kNavigationWrap, component->getCalculated(kPropertyNavigation).getInteger());

    ASSERT_TRUE(IsEqual(Rect(0, 0, 100, 200), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 2}, true));

    // Children
    ASSERT_EQ(3, component->getChildCount());

    for (int i = 0 ; i < component->getChildCount() ; i++) {
        auto text = component->getChildAt(i);

        ASSERT_TRUE(IsEqual("", text->getCalculated(kPropertyText).asString()));
        ASSERT_TRUE(IsEqual(Rect(0, 0, 100, 200), text->getCalculated(kPropertyBounds)));
    }
}

static const char *PAGER_WITH_SIZES = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "width": 500,
      "height": 600,
      "items": [
        {
          "type": "Text",
          "width": "50%",
          "height": 30
        },
        {
          "type": "Text",
          "width": "auto",
          "height": "auto"
        }
      ]
    }
  }
})";

TEST_F(BuilderTestPager, PagerWithSizes)
{
    loadDocument(PAGER_WITH_SIZES);
    advanceTime(10);
    root->clearDirty();

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypePager, component->getType());

    // Standard properties
    ASSERT_TRUE(IsEqual(Rect(0, 0, 500, 600), component->getCalculated(kPropertyBounds)));

    // Children - check their sizes. They all should be 100%
    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(IsEqual(Rect(0, 0, 500, 600), component->getChildAt(0)->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0, 0, 500, 600), component->getChildAt(1)->getCalculated(kPropertyBounds)));
}

static const char *PAGER_WITH_NUMBERED = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "width": 500,
      "height": 600,
      "numbered": true,
      "items": [
        {
          "type": "Text",
          "width": "50%",
          "height": 30
        },
        {
          "type": "Text",
          "width": "auto",
          "height": "auto"
        }
      ]
    }
  }
})";

TEST_F(BuilderTestPager, PagerWithNumbered)
{
    loadDocument(PAGER_WITH_NUMBERED);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypePager, component->getType());

    // Pager inflated
    ASSERT_TRUE(IsEqual(Rect(0, 0, 500, 600), component->getCalculated(kPropertyBounds)));

    // check that children do not have an assigned ordinal
    ASSERT_EQ(2, component->getChildCount());
    ASSERT_FALSE(component->getChildAt(0)->getContext()->has("ordinal"));
    ASSERT_FALSE(component->getChildAt(1)->getContext()->has("ordinal"));
}

static const char *AUTO_SIZED_PAGER = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "width": "50%",
      "height": "auto",
      "items": [
        { "type": "Text" }
      ]
    }
  }
})";

TEST_F(BuilderTestPager, AutoSizedPager)
{
    loadDocument(AUTO_SIZED_PAGER);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypePager, component->getType());

    // Standard properties
    ASSERT_TRUE(IsEqual(Dimension(0), component->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 50), component->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0, 0, metrics.getWidth() / 2, 0), component->getCalculated(kPropertyBounds)));

    // Children - check their sizes. They all should be 100%
    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(IsEqual(Rect(0, 0, metrics.getWidth() / 2, 0), component->getChildAt(0)->getCalculated(kPropertyBounds)));
}

static const char *LAZY_PAGER = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "width": 100,
      "height": 200,
      "data": [0, 1, 2, 3],
      "navigation": "normal",
      "items": [
        {
          "type": "Text",
          "id": "${data}"
        }
      ]
    }
  }
})";

TEST_F(BuilderTestPager, LazyPager)
{
    loadDocument(LAZY_PAGER);
    advanceTime(10);
    root->clearDirty();

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypePager, component->getType());

    ASSERT_EQ(0, component->getCalculated(kPropertyCurrentPage).getInteger());
    ASSERT_EQ(kNavigationNormal, component->getCalculated(kPropertyNavigation).getInteger());

    ASSERT_TRUE(IsEqual(Rect(0, 0, 100, 200), component->getCalculated(kPropertyBounds)));

    // Children
    ASSERT_EQ(4, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 3}, false));
    ASSERT_TRUE(CheckChild(0, "0", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(1, "1", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(2, "2", Rect(0, 0, 0, 0)));
    ASSERT_TRUE(CheckChild(3, "3", Rect(0, 0, 0, 0)));

    component->update(kUpdatePagerByEvent, 1);
    root->clearPending();
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 2));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 2}, true));
    ASSERT_TRUE(CheckChildLaidOut(component, 3, false));
    ASSERT_TRUE(CheckChild(0, "0", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(1, "1", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(2, "2", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(3, "3", Rect(0, 0, 0, 0)));
}

static const char *LAZY_INITIAL_SET_PAGER = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "width": 100,
      "height": 200,
      "initialPage": 2,
      "data": [0, 1, 2, 3, 4],
      "navigation": "normal",
      "items": [
        {
          "type": "Text",
          "id": "${data}"
        }
      ]
    }
  }
})";

TEST_F(BuilderTestPager, LazyInitialSetPager)
{
    loadDocument(LAZY_INITIAL_SET_PAGER);
    advanceTime(10);
    root->clearDirty();

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypePager, component->getType());

    ASSERT_EQ(2, component->getCalculated(kPropertyCurrentPage).getInteger());
    ASSERT_EQ(kNavigationNormal, component->getCalculated(kPropertyNavigation).getInteger());

    ASSERT_TRUE(IsEqual(Rect(0, 0, 100, 200), component->getCalculated(kPropertyBounds)));

    // Children
    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildLaidOut(component, 0, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {1, 3}, true));
    ASSERT_TRUE(CheckChildLaidOut(component, 4, false));
    ASSERT_TRUE(CheckChild(0, "0", Rect(0, 0, 0, 0)));
    ASSERT_TRUE(CheckChild(1, "1", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(2, "2", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(3, "3", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(4, "4", Rect(0, 0, 0, 0)));

    component->update(kUpdatePagerByEvent, 1);
    root->clearPending();
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 0));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 3}, true));
    ASSERT_TRUE(CheckChildLaidOut(component, 4, false));
    ASSERT_TRUE(CheckChild(0, "0", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(1, "1", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(2, "2", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(3, "3", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(4, "4", Rect(0, 0, 0, 0)));
}

/*
 * Repeat a complex test with RTL layout
 */
TEST_F(BuilderTestPager, LazyInitialSetPagerRTL)
{
    loadDocument(LAZY_INITIAL_SET_PAGER);

    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypePager, component->getType());

    ASSERT_EQ(2, component->getCalculated(kPropertyCurrentPage).getInteger());
    ASSERT_EQ(kNavigationNormal, component->getCalculated(kPropertyNavigation).getInteger());

    ASSERT_TRUE(IsEqual(Rect(0, 0, 100, 200), component->getCalculated(kPropertyBounds)));

    // Children
    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildLaidOut(component, 0, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {1, 3}, true));
    ASSERT_TRUE(CheckChildLaidOut(component, 4, false));
    ASSERT_TRUE(CheckChild(0, "0", Rect(0, 0, 0, 0)));
    ASSERT_TRUE(CheckChild(1, "1", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(2, "2", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(3, "3", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(4, "4", Rect(0, 0, 0, 0)));

    component->update(kUpdatePagerByEvent, 1);
    root->clearPending();
    ASSERT_EQ(1, component->getCalculated(kPropertyCurrentPage).getInteger());

    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyLayoutDirection, kPropertyVisualHash));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 3}, true));
    ASSERT_TRUE(CheckChildLaidOut(component, 4, false));
    ASSERT_TRUE(CheckChild(0, "0", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(1, "1", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(2, "2", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(3, "3", Rect(0, 0, 100, 200)));
    ASSERT_TRUE(CheckChild(4, "4", Rect(0, 0, 0, 0)));
}

static const char *NORMAL_HORIZONTAL_PAGER = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "width": 100,
      "height": 200,
      "data": [0, 1],
      "navigation": "normal",
      "items": [{ "type": "Text", "id": "${data}" }]
    }
  }
})";

TEST_F(BuilderTestPager, NormalHorizontalPager)
{
    loadDocument(NORMAL_HORIZONTAL_PAGER);
    ASSERT_EQ(ScrollType::kScrollTypeHorizontalPager, component->scrollType());
    ASSERT_EQ(PageDirection::kPageDirectionForward, component->pageDirection());
    ASSERT_TRUE(component->allowForward());
    ASSERT_FALSE(component->allowBackwards());
}

static const char *NORMAL_VERTICAL_PAGER = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "width": 100,
      "height": 200,
      "data": [0, 1],
      "navigation": "normal",
      "pageDirection": "vertical",
      "items": [{ "type": "Text", "id": "${data}" }]
    }
  }
})";

TEST_F(BuilderTestPager, NormalVerticalPager)
{
    loadDocument(NORMAL_VERTICAL_PAGER);
    ASSERT_EQ(ScrollType::kScrollTypeVerticalPager, component->scrollType());
    ASSERT_EQ(PageDirection::kPageDirectionForward, component->pageDirection());
    ASSERT_TRUE(component->allowForward());
    ASSERT_FALSE(component->allowBackwards());
}

static const char *WRAPPED_HORIZONTAL_PAGER = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "width": 100,
      "height": 200,
      "data": [0, 1],
      "navigation": "wrap",
      "items": [{ "type": "Text", "id": "${data}" }]
    }
  }
})";

TEST_F(BuilderTestPager, WrappedHorizontalPager)
{
    loadDocument(WRAPPED_HORIZONTAL_PAGER);
    ASSERT_EQ(ScrollType::kScrollTypeHorizontalPager, component->scrollType());
    ASSERT_EQ(PageDirection::kPageDirectionBoth, component->pageDirection());
    ASSERT_TRUE(component->allowForward());
    ASSERT_TRUE(component->allowBackwards());
}

static const char *NORMAL_HORIZONTAL_END_PAGER = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "width": 100,
      "height": 200,
      "data": [0, 1],
      "navigation": "normal",
      "initialPage": 1,
      "items": [{ "type": "Text", "id": "${data}" }]
    }
  }
})";

TEST_F(BuilderTestPager, NormalHorizontalEndPager)
{
    loadDocument(NORMAL_HORIZONTAL_END_PAGER);
    ASSERT_EQ(ScrollType::kScrollTypeHorizontalPager, component->scrollType());
    ASSERT_EQ(PageDirection::kPageDirectionBack, component->pageDirection());
    ASSERT_FALSE(component->allowForward());
    ASSERT_TRUE(component->allowBackwards());
}

TEST_F(BuilderTestPager, DynamicChanges)
{
    loadDocument(NORMAL_VERTICAL_PAGER);
    ASSERT_EQ(ScrollType::kScrollTypeVerticalPager, component->scrollType());
    ASSERT_EQ(PageDirection::kPageDirectionForward, component->pageDirection());
    ASSERT_TRUE(component->allowForward());
    ASSERT_FALSE(component->allowBackwards());

    component->setProperty(kPropertyNavigation, "wrap");
    component->setProperty(kPropertyPageDirection, "horizontal");

    ASSERT_EQ(ScrollType::kScrollTypeHorizontalPager, component->scrollType());
    ASSERT_EQ(PageDirection::kPageDirectionBoth, component->pageDirection());
    ASSERT_TRUE(component->allowForward());
    ASSERT_TRUE(component->allowBackwards());
}

static const char *NORMAL_BIGGER_HORIZONTAL_PAGER = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "width": 100,
      "height": 200,
      "data": [0, 1, 2, 3],
      "navigation": "normal",
      "items": [
        {
          "type": "Container",
          "items": {
            "type": "Text", "id": "${data}"
          }
        }
      ]
    }
  }
})";

TEST_F(BuilderTestPager, LazierPager)
{
    loadDocument(NORMAL_BIGGER_HORIZONTAL_PAGER);
    ASSERT_TRUE(component->getCoreChildAt(0)->getChildCount() > 0);
    ASSERT_TRUE(component->getCoreChildAt(1)->getChildCount() == 0);
    ASSERT_TRUE(component->getCoreChildAt(2)->getChildCount() == 0);
    ASSERT_TRUE(component->getCoreChildAt(3)->getChildCount() == 0);

    advanceTime(10);
    ASSERT_TRUE(component->getCoreChildAt(0)->getChildCount() > 0);
    ASSERT_TRUE(component->getCoreChildAt(1)->getChildCount() > 0);
    ASSERT_TRUE(component->getCoreChildAt(2)->getChildCount() == 0);
    ASSERT_TRUE(component->getCoreChildAt(3)->getChildCount() == 0);

    component->update(kUpdatePagerPosition, 2);
    root->clearPending();
    ASSERT_TRUE(component->getCoreChildAt(0)->getChildCount() > 0);
    ASSERT_TRUE(component->getCoreChildAt(1)->getChildCount() > 0);
    ASSERT_TRUE(component->getCoreChildAt(2)->getChildCount() > 0);
    ASSERT_TRUE(component->getCoreChildAt(3)->getChildCount() > 0);
}