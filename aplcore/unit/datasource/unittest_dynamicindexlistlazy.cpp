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

#include "apl/component/pagercomponent.h"
#include "./dynamicindexlisttest.h"

#include "apl/dynamicdata.h"

using namespace apl;

class DynamicIndexListLazyTest : public DynamicIndexListTest {};

TEST_F(DynamicIndexListLazyTest, Configuration) {
    // Backward compatibility
    auto source = std::make_shared<DynamicIndexListDataSourceProvider>("magic", 42);
    auto actualConfiguration = source->getConfiguration();
    ASSERT_EQ("magic", actualConfiguration.type);
    ASSERT_EQ(42, actualConfiguration.cacheChunkSize);
    ASSERT_EQ(5, actualConfiguration.listUpdateBufferSize);
    ASSERT_EQ(2, actualConfiguration.fetchRetries);
    ASSERT_EQ(5000, actualConfiguration.fetchTimeout);
    ASSERT_EQ(5000, actualConfiguration.cacheExpiryTimeout);

    // Full config
    auto expectedConfiguration = DynamicIndexListConfiguration()
            .setType("magic")
            .setCacheChunkSize(42)
            .setListUpdateBufferSize(7)
            .setFetchRetries(3)
            .setFetchTimeout(2000)
            .setCacheExpiryTimeout(10000);
    source = std::make_shared<DynamicIndexListDataSourceProvider>(expectedConfiguration);
    actualConfiguration = source->getConfiguration();
    ASSERT_EQ(expectedConfiguration.type, actualConfiguration.type);
    ASSERT_EQ(expectedConfiguration.cacheChunkSize, actualConfiguration.cacheChunkSize);
    ASSERT_EQ(expectedConfiguration.listUpdateBufferSize, actualConfiguration.listUpdateBufferSize);
    ASSERT_EQ(expectedConfiguration.fetchRetries, actualConfiguration.fetchRetries);
    ASSERT_EQ(expectedConfiguration.fetchTimeout, actualConfiguration.fetchTimeout);
    ASSERT_EQ(expectedConfiguration.cacheExpiryTimeout, actualConfiguration.cacheExpiryTimeout);

    // Default
    source = std::make_shared<DynamicIndexListDataSourceProvider>();
    actualConfiguration = source->getConfiguration();
    ASSERT_EQ(SOURCE_TYPE, actualConfiguration.type);
    ASSERT_EQ(10, actualConfiguration.cacheChunkSize);
    ASSERT_EQ(5, actualConfiguration.listUpdateBufferSize);
    ASSERT_EQ(2, actualConfiguration.fetchRetries);
    ASSERT_EQ(5000, actualConfiguration.fetchTimeout);
    ASSERT_EQ(5000, actualConfiguration.cacheExpiryTimeout);
}

static const char *DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *SMALLER_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 10,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *BASIC = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "height": 300,
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  }
})";

TEST_F(DynamicIndexListLazyTest, Basic)
{
    loadDocument(BASIC, DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 11), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(12, 14), false));

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(9)->getId());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ(400, component->getCalculated(kPropertyScrollPosition).asNumber());
    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id19", component->getChildAt(19)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 5), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(6, 16), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(17, 19), false));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListLazyTest, BasicAsMap)
{
    loadDocument(BASIC, DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(StringToMapObject(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19"))));
    ASSERT_TRUE(ds->processUpdate(StringToMapObject(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9"))));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(ds->processUpdate(StringToMapObject(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4"))));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(400, component->getCalculated(kPropertyScrollPosition).asNumber());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *BASIC_HORIZONTAL_RTL = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "width": 300,
      "scrollDirection": "horizontal",
      "layoutDirection": "RTL",
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  }
})";

TEST_F(DynamicIndexListLazyTest, BasicRTL)
{
    loadDocument(BASIC_HORIZONTAL_RTL, DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 11), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(12, 14), false));

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(9)->getId());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ(-400, component->getCalculated(kPropertyScrollPosition).asNumber());
    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id19", component->getChildAt(19)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 5), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(6, 16), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(17, 19), false));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListLazyTest, BasicAsMapRTL)
{
    loadDocument(BASIC_HORIZONTAL_RTL, DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(StringToMapObject(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19"))));
    ASSERT_TRUE(ds->processUpdate(StringToMapObject(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9"))));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(ds->processUpdate(StringToMapObject(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4"))));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(-400, component->getCalculated(kPropertyScrollPosition).asNumber());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *EMPTY = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "minimumInclusiveIndex": -5,
    "maximumExclusiveIndex": 5,
    "startIndex": 0
  }
})";

TEST_F(DynamicIndexListLazyTest, Empty)
{
    loadDocument(BASIC, EMPTY);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 0, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 0, "0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 4), true));

    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id4", component->getChildAt(4)->getId());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, -5, "-5, -4, -3, -2, -1")));

    root->clearPending();

    ASSERT_EQ(10, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 9), true));

    ASSERT_EQ("id-5", component->getChildAt(0)->getId());
    ASSERT_EQ("id4", component->getChildAt(9)->getId());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIRST_AND_LAST = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "height": 300,
      "data": "${dynamicSource}",
      "firstItem": {
        "type": "Text",
        "id": "fi",
        "width": 100,
        "height": 100,
        "text": "FI"
      },
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      },
      "lastItem": {
        "type": "Text",
        "id": "li",
        "width": 100,
        "height": 100,
        "text": "LI"
      }
    }
  }
})";

static const char *FIRST_AND_LAST_DATA =
R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [ 10 ]
  }
})";

TEST_F(DynamicIndexListLazyTest, WithFirstAndLast)
{
    loadDocument(FIRST_AND_LAST, FIRST_AND_LAST_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 2}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 11, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 11, "11, 12, 13, 14, 15")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    // Whole range is laid out as we don't allow gaps
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 12), true));

    ASSERT_EQ(13, component->getChildCount());

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(1)->getId());
    ASSERT_EQ("id15", component->getChildAt(11)->getId());
    ASSERT_EQ("li", component->getChildAt(12)->getId());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

    component->update(kUpdateScrollPosition, 600);
    advanceTime(10);
    root->clearPending();


    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", 16, 4));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 104, 16, "16, 17, 18, 19")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(1100, component->getCalculated(kPropertyScrollPosition).asNumber());

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id0", component->getChildAt(1)->getId());
    ASSERT_EQ("id19", component->getChildAt(20)->getId());
    ASSERT_EQ("li", component->getChildAt(21)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 21), true));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIRST_AND_LAST_HORIZONTAL_RTL = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "scrollDirection": "horizontal",
      "layoutDirection": "RTL",
      "width": 300,
      "data": "${dynamicSource}",
      "firstItem": {
        "type": "Text",
        "id": "fi",
        "width": 100,
        "height": 100,
        "text": "FI"
      },
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      },
      "lastItem": {
        "type": "Text",
        "id": "li",
        "width": 100,
        "height": 100,
        "text": "LI"
      }
    }
  }
})";

