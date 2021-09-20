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

/**
 * Pager test cases covering
 *
 * (a) The number of pages laid out based on the cache size and the navigation direction
 * (b) What happens when a pager is resized
 */

class PagerTest : public DocumentWrapper {};

static const char *PAGE_CACHE_BY_NAVIGATION = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": {
            "type": "Pager",
            "id": "pager-${data}",
            "navigation": "${data}",
            "width": 100,
            "height": 100,
            "grow": 1,
            "items": {
              "type": "Text",
              "width": "100%",
              "height": "100%"
            },
            "data": "${Array.range(20)}"
          },
          "data": [
            "normal",
            "none",
            "wrap",
            "forward-only"
          ]
        }
      }
    }
)apl";

TEST_F(PagerTest, PageCacheByNavigation)
{
    config->pagerChildCache(2);  // Two pages around starting place
    loadDocument(PAGE_CACHE_BY_NAVIGATION);
    ASSERT_TRUE(component);
    ASSERT_EQ(4, component->getChildCount());
    advanceTime(10);

    // Navigation: "normal".  No wrapping, forward/backwards allowed
    auto pager = root->findComponentById("pager-normal");
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 2}));
    pager->update(kUpdatePagerByEvent, 10);    // Jump to middle
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 2, 8, 9, 10, 11, 12}));
    pager->update(kUpdatePagerByEvent, 19);    // Jump to end
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 2, 8, 9, 10, 11, 12, 17, 18, 19}));


    // Navigation: "none".  Only the currently loaded page is laid out.  The caching algorithm is the same as "normal"
    pager = root->findComponentById("pager-none");
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 2}));
    pager->update(kUpdatePagerByEvent, 10);    // Jump to middle
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 2, 8, 9, 10, 11, 12}));
    pager->update(kUpdatePagerByEvent, 19);    // Jump to end
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 2, 8, 9, 10, 11, 12, 17, 18, 19}));

    // Navigation: "wrap".  Forward/backwards allowed with wrapping.
    pager = root->findComponentById("pager-wrap");
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 2, 18, 19}));
    pager->update(kUpdatePagerByEvent, 10);    // Jump to middle
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 2, 8, 9, 10, 11, 12, 18, 19}));
    pager->update(kUpdatePagerByEvent, 19);    // Jump to end
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 2, 8, 9, 10, 11, 12, 17, 18, 19}));

    // Navigation: "forward-only".  No wrapping supported.
    pager = root->findComponentById("pager-forward-only");
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 2}));
    pager->update(kUpdatePagerByEvent, 10);    // Jump to middle
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 2, 10, 11, 12}));
    pager->update(kUpdatePagerByEvent, 19);    // Jump to end
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 2, 10, 11, 12, 19}));
}


static const char *VARIABLE_SIZE = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 600,
          "height": 600,
          "items": {
            "type": "Pager",
            "id": "PAGER-${index}",
            "width": 100,
            "height": 100,
            "grow": 1,
            "items": {
              "type": "Text",
              "width": "100%",
              "height": "100%"
            },
            "data": "${Array.range(4)}"
          },
          "data": "${Array.range(3)}"
        }
      }
    }
)apl";

TEST_F(PagerTest, VariableSize)
{
    loadDocument(VARIABLE_SIZE);
    ASSERT_TRUE(component);
    ASSERT_EQ(3, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1, 2}));  // All Pagers should be laid out
    advanceTime(10);

    auto pager = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Rect(0, 0, 100, 200), pager->getCalculated(kPropertyBounds)));
    ASSERT_EQ(4, pager->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(pager, {0, 1, 3}));  // All but page #2 should be laid out

    auto text = pager->getChildAt(0);  // Stash a text box for later reference
    ASSERT_TRUE(IsEqual(Rect(0, 0, 100, 200), text->getCalculated(kPropertyBounds)));

    auto pager2 = component->getChildAt(2);  // Stash for later
    auto text2 = pager2->getChildAt(0);

    // Setting display=none on one pager will case the Container to re-layout and the remaining Pagers to grow.
    executeCommand("SetValue", {{"componentId", "PAGER-2"}, {"property", "display"}, {"value", "none"}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0,0,100,300), pager->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,300), text->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(CheckDirty(pager, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(text, kPropertyBounds, kPropertyInnerBounds, kPropertyVisualHash));

    // The removed pager has zero size.  Its children have NOT been laid out again
    ASSERT_TRUE(IsEqual(Rect(0,0,0,0), pager2->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,200), text2->getCalculated(kPropertyBounds)));

    // Remove the middle pager
    executeCommand("SetValue", {{"componentId", "PAGER-1"}, {"property", "display"}, {"value", "none"}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0,0,100,600), pager->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,600), text->getCalculated(kPropertyBounds)));

    // Show the final pager
    executeCommand("SetValue", {{"componentId", "PAGER-2"}, {"property", "display"}, {"value", "visible"}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0,0,100,300), pager->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,300), text->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,300,100,300), pager2->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,300), text2->getCalculated(kPropertyBounds)));
}