TEST_F(DynamicIndexListLazyTest, WithFirstAndLastHorizontalRTL)
{
    loadDocument(FIRST_AND_LAST_HORIZONTAL_RTL, FIRST_AND_LAST_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(3, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 2}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 11, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 11, "11, 12, 13, 14, 15")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    // Whole range is laid out as we don't allow gaps
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 12), true));

    ASSERT_EQ(13, component->getChildCount());

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(1)->getId());
    ASSERT_EQ("id15", component->getChildAt(11)->getId());
    ASSERT_EQ("li", component->getChildAt(12)->getId());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

    component->update(kUpdateScrollPosition, -600);
    advanceTime(10);
    root->clearPending();


    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", 16, 4));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 104, 16, "16, 17, 18, 19")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(-1100, component->getCalculated(kPropertyScrollPosition).asNumber());

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id0", component->getChildAt(1)->getId());
    ASSERT_EQ("id19", component->getChildAt(20)->getId());
    ASSERT_EQ("li", component->getChildAt(21)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 21), true));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIRST = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "height": 300,
      "data": "${dynamicSource}",
      "firstItem": {
        "type": "Text",
        "id": "fi",
        "width": 100,
        "height": 100,
        "text": "FI"
      },
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  }
})";

TEST_F(DynamicIndexListLazyTest, WithFirst)
{
    loadDocument(FIRST, FIRST_AND_LAST_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 11, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 11, "11, 12, 13, 14, 15")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    // Whole range is laid out as we don't allow gaps
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 6), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(7, 11), false));

    ASSERT_EQ(12, component->getChildCount());

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(1)->getId());
    ASSERT_EQ("id15", component->getChildAt(11)->getId());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

    component->update(kUpdateScrollPosition, 600);
    advanceTime(10);
    root->clearPending();

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", 16, 4));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 0, "0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ("fi", component->getChildAt(0)->getId());
    ASSERT_EQ("id0", component->getChildAt(1)->getId());
    ASSERT_EQ("id15", component->getChildAt(16)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 16), true));

    ASSERT_FALSE(root->hasEvent());
}

static const char *LAST = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "height": 300,
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      },
      "lastItem": {
        "type": "Text",
        "id": "li",
        "width": 100,
        "height": 100,
        "text": "LI"
      }
    }
  }
})";

TEST_F(DynamicIndexListLazyTest, WithLast)
{
    loadDocument(LAST, FIRST_AND_LAST_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 11, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 11, "11, 12, 13, 14, 15")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    // Whole range is laid out as we don't allow gaps
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 11), true));

    ASSERT_EQ(12, component->getChildCount());

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id15", component->getChildAt(10)->getId());
    ASSERT_EQ("li", component->getChildAt(11)->getId());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(400, component->getCalculated(kPropertyScrollPosition).asNumber());

    component->update(kUpdateScrollPosition, 600);
    advanceTime(10);
    root->clearPending();

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 16, 4));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", 0, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 16, "16, 17, 18, 19")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id15", component->getChildAt(10)->getId());
    ASSERT_EQ("li", component->getChildAt(15)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 0), false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(1, 15), true));

    ASSERT_FALSE(root->hasEvent());
}

static const char *LAST_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [ 0 ]
  }
})";

TEST_F(DynamicIndexListLazyTest, WithLastOneWay)
{
    loadDocument(LAST, LAST_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 1, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 1, "1, 2, 3, 4, 5")));
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 6), true));

    ASSERT_EQ(7, component->getChildCount());

    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(5)->getId());
    ASSERT_EQ("li", component->getChildAt(6)->getId());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 6, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 6, "6, 7, 8, 9, 10")));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 11), true));
    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(5)->getId());
    ASSERT_EQ("id10", component->getChildAt(10)->getId());
    ASSERT_EQ("li", component->getChildAt(11)->getId());

    ASSERT_FALSE(root->hasEvent());


    ASSERT_EQ(0, component->getCalculated(kPropertyScrollPosition).asNumber());
    component->update(kUpdateScrollPosition, 600);
    advanceTime(10);
    root->clearPending();

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 11, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, 11, "11, 12, 13, 14, 15")));
    root->clearPending();
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", 16, 4));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ("id0", component->getChildAt(0)->getId());
    ASSERT_EQ("id5", component->getChildAt(5)->getId());
    ASSERT_EQ("id10", component->getChildAt(10)->getId());
    ASSERT_EQ("id15", component->getChildAt(15)->getId());
    ASSERT_EQ("li", component->getChildAt(16)->getId());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 16), true));

    ASSERT_FALSE(root->hasEvent());
}

static const char *SHRINKABLE_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 10,
    "maximumExclusiveIndex": 15,
    "items": [ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 ]
  }
})";

TEST_F(DynamicIndexListLazyTest, ShrinkData)
{
    loadDocument(BASIC, SHRINKABLE_DATA);
    advanceTime(10);
    ASSERT_TRUE(CheckBounds(10, 15));
    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
}

static const char *EMPTY_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": []
  }
})";

TEST_F(DynamicIndexListLazyTest, EmptySequence)
{
    loadDocument(BASIC, EMPTY_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 10, "10, 11, 12, 13, 14")));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 5, 5));
}

static const char *MULTI = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource1", "dynamicSource2"
    ],
    "item": {
      "type": "Container",
      "id": "container",
      "items": [
        {
          "type": "Sequence",
          "id": "sequence",
          "height": 300,
          "data": "${dynamicSource1}",
          "items": {
            "type": "Text",
            "id": "id${data}",
            "width": 100,
            "height": 100,
            "text": "${data}"
          }
        },
        {
          "type": "Sequence",
          "id": "sequence",
          "height": 300,
          "data": "${dynamicSource2}",
          "items": {
            "type": "Text",
            "id": "id${data}",
            "width": 100,
            "height": 100,
            "text": "${data}"
          }
        }
      ]
    }
  }
})";

static const char *MULTI_DATA = R"({
  "dynamicSource1": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok1",
    "startIndex": 10,
    "minimumInclusiveIndex": 10,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  },
  "dynamicSource2": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok2",
    "startIndex": 10,
    "minimumInclusiveIndex": 5,
    "maximumExclusiveIndex": 15,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

TEST_F(DynamicIndexListLazyTest, Multi) {
    loadDocument(MULTI, MULTI_DATA);
    advanceTime(10);

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok1", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok2", "102", 5, 5));
}

static const char *WRONG_NIN_INDEX_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 15,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *WRONG_MISSING_FIELDS_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "minimumInclusiveIndex": 15,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *WRONG_MAX_INDEX_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 15,
    "maximumExclusiveIndex": 15,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *MULTI_CLONED_DATA = R"({
  "dynamicSource1": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  },
  "dynamicSource2": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

TEST_F(DynamicIndexListLazyTest, WrongMissingFieldsData) {
    loadDocument(BASIC, WRONG_MISSING_FIELDS_DATA);
    ASSERT_TRUE(session->checkAndClear());
    ASSERT_TRUE(CheckErrors({"INTERNAL_ERROR"}));
    ASSERT_EQ(component->getChildCount(), 1);
}

TEST_F(DynamicIndexListLazyTest, WrongNINIndexData) {

    loadDocument(BASIC, WRONG_NIN_INDEX_DATA);
    ASSERT_TRUE(session->checkAndClear());
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_EQ(component->getChildCount(), 1);
}

TEST_F(DynamicIndexListLazyTest,WrongMaxIndexData) {
    loadDocument(BASIC, WRONG_MAX_INDEX_DATA);
    ASSERT_TRUE(session->checkAndClear());
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_EQ(component->getChildCount(), 1);
}

TEST_F(DynamicIndexListLazyTest,MultiCloneData) {
    loadDocument(MULTI, MULTI_CLONED_DATA);
    ASSERT_TRUE(session->checkAndClear());
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_EQ(component->getChildCount(), 2);
}

TEST_F(DynamicIndexListLazyTest, DuplicateListVersionErrorForRemovedComponent)
{
    loadDocument(BASIC, DATA);
    advanceTime(10);

    ASSERT_TRUE(ds->processUpdate(createLazyLoad(1, 101, 15, "15, 16, 17, 18, 19")));

    component = nullptr;
    rootDocument = nullptr;
    root = nullptr;
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(1, 101, 15, "15, 16, 17, 18, 19")));
}

TEST_F(DynamicIndexListLazyTest, MissingListVersionErrorForRemovedComponent)
{
    loadDocument(BASIC, DATA);
    advanceTime(10);

    ASSERT_TRUE(ds->processUpdate(createLazyLoad(1, 101, 15, "15, 16, 17, 18, 19")));

    component = nullptr;
    rootDocument = nullptr;
    root = nullptr;
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));
}

TEST_F(DynamicIndexListLazyTest, ConnectionInFailedStateForRemovedComponent)
{
    loadDocument(BASIC, DATA);
    advanceTime(10);

    ASSERT_TRUE(ds->processUpdate(createLazyLoad(1, 101, 15, "15, 16, 17, 18, 19")));
    // put connection into failed state with invalid update
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));

    component = nullptr;
    root = nullptr;
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(1, 101, 15, "15, 16, 17, 18, 19")));
    ASSERT_FALSE(ds->getPendingErrors().empty());
}

TEST_F(DynamicIndexListLazyTest, InvalidUpdatePayloadForRemovedComponent)
{
    loadDocument(BASIC, DATA);
    advanceTime(10);

    ASSERT_TRUE(ds->processUpdate(createLazyLoad(1, 101, 15, "15, 16, 17, 18, 19")));

    component = nullptr;
    rootDocument = nullptr;
    root = nullptr;
    auto invalidPayload =  "{\"presentationToken\": \"presentationToken\", \"listId\": \"vQdpOESlok\"}";
    ASSERT_FALSE(ds->processUpdate(invalidPayload));
}

static const char *BASIC_CONTAINER = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Container",
      "id": "container",
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  }
})";

TEST_F(DynamicIndexListLazyTest, Container)
{
    loadDocument(BASIC_CONTAINER, DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, -1, 5, "5, 6, 7, 8, 9")));
    root->clearPending();

    ASSERT_EQ(10, component->getChildCount());

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(9)->getId());

    root->clearDirty();

    ASSERT_FALSE(root->isDirty());

    ASSERT_EQ("id5", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(9)->getId());
}

static const char *WRONG_CORRELATION_TOKEN = R"({
  "token": "presentationToken",
  "correlationToken": "76",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "items": [ 5, 6, 7, 8, 9 ]
})";

static const char *TEN_TO_FOURTEEN_RANGE = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "minimumInclusiveIndex": 10,
  "maximumExclusiveIndex": 15
})";

static const char *INCOMPLETE_FOLLOWUP = R"({
  "token": "presentationToken",
  "startIndex": 5,
  "items": [ 5, 6, 7, 8, 9 ]
})";

static const char *UNCORRELATED_FOLLOWUP = R"({
  "token": "presentationToken",
  "correlationToken": "42",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "items": [ 5, 6, 7, 8, 9 ]
})";

static const char *WRONG_LIST_FOLLOWUP = R"({
  "token": "presentationToken",
  "listId": "DEADBEEF",
  "startIndex": 5,
  "items": [ 5, 6, 7, 8, 9 ]
})";

static const char *NOT_ARRAY_ITEMS_FOLLOWUP = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "items": { "abr": 1 }
})";

TEST_F(DynamicIndexListLazyTest, WrongUpdates)
{
    loadDocument(BASIC, DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));

    ASSERT_EQ("id10", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(4)->getId());

    ASSERT_FALSE(ds->processUpdate(7)); // Should do nothing, type is wrong.
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_FALSE(ds->processUpdate(INCOMPLETE_FOLLOWUP)); // Should do nothing, missing fields.
    ASSERT_TRUE(CheckErrors({ "INVALID_LIST_ID" }));
    ASSERT_FALSE(ds->processUpdate(UNCORRELATED_FOLLOWUP)); // Should do nothing, wrong correlation token.
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_FALSE(ds->processUpdate(WRONG_LIST_FOLLOWUP)); // Should do nothing, wrong list.
    ASSERT_TRUE(CheckErrors({ "INVALID_LIST_ID" }));
    ASSERT_FALSE(ds->processUpdate(NOT_ARRAY_ITEMS_FOLLOWUP)); // Should do nothing, not an items array.
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_FALSE(ds->processUpdate(WRONG_CORRELATION_TOKEN));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    root->clearPending();

    ASSERT_FALSE(root->isDirty());

    // Adjust boundaries and try to update around it.
    ASSERT_TRUE(ds->processUpdate(TEN_TO_FOURTEEN_RANGE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE", "MISSING_LIST_ITEMS" }));
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(-1, -1, 5, "5, 6, 7, 8, 9")));
    ASSERT_TRUE(CheckErrors({ "LOAD_INDEX_OUT_OF_RANGE" }));
}

static const char *DATA_PARTIAL_OOR = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 10,
    "maximumExclusiveIndex": 15,
    "items": []
  }
})";

TEST_F(DynamicIndexListLazyTest, PartialOutOfRange)
{
    loadDocument(BASIC, DATA_PARTIAL_OOR);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(CheckBounds(10, 15));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 9, "9, 10, 11, 12, 13, 14, 15")));
    ASSERT_TRUE(CheckErrors({ "LOAD_INDEX_OUT_OF_RANGE" }));

    root->clearPending();
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_EQ("id10", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(4)->getId());
}


static const char *UNKNOWN_BOUNDS_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": -10,
    "items": [ -10, -9, -8, -7, -6 ]
  }
})";

static const char *RESPONSE_AND_BOUND_UNKNOWN_DOWN = R"({
  "token": "presentationToken",
  "correlationToken": "103",
  "listId": "vQdpOESlok",
  "startIndex": -20,
  "minimumInclusiveIndex": -20,
  "maximumExclusiveIndex": 5,
  "items": [ -20, -19, -18, -17, -16 ]
})";

TEST_F(DynamicIndexListLazyTest, UnknownBounds)
{
    loadDocument(BASIC, UNKNOWN_BOUNDS_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(INT_MIN, INT_MAX));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", -5, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, -1, -15, "-15, -14, -13, -12, -11")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, -1, -5, "-5, -4, -3, -2, -1")));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_EQ("id-15", component->getChildAt(0)->getId());
    ASSERT_EQ("id-1", component->getChildAt(14)->getId());

    ASSERT_TRUE(ds->processUpdate(RESPONSE_AND_BOUND_UNKNOWN_DOWN));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", -20, 5));

    // Scroll down to get it fetching again
    ASSERT_EQ(400, component->getCalculated(kPropertyScrollPosition).asNumber());
    component->update(kUpdateScrollPosition, 550); // + 5 children down
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "105", 0, 5));
    ASSERT_TRUE(CheckBounds(-20, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 104, 0, "0, 1, 2, 3, 4")));
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(component));
    ASSERT_EQ(1, component->getDirty().count(kPropertyNotifyChildrenChanged));

    ASSERT_EQ(25, component->getChildCount());

    ASSERT_EQ("id-20", component->getChildAt(0)->getId());
    ASSERT_EQ("id4", component->getChildAt(24)->getId());
}

static const char *SIMPLE_UPDATE = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": -17,
  "items": [ "-17U", "-16U", "-15U", "-14U", "-13U" ]
})";

TEST_F(DynamicIndexListLazyTest, SimpleUpdate)
{
    loadDocument(BASIC, UNKNOWN_BOUNDS_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(INT_MIN, INT_MAX));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", -5, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, -1, -15, "-15, -14, -13, -12, -11")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, -1, -5, "-5, -4, -3, -2, -1")));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());

    ASSERT_EQ("-15", component->getChildAt(0)->getCalculated(kPropertyText).asString());
    ASSERT_EQ("-11", component->getChildAt(4)->getCalculated(kPropertyText).asString());
    ASSERT_EQ("-1", component->getChildAt(14)->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", -20, 5));

    ASSERT_TRUE(ds->processUpdate(SIMPLE_UPDATE));
    ASSERT_TRUE(CheckErrors({ "OCCUPIED_LIST_INDEX" }));
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    ASSERT_EQ(17, component->getChildCount());

    ASSERT_EQ("-17U", component->getChildAt(0)->getCalculated(kPropertyText).asString());
    ASSERT_EQ("-16U", component->getChildAt(1)->getCalculated(kPropertyText).asString());
    ASSERT_EQ("-15", component->getChildAt(2)->getCalculated(kPropertyText).asString());
}

static const char *POSITIVE_BOUNDS_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 7,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

static const char *RESPONSE_AND_BOUND_EXTEND = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 7,
  "minimumInclusiveIndex": 7,
  "maximumExclusiveIndex": 15,
  "items": [ 7, 8, 9 ]
})";

TEST_F(DynamicIndexListLazyTest, PositiveBounds)
{
    loadDocument(BASIC, POSITIVE_BOUNDS_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(7, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 7, 3));

    ASSERT_TRUE(ds->processUpdate(RESPONSE_AND_BOUND_EXTEND));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    ASSERT_TRUE(CheckBounds(7, 15));
    root->clearPending();

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(component));
    ASSERT_EQ(1, component->getDirty().count(kPropertyNotifyChildrenChanged));

    ASSERT_EQ(8, component->getChildCount());

    ASSERT_EQ("id7", component->getChildAt(0)->getId());
    ASSERT_EQ("id14", component->getChildAt(7)->getId());
}


static const char *BASIC_PAGER = R"({
  "type": "APL",
  "version": "1.2",
  "theme": "light",
  "layouts": {
    "square": {
      "parameters": ["color", "text"],
      "item": {
        "type": "Frame",
        "width": 200,
        "height": 200,
        "id": "frame-${text}",
        "backgroundColor": "${color}",
        "item": {
          "type": "Text",
          "text": "${text}",
          "color": "black",
          "width": 200,
          "height": 200
        }
      }
    }
  },
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Pager",
      "id": "pager",
      "data": "${dynamicSource}",
      "width": "100%",
      "height": "100%",
      "navigation": "normal",
      "items": {
        "type": "square",
        "index": "${index}",
        "color": "${data.color}",
        "text": "${data.text}"
      }
    }
  }
})";

static const char *BASIC_PAGER_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [
      { "color": "blue", "text": "10" },
      { "color": "red", "text": "11" },
      { "color": "green", "text": "12" },
      { "color": "yellow", "text": "13" },
      { "color": "white", "text": "14" }
    ]
  }
})";

static const char *FIVE_TO_NINE_FOLLOWUP_PAGER = R"({
 "token": "presentationToken",
 "listId": "vQdpOESlok",
 "startIndex": 5,
 "items": [
   { "color": "blue", "text": "5" },
   { "color": "red", "text": "6" },
   { "color": "green", "text": "7" },
   { "color": "yellow", "text": "8" },
   { "color": "white", "text": "9" }
 ]
})";

TEST_F(DynamicIndexListLazyTest, BasicPager)
{
    loadDocument(BASIC_PAGER, BASIC_PAGER_DATA);

    ASSERT_EQ(kComponentTypePager, component->getType());
    advanceTime(10);
    root->clearDirty();

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(0, 20));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 4}, false));

    // Load 5 pages BEFORE the current set of pages
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 5, 5));
    ASSERT_TRUE(ds->processUpdate(FIVE_TO_NINE_FOLLOWUP_PAGER));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_EQ("frame-5", component->getChildAt(0)->getId());
    ASSERT_EQ("frame-14", component->getChildAt(9)->getId());
    ASSERT_TRUE(CheckChildLaidOutDirtyFlagsWithNotify(component, 4));  // Page 4 gets loaded because we're on page 5
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0,3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4,6}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7,9}, false));

    // Switch to the first page (index=0)
    component->update(UpdateType::kUpdatePagerByEvent, 0);
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlagsWithNotify(component, {0, 1}));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4, 6}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7, 9}, false));

    // Load 5 more pages BEFORE the current set of pages
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 102, 15,
                                                 R"({ "color": "blue", "text": "15" },
                                                    { "color": "red", "text": "16" },
                                                    { "color": "green", "text": "17" },
                                                    { "color": "yellow", "text": "18" },
                                                    { "color": "white", "text": "19" })" )));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 103, 0,
                                                 R"({ "color": "blue", "text": "0" },
                                                    { "color": "red", "text": "1" },
                                                    { "color": "green", "text": "2" },
                                                    { "color": "yellow", "text": "3" },
                                                    { "color": "white", "text": "4" })" )));
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4, 6}, true));  // Page 4 gets loaded because we're on page 5
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7, 8}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {9, 11}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {12, 14}, false));

    // Switch to the last page (index=14)
    component->update(UpdateType::kUpdatePagerByEvent, 14);
    root->clearPending();
    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlagsWithNotify(component, {13, 14}));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4, 6}, true));  // Page 4 gets loaded because we're on page 5
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7, 8}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {9, 11}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {12, 12}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {13, 15}, true));  // Page 15 gets loaded because we're on page 14
    ASSERT_TRUE(CheckChildrenLaidOut(component, {16, 19}, false));

    ASSERT_TRUE(root->isDirty());

    auto dirty = root->getDirty();
    ASSERT_EQ(1, dirty.count(component));
    ASSERT_EQ(1, component->getDirty().count(kPropertyNotifyChildrenChanged));

    ASSERT_EQ("frame-0", component->getChildAt(0)->getId());
    ASSERT_EQ("frame-19", component->getChildAt(19)->getId());
}

static const char *EMPTY_PAGER_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": []
  }
})";

static const char *TEN_TO_FIFTEEN_RESPONSE_PAGER = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 10,
  "items": [
    { "color": "blue", "text": "10" },
    { "color": "red", "text": "11" },
    { "color": "green", "text": "12" },
    { "color": "yellow", "text": "13" },
    { "color": "white", "text": "14" }
  ]
})";

TEST_F(DynamicIndexListLazyTest, EmptyPager)
{
    loadDocument(BASIC_PAGER, EMPTY_PAGER_DATA);

    ASSERT_EQ(kComponentTypePager, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 10, 5));
    ASSERT_TRUE(ds->processUpdate(TEN_TO_FIFTEEN_RESPONSE_PAGER));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 4}, false));

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
}

static const char *WRAPPING_PAGER = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "light",
  "layouts": {
    "square": {
      "parameters": ["color", "text"],
      "item": {
        "type": "Frame",
        "width": 200,
        "height": 200,
        "id": "frame-${text}",
        "backgroundColor": "${color}",
        "item": {
          "type": "Text",
          "text": "${text}",
          "color": "black",
          "width": "100%",
          "height": "100%"
        }
      }
    }
  },
  "mainTemplate": {
    "parameters": [ "dynamicSource" ],
    "item": {
      "type": "Pager",
      "id": "pager",
      "data": "${dynamicSource}",
      "width": "100%",
      "height": "100%",
      "navigation": "wrap",
      "items": {
        "type": "square",
        "index": "${index}",
        "color": "${data.color}",
        "text": "${data.text}"
      }
    }
  }
})";

static const char *WRAPPING_PAGER_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 5,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [
      { "color": "blue", "text": "5" },
      { "color": "red", "text": "6" },
      { "color": "green", "text": "7" },
      { "color": "yellow", "text": "8" },
      { "color": "white", "text": "9" }
    ]
  }
})";

TEST_F(DynamicIndexListLazyTest, WrappedPager)
{
    loadDocument(WRAPPING_PAGER, WRAPPING_PAGER_DATA);

    ASSERT_EQ(kComponentTypePager, component->getType());
    ASSERT_EQ(kNavigationWrap, static_cast<Navigation>(component->getCalculated(kPropertyNavigation).getInteger()));

    ASSERT_EQ(5, component->getChildCount());
    advanceTime(10);
    root->clearDirty();

    // Load 5 pages every direction the current set of pages
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 101, 0,
                                                 R"({ "color": "blue", "text": "0" },
                                                    { "color": "red", "text": "1" },
                                                    { "color": "green", "text": "2" },
                                                    { "color": "yellow", "text": "3" },
                                                    { "color": "white", "text": "4" })" )));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 102, 10,
                                                 R"({ "color": "blue", "text": "10" },
                                                    { "color": "red", "text": "11" },
                                                    { "color": "green", "text": "12" },
                                                    { "color": "yellow", "text": "13" },
                                                    { "color": "white", "text": "14" })" )));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());

    // Go back to 0
    component->update(UpdateType::kUpdatePagerByEvent, 0);
    root->clearPending();

    // We need to wrap to load from 15 to 20
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 103, 15,
                                                 R"({ "color": "blue", "text": "15" },
                                                    { "color": "red", "text": "16" },
                                                    { "color": "green", "text": "17" },
                                                    { "color": "yellow", "text": "18" },
                                                    { "color": "white", "text": "19" })" )));
    root->clearPending();

    ASSERT_EQ(20, component->getChildCount());
}

static const char *OLD_WRAPPING_PAGER = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "light",
  "layouts": {
    "square": {
      "parameters": ["color", "text"],
      "item": {
        "type": "Frame",
        "width": 200,
        "height": 200,
        "id": "frame-${text}",
        "backgroundColor": "${color}",
        "item": {
          "type": "Text",
          "text": "${text}",
          "color": "black",
          "width": "100%",
          "height": "100%"
        }
      }
    }
  },
  "mainTemplate": {
    "parameters": [ "dynamicSource" ],
    "item": {
      "type": "Pager",
      "id": "pager",
      "data": "${dynamicSource}",
      "width": "100%",
      "height": "100%",
      "navigation": "wrap",
      "items": {
        "type": "square",
        "index": "${index}",
        "color": "${data.color}",
        "text": "${data.text}"
      }
    }
  }
})";

TEST_F(DynamicIndexListLazyTest, OldWrappedPager)
{
    loadDocument(OLD_WRAPPING_PAGER, WRAPPING_PAGER_DATA);

    ASSERT_EQ(kComponentTypePager, component->getType());
    // Check the override
    ASSERT_EQ(kNavigationNormal, static_cast<Navigation>(component->getCalculated(kPropertyNavigation).getInteger()));

    ASSERT_EQ(5, component->getChildCount());
    advanceTime(10);
    root->clearDirty();

    // Load 5 pages every direction the current set of pages
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 0, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 102, 10,
                                                 R"({ "color": "blue", "text": "10" },
                                                    { "color": "red", "text": "11" },
                                                    { "color": "green", "text": "12" },
                                                    { "color": "yellow", "text": "13" },
                                                    { "color": "white", "text": "14" })" )));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(0, 101, 0,
                                                 R"({ "color": "blue", "text": "0" },
                                                    { "color": "red", "text": "1" },
                                                    { "color": "green", "text": "2" },
                                                    { "color": "yellow", "text": "3" },
                                                    { "color": "white", "text": "4" })" )));
    root->clearPending();

    ASSERT_EQ(15, component->getChildCount());
}

static const char *SMALLER_DATA_BACK = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 5,
    "maximumExclusiveIndex": 15,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

TEST_F(DynamicIndexListLazyTest, GarbageCollection) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);
    root->clearDirty();

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // Kill RootContext and re-inflate.
    component = nullptr;
    context = nullptr;
    rootDocument = nullptr;
    root = nullptr;

    loop = std::make_shared<TestTimeManager>();
    config->timeManager(loop);
    loadDocument(BASIC, SMALLER_DATA_BACK);
    advanceTime(20);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(5, 15));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIFTEEN_TO_NINETEEN_WRONG_LIST_AND_TOKEN_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "76",
  "listId": "vQdpOESlok1",
  "startIndex": 15,
  "items": [ 15, 16, 17, 18, 19 ]
})";

static const char *FIFTEEN_TO_NINETEEN_WRONG_LIST_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok1",
  "startIndex": 15,
  "items": [ 15, 16, 17, 18, 19 ]
})";

TEST_F(DynamicIndexListLazyTest, CorrelationTokenSubstitute) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);
    root->clearDirty();

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_FALSE(ds->processUpdate(FIFTEEN_TO_NINETEEN_WRONG_LIST_AND_TOKEN_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INVALID_LIST_ID" }));

    ASSERT_TRUE(ds->processUpdate(FIFTEEN_TO_NINETEEN_WRONG_LIST_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_LIST_ID" }));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIFTEEN_TO_TWENTY_FOUR_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 15,
  "items": [ 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 ]
})";

TEST_F(DynamicIndexListLazyTest, BigLazyLoad) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);
    root->clearDirty();

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(ds->processUpdate(FIFTEEN_TO_TWENTY_FOUR_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "LOAD_INDEX_OUT_OF_RANGE" }));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIFTEEN_TO_NINETEEN_SHRINK_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 15,
  "minimumInclusiveIndex": 12,
  "items": [ 15, 16, 17, 18, 19 ]
})";

TEST_F(DynamicIndexListLazyTest, BoundsShrinkBottom) {
    loadDocument(BASIC, SMALLER_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(ds->processUpdate(FIFTEEN_TO_NINETEEN_SHRINK_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE", "OCCUPIED_LIST_INDEX" }));
    root->clearPending();

    ASSERT_EQ(8, component->getChildCount());
    ASSERT_TRUE(CheckBounds(12, 20));
}

static const char *FIVE_TO_NINE_SHRINK_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "maximumExclusiveIndex": 13,
  "items": [ 5, 6, 7, 8, 9 ]
})";

TEST_F(DynamicIndexListLazyTest, BoundsShrinkTop) {
    loadDocument(BASIC, SMALLER_DATA_BACK);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(5, 15));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 5, 5));
    ASSERT_TRUE(ds->processUpdate(FIVE_TO_NINE_SHRINK_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    root->clearPending();

    ASSERT_EQ(8, component->getChildCount());
    ASSERT_TRUE(CheckBounds(5, 13));
}

static const char *SHRINK_FULL_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "minimumInclusiveIndex": 0,
  "maximumExclusiveIndex": 0,
  "items": [ 5, 6, 7, 8, 9 ]
})";

TEST_F(DynamicIndexListLazyTest, BoundsShrinkFull) {
    loadDocument(BASIC, SMALLER_DATA_BACK);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(5, 15));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 5, 5));
    ASSERT_TRUE(ds->processUpdate(SHRINK_FULL_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE", "INTERNAL_ERROR" }));
    root->clearPending();

    ASSERT_EQ(0, component->getChildCount());
    ASSERT_TRUE(CheckBounds(0, 0));
}

static const char *EXPAND_BOTTOM_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 15,
  "minimumInclusiveIndex": 5,
  "items": [ 15, 16, 17, 18, 19 ]
})";

TEST_F(DynamicIndexListLazyTest, BoundsExpandBottom) {
    loadDocument(BASIC, SMALLER_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(ds->processUpdate(EXPAND_BOTTOM_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    root->clearPending();

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckBounds(5, 20));
}

static const char *EXPAND_TOP_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "maximumExclusiveIndex": 20,
  "items": [ 5, 6, 7, 8, 9 ]
})";

TEST_F(DynamicIndexListLazyTest, BoundsExpandTop) {
    loadDocument(BASIC, SMALLER_DATA_BACK);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(5, 15));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 5, 5));
    ASSERT_TRUE(ds->processUpdate(EXPAND_TOP_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    root->clearPending();

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckBounds(5, 20));
}

static const char *EXPAND_FULL_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "minimumInclusiveIndex": -5,
  "maximumExclusiveIndex": 20,
  "items": [ 5, 6, 7, 8, 9 ]
})";

TEST_F(DynamicIndexListLazyTest, BoundsExpandFull) {
    loadDocument(BASIC, SMALLER_DATA_BACK);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(5, 15));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 5, 5));
    ASSERT_TRUE(ds->processUpdate(EXPAND_FULL_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    root->clearPending();

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 20));
}

static const char *FIFTEEN_EMPTY_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "101",
  "listId": "vQdpOESlok",
  "startIndex": 15,
  "items": []
})";

TEST_F(DynamicIndexListLazyTest, EmptyLazyResponseRetryFail) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(0, 101, 15, "")));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(0, 102, 15, "")));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 15, 5));
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(0, 103, 15, "")));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListLazyTest, EmptyLazyResponseRetryResolved) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_FALSE(ds->processUpdate(FIFTEEN_EMPTY_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *FIFTEEN_SHRINK_RESPONSE = R"({
  "token": "presentationToken",
  "correlationToken": "102",
  "listId": "vQdpOESlok",
  "startIndex": 15,
  "minimumInclusiveIndex": 10,
  "maximumExclusiveIndex": 15,
  "items": []
})";

TEST_F(DynamicIndexListLazyTest, EmptyLazyResponseRetryBoundsUpdated) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_FALSE(ds->processUpdate(FIFTEEN_EMPTY_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_FALSE(ds->processUpdate(FIFTEEN_SHRINK_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE", "MISSING_LIST_ITEMS" }));
    ASSERT_TRUE(CheckBounds(10, 15));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListLazyTest, LazyResponseTimeout) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    // Not yet
    advanceTime(50);
    ASSERT_TRUE(CheckErrors({}));

    // Should go from here
    advanceTime(40);
    ASSERT_TRUE(CheckErrors({ "LOAD_TIMEOUT" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    advanceTime(100);
    ASSERT_TRUE(CheckErrors({ "LOAD_TIMEOUT" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 15, 5));
    advanceTime(100);
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListLazyTest, LazyResponseTimeoutResolvedAfterLost) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    // Not yet
    advanceTime(50);
    ASSERT_TRUE(CheckErrors({}));

    // Should go from here
    advanceTime(40);
    ASSERT_TRUE(CheckErrors({ "LOAD_TIMEOUT" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));

    // Retry response arrives
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 15, "15, 16, 17, 18, 19")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListLazyTest, LazyResponseTimeoutResolvedAfterDelayed) {
    loadDocument(BASIC, SMALLER_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
    ASSERT_TRUE(CheckBounds(10, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    // Not yet
    advanceTime(50);
    ASSERT_TRUE(CheckErrors({}));

    // Should go from here
    advanceTime(40);
    ASSERT_TRUE(CheckErrors({ "LOAD_TIMEOUT" }));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));

    // Original response arrives
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 15, "15, 16, 17, 18, 19")));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // Retry arrives
    ASSERT_FALSE(ds->processUpdate(createLazyLoad(-1, 102, 15, "15, 16, 17, 18, 19")));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *PROACTIVE_LOAD_ONLY = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 5,
    "minimumInclusiveIndex": 5,
    "maximumExclusiveIndex": 5
  }
})";

static const char *PROACTIVE_EXPAND_RESPONSE = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "minimumInclusiveIndex": 5,
  "maximumExclusiveIndex": 10,
  "items": [ 5, 6, 7, 8, 9 ]
})";

TEST_F(DynamicIndexListLazyTest, ProactiveLoadOnly)
{
    loadDocument(BASIC, PROACTIVE_LOAD_ONLY);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(ds->processUpdate(PROACTIVE_EXPAND_RESPONSE));
    ASSERT_TRUE(CheckErrors({ "INCONSISTENT_RANGE" }));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    ASSERT_TRUE(CheckBounds(5, 10));

    ASSERT_FALSE(root->hasEvent());
}

static const char *PROACTIVE_EXPAND_BAD_RESPONSE = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": 5,
  "minimumInclusiveIndex": 5
  "maximumExclusiveIndex": 10
  "items": [ 5, 6, 7, 8, 9 ]
})";

TEST_F(DynamicIndexListLazyTest, ProactiveLoadOnlyBadJson)
{
    loadDocument(BASIC, PROACTIVE_LOAD_ONLY);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(0, component->getChildCount());

    ASSERT_FALSE(root->hasEvent());

    ASSERT_FALSE(ds->processUpdate(PROACTIVE_EXPAND_BAD_RESPONSE));
}

static const char *BASIC_CONFIG_CHANGE = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "preserve": ["centerIndex"],
      "height": 300,
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  },
  "onConfigChange": [
    {
      "type": "Reinflate"
    }
  ]
})";

TEST_F(DynamicIndexListLazyTest, Reinflate) {
    config->set(RootProperty::kSequenceChildCache, 0);

    loadDocument(BASIC_CONFIG_CHANGE, DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(0, 20));
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 15, 5));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(1, 101, 15, "15, 16, 17, 18, 19")));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(2, 102, 5, "5, 6, 7, 8, 9")));
    root->clearPending();
    ASSERT_EQ(15, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 0, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(3, 103, 0, "0, 1, 2, 3, 4")));
    root->clearPending();
    ASSERT_EQ(20, component->getChildCount());
    ASSERT_FALSE(root->hasEvent());

    // re-inflate should get same result.
    auto oldComponentId = component->getId();
    configChangeReinflate(ConfigurationChange(100, 100));
    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getId(), oldComponentId);
    ASSERT_EQ(20, component->getChildCount());
    ASSERT_TRUE(CheckBounds(0, 20));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(ds->processUpdate(createReplace(4, 10, 110)));
}

static const char *TYPED_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 20,
    "items": [
      { "type": "TYPE1", "value": 0 },
      { "type": "TYPE2", "value": 1 },
      { "type": "TYPE2", "value": 2 },
      { "type": "TYPE1", "value": 3 },
      { "type": "TYPE1", "value": 4 },
      { "type": "TYPE1", "value": 5 },
      { "type": "TYPE1", "value": 6 },
      { "type": "TYPE1", "value": 7 },
      { "type": "TYPE1", "value": 8 },
      { "type": "TYPE1", "value": 9 }
    ]
  }
})";

static const char *MULTITYPE_SEQUENCE = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "height": 200,
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "when": "${data.type == 'TYPE2'}",
        "id": "id${data.value}",
        "width": 100,
        "height": 100,
        "text": "${data.value}"
      }
    }
  }
})";

TEST_F(DynamicIndexListLazyTest, ConditionalSequenceChildren)
{
    loadDocument(MULTITYPE_SEQUENCE, TYPED_DATA);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 10,
        "{\"type\": \"TYPE1\", \"value\": 10},"
        "{\"type\": \"TYPE1\", \"value\": 11},"
        "{\"type\": \"TYPE1\", \"value\": 12},"
        "{\"type\": \"TYPE1\", \"value\": 13},"
        "{\"type\": \"TYPE1\", \"value\": 14}")));
    root->clearPending();

    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 15,
         "{\"type\": \"TYPE1\", \"value\": 15},"
         "{\"type\": \"TYPE1\", \"value\": 16},"
         "{\"type\": \"TYPE1\", \"value\": 17},"
         "{\"type\": \"TYPE2\", \"value\": 18},"
         "{\"type\": \"TYPE2\", \"value\": 19}")));
    root->clearPending();

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *TYPED_DATA_BACK = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": -15,
    "maximumExclusiveIndex": 2,
    "items": [
      { "type": "TYPE2", "value": 0 },
      { "type": "TYPE1", "value": 1 }
    ]
  }
})";

TEST_F(DynamicIndexListLazyTest, ConditionalSequenceChildrenBackwards)
{
    loadDocument(MULTITYPE_SEQUENCE, TYPED_DATA_BACK);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(-15, 2));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", -5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, -5,
                                                 "{\"type\": \"TYPE1\", \"value\": -5},"
                                                 "{\"type\": \"TYPE1\", \"value\": -4},"
                                                 "{\"type\": \"TYPE1\", \"value\": -3},"
                                                 "{\"type\": \"TYPE1\", \"value\": -2},"
                                                 "{\"type\": \"TYPE1\", \"value\": -1}")));
    root->clearPending();

    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, -10,
                                                 "{\"type\": \"TYPE1\", \"value\": -10},"
                                                 "{\"type\": \"TYPE1\", \"value\": -9},"
                                                 "{\"type\": \"TYPE1\", \"value\": -8},"
                                                 "{\"type\": \"TYPE2\", \"value\": -7},"
                                                 "{\"type\": \"TYPE2\", \"value\": -6}")));
    root->clearPending();

    ASSERT_EQ(3, component->getChildCount());
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", -15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, -15,
                                                 "{\"type\": \"TYPE1\", \"value\": -15},"
                                                 "{\"type\": \"TYPE1\", \"value\": -14},"
                                                 "{\"type\": \"TYPE1\", \"value\": -13},"
                                                 "{\"type\": \"TYPE2\", \"value\": -12},"
                                                 "{\"type\": \"TYPE2\", \"value\": -11}")));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *TYPED_DATA_START_EMPTY = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 5,
    "items": [
      { "type": "TYPE1", "value": 0 },
      { "type": "TYPE1", "value": 1 }
    ]
  }
})";

TEST_F(DynamicIndexListLazyTest, ConditionalSequenceChildrenStartEmpty)
{
    loadDocument(MULTITYPE_SEQUENCE, TYPED_DATA_START_EMPTY);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(0, 5));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 2, 3));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 2,
                                                 "{\"type\": \"TYPE1\", \"value\": 2},"
                                                 "{\"type\": \"TYPE2\", \"value\": 3},"
                                                 "{\"type\": \"TYPE1\", \"value\": 4}")));
    root->clearPending();
    ASSERT_EQ(1, component->getChildCount());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *MULTITYPE_PAGER = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Pager",
      "height": 200,
      "width": 200,
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "when": "${data.type == 'TYPE2'}",
        "id": "id${data.value}",
        "width": 100,
        "height": 100,
        "text": "${data.value}"
      }
    }
  }
})";

TEST_F(DynamicIndexListLazyTest, ConditionalPagerChildren)
{
    loadDocument(MULTITYPE_PAGER, TYPED_DATA);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(0, 20));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 10,
                                                 "{\"type\": \"TYPE1\", \"value\": 10},"
                                                 "{\"type\": \"TYPE1\", \"value\": 11},"
                                                 "{\"type\": \"TYPE1\", \"value\": 12},"
                                                 "{\"type\": \"TYPE1\", \"value\": 13},"
                                                 "{\"type\": \"TYPE1\", \"value\": 14}")));
    root->clearPending();

    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", 15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, 15,
                                                 "{\"type\": \"TYPE1\", \"value\": 15},"
                                                 "{\"type\": \"TYPE1\", \"value\": 16},"
                                                 "{\"type\": \"TYPE1\", \"value\": 17},"
                                                 "{\"type\": \"TYPE2\", \"value\": 18},"
                                                 "{\"type\": \"TYPE2\", \"value\": 19}")));
    root->clearPending();

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListLazyTest, ConditionalPagerChildrenBackwards)
{
    loadDocument(MULTITYPE_PAGER, TYPED_DATA_BACK);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(-15, 2));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", -5, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, -5,
                                                 "{\"type\": \"TYPE1\", \"value\": -5},"
                                                 "{\"type\": \"TYPE1\", \"value\": -4},"
                                                 "{\"type\": \"TYPE1\", \"value\": -3},"
                                                 "{\"type\": \"TYPE1\", \"value\": -2},"
                                                 "{\"type\": \"TYPE1\", \"value\": -1}")));
    root->clearPending();

    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -10, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 102, -10,
                                                 "{\"type\": \"TYPE1\", \"value\": -10},"
                                                 "{\"type\": \"TYPE1\", \"value\": -9},"
                                                 "{\"type\": \"TYPE1\", \"value\": -8},"
                                                 "{\"type\": \"TYPE2\", \"value\": -7},"
                                                 "{\"type\": \"TYPE2\", \"value\": -6}")));
    root->clearPending();

    ASSERT_EQ(3, component->getChildCount());
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", -15, 5));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 103, -15,
                                                 "{\"type\": \"TYPE1\", \"value\": -15},"
                                                 "{\"type\": \"TYPE1\", \"value\": -14},"
                                                 "{\"type\": \"TYPE1\", \"value\": -13},"
                                                 "{\"type\": \"TYPE2\", \"value\": -12},"
                                                 "{\"type\": \"TYPE2\", \"value\": -11}")));
    root->clearPending();

    ASSERT_EQ(5, component->getChildCount());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListLazyTest, ConditionalPagerChildrenStartEmpty)
{
    loadDocument(MULTITYPE_PAGER, TYPED_DATA_START_EMPTY);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(0, 5));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 2, 3));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(-1, 101, 2,
                                                 "{\"type\": \"TYPE1\", \"value\": 2},"
                                                 "{\"type\": \"TYPE2\", \"value\": 3},"
                                                 "{\"type\": \"TYPE1\", \"value\": 4}")));
    root->clearPending();
    ASSERT_EQ(1, component->getChildCount());

    // Check that timeout is not there
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

static const char *FORWARD_ONLY_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 10,
    "items": [ 0, 1, 2, 3, 4 ]
  }
})";

static const char *SHRINK_BOUNDS_WITHOUT_ITEMS = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "startIndex": 0,
  "minimumInclusiveIndex": 0,
  "maximumExclusiveIndex": 5
})";

TEST_F(DynamicIndexListLazyTest, ShrinkWithoutItems)
{
    loadDocument(BASIC, FORWARD_ONLY_DATA);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 5, 5));

    ASSERT_TRUE(ds->processUpdate(SHRINK_BOUNDS_WITHOUT_ITEMS));
    ASSERT_TRUE(CheckErrors({"INCONSISTENT_RANGE", "MISSING_LIST_ITEMS"}));
    root->clearPending();

    advanceTime(10000);
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(DynamicIndexListLazyTest, NewDataCanArriveDuringPageTransitions)
{
    auto swipeToNextPage = [&]() {
        root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(150, 10)));
        advanceTime(100);
        root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(50, 10)));
        root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(50, 10)));
        root->clearPending();
    };

    loadDocument(BASIC_PAGER, BASIC_PAGER_DATA);

    // Start with a pager that has 5 children and is on the first page (frame-10)
    ASSERT_EQ(kComponentTypePager, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_EQ(0, component->getCalculated(kPropertyCurrentPage).asNumber());
    ASSERT_EQ("frame-10", component->getChildAt(0)->getId());

    // Swipe! There's an animation involved, so it's not instantaneous, but we get to the next page
    swipeToNextPage();
    ASSERT_EQ(0, component->getCalculated(kPropertyCurrentPage).asNumber());
    advanceTime(1000);
    ASSERT_EQ(1, component->getCalculated(kPropertyCurrentPage).asNumber());
    ASSERT_EQ("frame-11", component->getChildAt(1)->getId());

    // Swipe! But quickly load 5 pages to the left, before the swipe completes
    swipeToNextPage();
    ASSERT_TRUE(ds->processUpdate(FIVE_TO_NINE_FOLLOWUP_PAGER));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_EQ("frame-5", component->getChildAt(0)->getId());
    ASSERT_EQ("frame-14", component->getChildAt(9)->getId());

    // After the swipe completes, we're at page 1 (original) + 1 (swipe) + 5 (new items) = 7
    advanceTime(1000);
    ASSERT_EQ(7, component->getCalculated(kPropertyCurrentPage).asNumber());
    ASSERT_EQ("frame-12", component->getChildAt(7)->getId());

    // Some errors are expected from unfulfilled requests
    ASSERT_TRUE(ds->getPendingErrors().size() > 0);
}
