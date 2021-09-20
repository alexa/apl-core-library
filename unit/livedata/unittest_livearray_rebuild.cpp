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

#include "../testeventloop.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/livedata/livedatamanager.h"

namespace apl {

class LiveArrayRebuildTest : public DocumentWrapper {
public:
    ::testing::AssertionResult
    CheckChildOrder(const std::vector<std::string>& values) {
        if (values.size() != component->getChildCount())
            return ::testing::AssertionFailure() << "Mismatch in list length (expected=" << values.size()
                                                 << " actual=" << component->getChildCount() << ")";

        int i = 0;
        for (const auto& v : values) {
            auto s = component->getChildAt(i)->getCalculated(kPropertyText).asString();
            if (v != s)
                return ::testing::AssertionFailure() << "Mismatch at index=" << i << " expected='" << v
                                                     << "' actual='" << s << "'";
            i++;
        }

        return ::testing::AssertionSuccess();
    }

    /**
     * Check child type and value for all children.  The value comparison is the text property for
     * text components and the "source" property for images.
     */
    ::testing::AssertionResult
    CheckChildAndType(const std::vector<std::pair<ComponentType, std::string>>& values) {
        if (values.size() != component->getChildCount())
            return ::testing::AssertionFailure() << "Mismatch in list length (expected=" << values.size()
                                                 << " actual=" << component->getChildCount() << ")";

        int i = 0;
        for (const auto& v : values) {
            auto child = component->getChildAt(i);
            auto childType = child->getType();
            if (childType != v.first)
                return ::testing::AssertionFailure() << "Mismatch child type at index=" << i
                                                     << " expected=" << sComponentTypeBimap.at(v.first)
                                                     << " actual=" << sComponentTypeBimap.at(childType);

            auto s = child->getCalculated(childType == kComponentTypeText ? kPropertyText : kPropertySource).asString();

            if (v.second != s)
                return ::testing::AssertionFailure() << "Mismatch at index=" << i
                                                     << " expected='" << v.second
                                                     << "' actual='" << s << "'";
            i++;
        }

        return ::testing::AssertionSuccess();
    }

    ObjectMap
    makeInsert( int index, const std::string& uid ) {
        return ObjectMap{{"index", index}, {"uid", uid}, {"action", "insert"}};
    }

    ObjectMap
    makeRemove( int index, const std::string& uid ) {
        return ObjectMap{{"index", index}, {"uid", uid}, {"action", "remove"}};
    }

    /**
     * Check content of kPropertyNotifyChildrenChanged
     */
    ::testing::AssertionResult
    CheckUpdatedComponentsNotification(const std::vector<ObjectMap>& change) {
        auto dirty = root->getDirty();
        if (!dirty.count(component))
            return ::testing::AssertionFailure()
                    << "No dirty property set.";

        if (!component->getDirty().count(kPropertyNotifyChildrenChanged))
            return ::testing::AssertionFailure()
                    << "No NotifyChildrenChanged property set.";

        auto notify = component->getCalculated(kPropertyNotifyChildrenChanged);
        const auto& changed = notify.getArray();

        if (changed.size() != change.size())
            return ::testing::AssertionFailure()
                    << "Inserted components count is wrong. Expected: " << change.size()
                    << ", actual: " << changed.size();

        for (int i = 0; i<changed.size(); i++) {
            if (changed.at(i).getMap() != change.at(i)) {
                return ::testing::AssertionFailure()
                        << "Change notification on position " + std::to_string(i) + " is wrong. "
                        << "Expected: " << Object(std::make_shared<ObjectMap>(change.at(i))).toDebugString()
                        << ", actual: " << changed.at(i).toDebugString();
            }
        }

        root->clearDirty();

        return ::testing::AssertionSuccess();
    }

    // Few commodity functions to perform scrolling to handle special change cases
    void completeScroll(const std::string& component, float distance) {
        ASSERT_FALSE(root->hasEvent());
        executeScroll(component, distance);
        advanceTime(1000);
    }

private:
    void executeScroll(const std::string& component, float distance) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "Scroll", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("distance", distance, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        root->executeCommands(doc, false);
    }

    rapidjson::Document doc;
};


static const char *BASIC_DOC = R"({
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

TEST_F(LiveArrayRebuildTest, ComponentClear) {
    auto myArray = LiveArray::create(ObjectArray{1, 2});
    config->liveData("TestArray", myArray);

    loadDocument(BASIC_DOC);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());

    myArray->clear();
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(0, component->getChildCount());
}

TEST_F(LiveArrayRebuildTest, ComponentExtendEmpty) {
    auto myArray = LiveArray::create();
    config->liveData("TestArray", myArray);

    loadDocument(BASIC_DOC);
    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->getChildCount());

    myArray->push_back("A");  // A
    root->clearPending();
    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"A 0 0 1"}));

    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 0));
    ASSERT_TRUE(CheckUpdatedComponentsNotification({
       makeInsert(0, component->getChildAt(0)->getUniqueId())
    }));
}

TEST_F(LiveArrayRebuildTest, ComponentUpdate)
{
    auto myArray = LiveArray::create(ObjectArray{"A", "B"});
    config->liveData("TestArray", myArray);

    loadDocument(BASIC_DOC);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"A 0 0 2", "B 1 1 2"}));

    myArray->update(1, "B+");
    root->clearPending();

    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"A 0 0 2", "B+ 1 1 2"}));

    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(component->getChildAt(0)));
    ASSERT_TRUE(CheckDirty(component->getChildAt(1), kPropertyText, kPropertyVisualHash));
}

TEST_F(LiveArrayRebuildTest, ComponentPushBack)
{
    auto myArray = LiveArray::create(ObjectArray{"A", "B"});
    config->liveData("TestArray", myArray);

    loadDocument(BASIC_DOC);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"A 0 0 2", "B 1 1 2"}));

    myArray->push_back("C");  // A, B, C
    root->clearPending();
    ASSERT_EQ(3, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"A 0 0 3", "B 1 1 3", "C 2 2 3"}));

    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component->getChildAt(1), kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 2));

    ASSERT_TRUE(CheckUpdatedComponentsNotification({
        makeInsert(2, component->getChildAt(2)->getUniqueId())
    }));
}

TEST_F(LiveArrayRebuildTest, ComponentInsert)
{
    auto myArray = LiveArray::create(ObjectArray{"A", "B"});
    config->liveData("TestArray", myArray);

    loadDocument(BASIC_DOC);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"A 0 0 2", "B 1 1 2"}));

    myArray->insert(0, "C");  // C, A, B
    root->clearPending();
    ASSERT_EQ(3, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"C 0 0 3", "A 1 1 3", "B 2 2 3"}));

    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 0));
    ASSERT_TRUE(CheckDirty(component->getChildAt(1), kPropertyText, kPropertyBounds,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component->getChildAt(2), kPropertyText, kPropertyBounds,
                           kPropertyVisualHash));

    ASSERT_TRUE(CheckUpdatedComponentsNotification({
        makeInsert(0, component->getChildAt(0)->getUniqueId())
    }));
}

TEST_F(LiveArrayRebuildTest, ComponentRemove)
{
    auto myArray = LiveArray::create(ObjectArray{"A", "B"});
    config->liveData("TestArray", myArray);

    loadDocument(BASIC_DOC);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"A 0 0 2", "B 1 1 2"}));
    auto removedComponentId = component->getChildAt(0)->getUniqueId();

    myArray->remove(0);  // B
    root->clearPending();
    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"B 0 0 1"}));

    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyText, kPropertyBounds,
                           kPropertyVisualHash));

    ASSERT_TRUE(CheckUpdatedComponentsNotification({
        makeRemove(0, removedComponentId)
    }));
}


TEST_F(LiveArrayRebuildTest, ComponentRemoveFromEnd)
{
    auto myArray = LiveArray::create(ObjectArray{"A", "B"});
    config->liveData("TestArray", myArray);

    loadDocument(BASIC_DOC);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"A 0 0 2", "B 1 1 2"}));
    auto removedComponentId = component->getChildAt(1)->getUniqueId();

    myArray->remove(1);  // A
    root->clearPending();
    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"A 0 0 1"}));

    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyText, kPropertyVisualHash));

    ASSERT_TRUE(CheckUpdatedComponentsNotification({
        makeRemove(1, removedComponentId)
    }));
}

TEST_F(LiveArrayRebuildTest, ComponentInsertPushBack)
{
    auto myArray = LiveArray::create(ObjectArray{"A", "B"});
    config->liveData("TestArray", myArray);

    loadDocument(BASIC_DOC);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"A 0 0 2", "B 1 1 2"}));

    myArray->insert(0, "Z");  // Z, A, B
    myArray->push_back("C");  // Z, A, B, C
    root->clearPending();

    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 0));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 3));
    ASSERT_TRUE(CheckUpdatedComponentsNotification({
        makeInsert(0, component->getChildAt(0)->getUniqueId()),
        makeInsert(3, component->getChildAt(3)->getUniqueId())
    }));

    ASSERT_EQ(4, component->getChildCount());
    ASSERT_TRUE(CheckChildOrder({"Z 0 0 4", "A 1 1 4", "B 2 2 4", "C 3 3 4"}));
}

TEST_F(LiveArrayRebuildTest, InsertAround) {
    auto myArray = LiveArray::create(ObjectArray{1, 2});
    config->liveData("TestArray", myArray);

    loadDocument(BASIC_DOC);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());

    myArray->insert(0, 0);
    root->clearPending();
    ASSERT_EQ(3, component->getChildCount());

    myArray->insert(3, 3);
    root->clearPending();
    ASSERT_EQ(4, component->getChildCount());

    myArray->insert(2, 2.1);
    root->clearPending();
    ASSERT_EQ(5, component->getChildCount());
}

TEST_F(LiveArrayRebuildTest, Remove) {
    auto myArray = LiveArray::create(ObjectArray{1, 2, 3});
    config->liveData("TestArray", myArray);

    loadDocument(BASIC_DOC);
    ASSERT_TRUE(component);
    ASSERT_EQ(3, component->getChildCount());

    myArray->insert(0, 0);
    myArray->insert(4, 4);
    root->clearPending();
    ASSERT_EQ(5, component->getChildCount());

    myArray->remove(2);
    root->clearPending();
    ASSERT_EQ(4, component->getChildCount());

    myArray->remove(3);
    myArray->remove(0);
    root->clearPending();
    ASSERT_EQ(2, component->getChildCount());
}

static const char *LIMITED_SEQUENCE = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "width": 100,
      "height": 100,
      "data": "${TestArray}",
      "item": {
        "type": "Frame",
        "width": "100%",
        "height": 60
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, CacheOnSecondFrame) {
    auto myArray = LiveArray::create(ObjectArray{0, 1, 2, 3, 4, 5});
    config->liveData("TestArray", myArray);

    loadDocument(LIMITED_SEQUENCE);
    ASSERT_TRUE(component);
    ASSERT_EQ(6, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0,1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2,5}, false));
    root->clearDirty();

    advanceTime(10);
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0,3}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4,5}, false));
}

static const char *LIMITED_SEQUENCE_DEEP = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "width": 100,
      "height": 100,
      "data": "${TestArray}",
      "item": {
        "type": "Frame",
        "width": "100%",
        "height": 60,
        "item": {
          "type": "Text",
          "text": "${data}",
          "width": "100%",
          "height": "100%"
        }
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, CacheOnSecondFrameDeep) {
    auto myArray = LiveArray::create(ObjectArray{0, 1, 2, 3, 4, 5});
    config->liveData("TestArray", myArray);

    loadDocument(LIMITED_SEQUENCE_DEEP);
    ASSERT_TRUE(component);
    ASSERT_EQ(6, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0,1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2,5}, false));
    root->clearDirty();

    advanceTime(10);
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    for (auto dp : component->getCoreChildAt(2)->getCalculated(kPropertyNotifyChildrenChanged).getArray()) {
        LOG(LogLevel::kError) << dp.toDebugString();
    }
    ASSERT_TRUE(CheckDirty(component->getCoreChildAt(2), kPropertyNotifyChildrenChanged, kPropertyBounds,
                           kPropertyInnerBounds, kPropertyLaidOut, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component->getCoreChildAt(2)->getChildAt(0), kPropertyBounds,
                           kPropertyInnerBounds, kPropertyLaidOut, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component->getCoreChildAt(3), kPropertyNotifyChildrenChanged, kPropertyBounds,
                           kPropertyInnerBounds, kPropertyLaidOut, kPropertyVisualHash));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0,3}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4,5}, false));
}

/**
 * Check that removing and adding around conditionally inflated items works
 */
static const char * CONDITIONAL = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "item": {
        "type": "Text",
        "when": "${data % 2 == 0}",
        "text": "${data} ${index} ${dataIndex} ${length}"
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, Conditional)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2, 3, 4});
    config->liveData("TestArray", myArray);

    loadDocument(CONDITIONAL);
    ASSERT_TRUE(component);
    ASSERT_TRUE(CheckChildOrder({"2 0 1 4", "4 1 3 4"}));

    myArray->remove(0);   // 2 3 4
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"2 0 0 3", "4 1 2 3"}));
    auto removedComponentId = component->getChildAt(0)->getUniqueId();

    myArray->insert(0, 10);  // 10 2 3 4
    myArray->insert(0, 11);  // 11 10 2 3 4
    myArray->remove(2);      // 11 10 3 4
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"10 0 1 4", "4 1 3 4"}));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 0));
    ASSERT_TRUE(CheckUpdatedComponentsNotification({
        makeInsert(0, component->getChildAt(0)->getUniqueId()),
        makeRemove(1, removedComponentId)
    }));
    removedComponentId = component->getChildAt(1)->getUniqueId();

    myArray->push_back(23);  // 11 10 3 4 23
    myArray->remove(0);      // 10 3 4 23
    myArray->remove(2);      // 10 3 23
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"10 0 0 3"}));
    ASSERT_TRUE(CheckUpdatedComponentsNotification({
        makeRemove(1, removedComponentId)
    }));
}

/**
 * Verify that changing around the children doesn't re-inflate existing components
 */
static const char *DOUBLE_CONDITIONAL = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "items": [
        {
          "type": "Text",
          "when": "${data % 2 == 0}",
          "text": "${data} ${index} ${dataIndex} ${length}"
        },
        {
          "type": "Image",
          "when": "${data % 3 == 0}",
          "source": "${data} ${index} ${dataIndex} ${length}"
        }
      ]
    }
  }
})";

TEST_F(LiveArrayRebuildTest, DoubleConditional)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2, 3, 4, 5, 6});
    config->liveData("TestArray", myArray);

    loadDocument(DOUBLE_CONDITIONAL);
    ASSERT_TRUE(component);
    ASSERT_TRUE(CheckChildAndType({{kComponentTypeText,  "2 0 1 6"},
                                   {kComponentTypeImage, "3 1 2 6"},
                                   {kComponentTypeText,  "4 2 3 6"},
                                   {kComponentTypeText,  "6 3 5 6"}
                                  }));

    myArray->update(1, 9);  // 1 9 2 3 4 5 6    [Normally 9 would become an image, but it already exists]
    myArray->push_back(9);  // 1 9 2 3 4 5 6 9  [The second 9 becomes an image]
    root->clearPending();
    ASSERT_TRUE(CheckChildAndType({{kComponentTypeText,  "9 0 1 7"},
                                   {kComponentTypeImage, "3 1 2 7"},
                                   {kComponentTypeText,  "4 2 3 7"},
                                   {kComponentTypeText,  "6 3 5 7"},
                                   {kComponentTypeImage, "9 4 6 7"}
                                  }));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 4));
}

static const char *FIRST_AND_LAST = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "items": {
        "type": "Text",
        "when": "${data % 2 == 0}",
        "text": "${data} ${index} ${dataIndex} ${length}"
      },
      "firstItem": {
        "type": "Text",
        "text": "first"
      },
      "lastItem": {
        "type": "Text",
        "text": "last"
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, FirstAndLast)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2, 3, 4});
    config->liveData("TestArray", myArray);

    loadDocument(FIRST_AND_LAST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(CheckChildOrder({"first", "2 0 1 4", "4 1 3 4", "last"}));

    myArray->push_back(10);  // 1, 2, 3, 4, 10
    myArray->insert(0, 20);  // 20, 1, 2, 3, 4, 10
    myArray->remove(1);      // 20, 2, 3, 4, 10
    myArray->remove(4);      // 20, 2, 3, 4
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"first", "20 0 0 4", "2 1 1 4", "4 2 3 4", "last"}));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 1));

    myArray->clear();   // ...none left...
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"first", "last"}));

    myArray->push_back(100);  // 100
    myArray->insert(0, 200);  // 200, 100
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"first", "200 0 0 2", "100 1 1 2", "last"}));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 1));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 2));
}

static const char *FIRST_ONLY = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "items": {
        "type": "Text",
        "when": "${data % 2 == 0}",
        "text": "${data} ${index} ${dataIndex} ${length}"
      },
      "firstItem": {
        "type": "Text",
        "text": "first"
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, FirstOnly)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2, 3, 4});
    config->liveData("TestArray", myArray);

    loadDocument(FIRST_ONLY);
    ASSERT_TRUE(component);
    ASSERT_TRUE(CheckChildOrder({"first", "2 0 1 4", "4 1 3 4"}));

    myArray->push_back(10);  // 1, 2, 3, 4, 10
    myArray->insert(0, 20);  // 20, 1, 2, 3, 4, 10
    myArray->remove(1);      // 20, 2, 3, 4, 10
    myArray->remove(4);      // 20, 2, 3, 4
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"first", "20 0 0 4", "2 1 1 4", "4 2 3 4"}));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 1));

    myArray->clear();   // ...none left...
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"first"}));

    myArray->push_back(100);  // 100
    myArray->insert(0, 200);  // 200, 100
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"first", "200 0 0 2", "100 1 1 2"}));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 1));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 2));
}

static const char *LAST_ONLY = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "items": {
        "type": "Text",
        "when": "${data % 2 == 0}",
        "text": "${data} ${index} ${dataIndex} ${length}"
      },
      "lastItem": {
        "type": "Text",
        "text": "last"
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, LastOnly)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2, 3, 4});
    config->liveData("TestArray", myArray);

    loadDocument(LAST_ONLY);
    ASSERT_TRUE(component);
    ASSERT_TRUE(CheckChildOrder({"2 0 1 4", "4 1 3 4", "last"}));

    myArray->push_back(10);  // 1, 2, 3, 4, 10
    myArray->insert(0, 20);  // 20, 1, 2, 3, 4, 10
    myArray->remove(1);      // 20, 2, 3, 4, 10
    myArray->remove(4);      // 20, 2, 3, 4
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"20 0 0 4", "2 1 1 4", "4 2 3 4", "last"}));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 0));

    myArray->clear();   // ...none left...
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"last"}));

    myArray->push_back(100);  // 100
    myArray->insert(0, 200);  // 200, 100
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"200 0 0 2", "100 1 1 2", "last"}));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 0));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 1));
}

::testing::AssertionResult
AlternateColor(const CoreComponentPtr& component, const ObjectArray& colors)
{
    for (int i = 0 ; i < component->getChildCount() ; i++) {
        const auto& child = component->getChildAt(i);
        const auto& color = colors.at(i % colors.size());

        if (child->getCalculated(kPropertyColor) != color)
            return ::testing::AssertionFailure() << "Color index " << i << " does not match";
    }

    return ::testing::AssertionSuccess();
}

static const char *NUMBERING = R"({
  "type": "APL",
  "version": "1.2",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "data": "${TestArray}",
      "numbered": true,
      "items": {
        "type": "Text",
        "color": "${index % 2 ? 'black' : 'gray'}",
        "numbering": "${index == 3 ? 'reset' : 'normal'}",
        "text": "${ordinal}-${data}"
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, Numbering)
{
    auto myArray = LiveArray::create(ObjectArray{"a", "b", "c", "d", "e", "f"});
    config->liveData("TestArray", myArray);

    loadDocument(NUMBERING);
    ASSERT_TRUE(component);
    ASSERT_EQ(6, component->getChildCount());

    ASSERT_TRUE(CheckChildOrder({"1-a", "2-b", "3-c", "4-d", "1-e", "2-f"}));
    ASSERT_TRUE(AlternateColor(component, {Color(Color::GRAY), Color(Color::BLACK)}));

    myArray->remove(0);  // Remove the first element
    root->clearPending();
    ASSERT_TRUE(CheckChildOrder({"1-b", "2-c", "3-d", "1-e", "2-f"})); // NOTE: Numbering is NOT dynamic
    ASSERT_TRUE(AlternateColor(component, {Color(Color::GRAY), Color(Color::BLACK)}));

    ASSERT_EQ("Property numbering is not dynamic and can't be updated.", session->getLast());
    session->clear();
}

::testing::AssertionResult
CheckComponentChildOrder(const CoreComponentPtr& component, const std::vector<std::string>& values) {
    if (values.size() != component->getChildCount())
        return ::testing::AssertionFailure() << "Mismatch in list length (expected=" << values.size()
                                             << " actual=" << component->getChildCount() << ")";

    int i = 0;
    for (const auto& v : values) {
        auto s = component->getChildAt(i)->getCalculated(kPropertyText).asString();
        if (v != s)
            return ::testing::AssertionFailure() << "Mismatch at index=" << i << " expected='" << v
                                                 << "' actual='" << s << "'";
        i++;
    }

    return ::testing::AssertionSuccess();
}

static const char *MULTIPLE_CONTEXT = R"({
  "type": "APL",
  "version": "1.2",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "data": "${TestArray}",
      "items": {
        "type": "Text",
        "color": "${index % 2 ? 'black' : 'gray'}",
        "text": "${data}"
      }
    }
  }
})";

// Demonstrate that you can connect the same LiveArray to multiple RootContext objects and
// have them update separately
TEST_F(LiveArrayRebuildTest, MultipleContexts)
{
    auto myArray = LiveArray::create(ObjectArray{"a", "b", "c", "d", "e", "f"});
    config->liveData("TestArray", myArray);

    auto content1 = Content::create(MULTIPLE_CONTEXT, session);
    auto root1 = RootContext::create(metrics, content1, *config);
    auto root2 = RootContext::create(metrics, content1, *config);

    auto component1 = std::dynamic_pointer_cast<CoreComponent>(root1->topComponent());
    auto component2 = std::dynamic_pointer_cast<CoreComponent>(root2->topComponent());

    ASSERT_TRUE(CheckComponentChildOrder(component1, {"a", "b", "c", "d", "e", "f"}));
    ASSERT_TRUE(CheckComponentChildOrder(component2, {"a", "b", "c", "d", "e", "f"}));

    myArray->remove(1, 3);  // a, e, f
    root1->clearPending();

    ASSERT_TRUE(CheckComponentChildOrder(component1, {"a", "e", "f"}));
    ASSERT_TRUE(CheckComponentChildOrder(component2, {"a", "b", "c", "d", "e", "f"}));

    myArray->insert(0, "z");

    root2->clearPending();
    ASSERT_TRUE(CheckComponentChildOrder(component1, {"a", "e", "f"}));
    ASSERT_TRUE(CheckComponentChildOrder(component2, {"z", "a", "e", "f"}));

    root1->clearPending();
    ASSERT_TRUE(CheckComponentChildOrder(component1, {"z", "a", "e", "f"}));
    ASSERT_TRUE(CheckComponentChildOrder(component2, {"z", "a", "e", "f"}));

    root1->clearDirty();
    root2->clearDirty();
}

class InflateTextMeasure : public TextMeasurement {
public:
    LayoutSize measure(Component *component, float width, MeasureMode widthMode, float height,
                   MeasureMode heightMode) override {
        int symbols = component->getCalculated(kPropertyText).asString().size();
        float newHeight = symbols > 10 ? 200 : 100;
        return LayoutSize({width, newHeight});
    }

    float baseline(Component *component, float width, float height) override {
        return height;
    }
};

static const char *LIVE_SEQUENCE = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "data": "${TestArray}",
      "height": 300,
      "items": {
        "type": "Text",
        "text": "${data}",
        "color": "black",
        "width": 100,
        "height": "auto"
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, SequencePositionContext)
{
    config->measure(std::make_shared<InflateTextMeasure>());
    auto myArray = LiveArray::create();
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_SEQUENCE);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(0, component->getChildCount());

    auto scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(0, scrollPosition);

    myArray->push_back("10");
    myArray->push_back("11");
    myArray->push_back("12");
    myArray->push_back("13");
    myArray->push_back("14");
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlags(component, {0, 4}));

    // Ensure current and collect context

    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(0, scrollPosition);

    ASSERT_TRUE(CheckDirtyVisualContext(root,component));
    rapidjson::Document document(rapidjson::kObjectType);
    auto context = root->serializeVisualContext(document.GetAllocator());

    ASSERT_TRUE(context.HasMember("tags"));
    auto& tags = context["tags"];
    ASSERT_STREQ("sequence", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    auto& list = tags["list"];
    ASSERT_EQ(5, list["itemCount"].GetInt());
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(2, list["highestIndexSeen"].GetInt());

    // Add some more items and check context still correct.
    myArray->insert(0, "5");
    myArray->insert(0, "6");
    myArray->insert(0, "7");
    myArray->insert(0, "8");
    myArray->insert(0, "9");
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlags(component, {2, 4}));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(component->getCalculated(kPropertyScrollPosition).isDimension());
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(400, scrollPosition);

    ASSERT_TRUE(component->isVisualContextDirty());
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_FALSE(component->isVisualContextDirty());

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("sequence", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(10, list["itemCount"].GetInt());
    ASSERT_EQ(5, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(7, list["highestIndexSeen"].GetInt());

    // Move position and check it's still right
    component->update(kUpdateScrollPosition, 100);
    root->clearPending();

    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_TRUE(component->getCalculated(kPropertyScrollPosition).isDimension());
    ASSERT_EQ(200, scrollPosition);

    ASSERT_TRUE(component->isVisualContextDirty());
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_FALSE(component->isVisualContextDirty());

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("sequence", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(10, list["itemCount"].GetInt());
    ASSERT_EQ(2, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(7, list["highestIndexSeen"].GetInt());

    // Add even more items and check context still correct.
    myArray->insert(0, "0");
    myArray->insert(0, "1");
    myArray->insert(0, "2");
    myArray->insert(0, "3");
    myArray->insert(0, "4");
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 6));
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(400, scrollPosition);

    ASSERT_TRUE(component->isVisualContextDirty());
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_FALSE(component->isVisualContextDirty());

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("sequence", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(15, list["itemCount"].GetInt());
    ASSERT_EQ(7, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(12, list["highestIndexSeen"].GetInt());
}

TEST_F(LiveArrayRebuildTest, SequenceContextInsertRemove)
{
    config->measure(std::make_shared<InflateTextMeasure>());
    auto myArray = LiveArray::create(ObjectArray{"10", "11", "12", "13", "14"});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_SEQUENCE);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());

    auto scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(0, scrollPosition);

    rapidjson::Document document(rapidjson::kObjectType);
    auto context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_FALSE(component->isVisualContextDirty());

    ASSERT_TRUE(context.HasMember("tags"));
    auto& tags = context["tags"];
    ASSERT_STREQ("sequence", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    auto& list = tags["list"];
    ASSERT_EQ(5, list["itemCount"].GetInt());
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(2, list["highestIndexSeen"].GetInt());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));

    component->update(kUpdateScrollPosition, 200);
    ASSERT_TRUE(component->isVisualContextDirty());

    // Insert items before scroll position in un-ensured and ensured area.
    myArray->insert(2, "12.5");
    myArray->insert(0, "9");
    myArray->insert(0, "8");
    root->clearPending();

    // Check if it processed this well
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 4));
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(400, scrollPosition);
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 0}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {1, 6}, true));

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    context = root->serializeVisualContext(document.GetAllocator());

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("sequence", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(8, list["itemCount"].GetInt());
    ASSERT_EQ(2, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(7, list["highestIndexSeen"].GetInt());

    // Remove items before scroll position in un-ensured and ensured area.
    myArray->remove(1, 2); // 1 unensured + 1 ensured
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 5}, true));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(300, scrollPosition);

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    context = root->serializeVisualContext(document.GetAllocator());

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("sequence", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(6, list["itemCount"].GetInt());
    ASSERT_EQ(1, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(5, list["highestIndexSeen"].GetInt());
}

TEST_F(LiveArrayRebuildTest, SequenceScrollingContext)
{
    config->measure(std::make_shared<InflateTextMeasure>());
    auto myArray = LiveArray::create();
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_SEQUENCE);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(0, component->getChildCount());

    auto scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(0, scrollPosition);

    myArray->push_back("10");
    myArray->push_back("11");
    myArray->push_back("12");
    myArray->push_back("13");
    myArray->push_back("14");
    root->clearPending();

    ASSERT_TRUE(CheckDirtyVisualContext(root, component));
    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlags(component, {0, 4}));
    // Verify initial context
    rapidjson::Document document(rapidjson::kObjectType);
    auto context = root->serializeVisualContext(document.GetAllocator());

    ASSERT_TRUE(context.HasMember("tags"));
    auto& tags = context["tags"];
    ASSERT_STREQ("sequence", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    auto& list = tags["list"];
    ASSERT_EQ(5, list["itemCount"].GetInt());
    ASSERT_EQ(0, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(2, list["highestIndexSeen"].GetInt());

    // Add some items and scroll backwards
    myArray->insert(0, "5");
    myArray->insert(0, "6");
    myArray->insert(0, "7");
    myArray->insert(0, "8");
    myArray->insert(0, "9");
    root->clearPending();

    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlags(component, {2, 4}));

    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(400, scrollPosition);

    completeScroll("sequence", -1);

    // Check context and position (-1 page == 3 children back == 300 - 300 + 2 new item = 200)
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(200, scrollPosition);

    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlags(component, {0, 1}));

    ASSERT_TRUE(component->isVisualContextDirty());
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_FALSE(component->isVisualContextDirty());

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("sequence", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(10, list["itemCount"].GetInt());
    ASSERT_EQ(2, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(7, list["highestIndexSeen"].GetInt());

    myArray->insert(0, "0");
    myArray->insert(0, "1");
    myArray->insert(0, "2");
    myArray->insert(0, "3");
    myArray->insert(0, "4");
    myArray->push_back("15");
    myArray->push_back("16");
    myArray->push_back("17");
    myArray->push_back("18");
    myArray->push_back("19");
    root->clearPending();

    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 4));

    ASSERT_TRUE(component->isVisualContextDirty());
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_FALSE(component->isVisualContextDirty());

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("sequence", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(20, list["itemCount"].GetInt());
    ASSERT_EQ(7, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(12, list["highestIndexSeen"].GetInt());

    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(400, scrollPosition);

    // Scroll forwards
    completeScroll("sequence", 2);

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(1000, scrollPosition);

    ASSERT_TRUE(component->isVisualContextDirty());
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_FALSE(component->isVisualContextDirty());

    tags = context["tags"];
    ASSERT_STREQ("sequence", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("list"));
    list = tags["list"];
    ASSERT_EQ(20, list["itemCount"].GetInt());
    ASSERT_EQ(7, list["lowestIndexSeen"].GetInt());
    ASSERT_EQ(15, list["highestIndexSeen"].GetInt());
}

TEST_F(LiveArrayRebuildTest, SequenceUpdateContext)
{
    config->measure(std::make_shared<InflateTextMeasure>());
    auto myArray = LiveArray::create(ObjectArray{"10", "11", "12", "13", "14"});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_SEQUENCE);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());

    // Ensure current and check position
    auto scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(0, scrollPosition);

    // Move position
    component->update(kUpdateScrollPosition, 100);
    root->clearPending();

    // Update first item size and see if position moved on.
    myArray->update(0, R"({"color": "#BEEF00", "text": "It's a very, very, very, very long string (kind of)."})");
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyScrollPosition, kPropertyNotifyChildrenChanged));
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(200, scrollPosition);
}

static const char *LIVE_SEQUENCE_DEEP = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "data": "${TestArray}",
      "height": 300,
      "items": {
        "type": "Frame",
        "item": {
          "type": "Text",
          "text": "${data}",
          "color": "black",
          "width": 100,
          "height": "auto"
        }
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, SequenceScrollingDeep) {
    config->measure(std::make_shared<InflateTextMeasure>());
    auto myArray = LiveArray::create(ObjectArray{"10", "11", "12", "13", "14"});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_SEQUENCE_DEEP);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());

    auto scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(0, scrollPosition);

    // Add some items and scroll backwards
    myArray->insert(0, "5");
    myArray->insert(0, "6");
    myArray->insert(0, "7");
    myArray->insert(0, "8");
    myArray->insert(0, "9");
    root->clearPending();

    completeScroll("sequence", -1);
    ASSERT_TRUE(CheckChildrenLaidOutDirtyFlagsWithNotify(component, {0, 1}));

    // Check position (-1 page == 5 children back == 300 - 300 + 2 new after move = 200)
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(200, scrollPosition);

    myArray->insert(0, "0");
    myArray->insert(0, "1");
    myArray->insert(0, "2");
    myArray->insert(0, "3");
    myArray->insert(0, "4");
    myArray->push_back("15");
    myArray->push_back("16");
    myArray->push_back("17");
    myArray->push_back("18");
    myArray->push_back("19");
    root->clearPending();

    ASSERT_TRUE(CheckChildLaidOutDirtyFlagsWithNotify(component, 4));

    // Scroll forwards
    completeScroll("sequence", 2);

    // Check position (300 cache + 2 pages * 300 = 900)
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    scrollPosition = component->getCalculated(kPropertyScrollPosition).asNumber();
    ASSERT_EQ(1000, scrollPosition);
}

static const char *LIVE_SEQUENCE_VARIABLE = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "scrollDirection": "vertical",
      "data": "${TestArray}",
      "height": 200,
      "items": {
        "type": "Frame",
        "height": "${data}",
        "item": {
          "type": "Text",
          "text": "${data}",
          "color": "black",
          "width": 100
        }
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, SequenceVariableSize)
{
    config->measure(std::make_shared<InflateTextMeasure>());
    auto myArray = LiveArray::create(ObjectArray{100, 25, 50, 25, 25, 100, 50, 50, 100});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_SEQUENCE_VARIABLE);
    advanceTime(10);

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(9, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 7}, true));
}

static const char *LIVE_PAGER = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "id": "pager",
      "navigation": "normal",
      "data": "${TestArray}",
      "items": {
        "type": "Text",
        "text": "data",
        "color": "black",
        "width": 100,
        "height": 100
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, PagerContext)
{
    auto myArray = LiveArray::create(ObjectArray{"10", "11", "12", "13", "14"});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_PAGER);
    advanceTime(10);
    root->clearDirty();

    ASSERT_EQ(kComponentTypePager, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 1}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {2, 4}, false));

    rapidjson::Document document(rapidjson::kObjectType);
    auto context = root->serializeVisualContext(document.GetAllocator());

    ASSERT_TRUE(context.HasMember("tags"));
    auto& tags = context["tags"];
    ASSERT_STREQ("pager", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("pager"));
    auto& list = tags["pager"];
    ASSERT_EQ(5, list["pageCount"].GetInt());
    ASSERT_EQ(0, list["index"].GetInt());
    ASSERT_EQ(true, list["allowForward"].GetBool());
    ASSERT_EQ(false, list["allowBackwards"].GetBool());

    myArray->insert(0, "5");
    myArray->insert(0, "6");
    myArray->insert(0, "7");
    myArray->insert(0, "8");
    myArray->insert(0, "9");
    myArray->push_back("15");
    myArray->push_back("16");
    myArray->push_back("17");
    myArray->push_back("18");
    myArray->push_back("19");
    root->clearPending();

    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 4));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 3}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4, 6}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {7, 9}, false));

    ASSERT_TRUE(component->isVisualContextDirty());
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_FALSE(component->isVisualContextDirty());

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("pager", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("pager"));
    list = tags["pager"];
    ASSERT_EQ(15, list["pageCount"].GetInt());
    ASSERT_EQ(5, list["index"].GetInt());
    ASSERT_EQ(true, list["allowForward"].GetBool());
    ASSERT_EQ(true, list["allowBackwards"].GetBool());
}

TEST_F(LiveArrayRebuildTest, PagerContextInsertRemove)
{
    auto myArray = LiveArray::create(ObjectArray{"10", "11", "12", "13", "14"});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_PAGER);

    ASSERT_EQ(kComponentTypePager, component->getType());
    ASSERT_EQ(5, component->getChildCount());

    rapidjson::Document document(rapidjson::kObjectType);
    auto context = root->serializeVisualContext(document.GetAllocator());

    ASSERT_TRUE(context.HasMember("tags"));
    auto& tags = context["tags"];
    ASSERT_STREQ("pager", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("pager"));
    auto& list = tags["pager"];
    ASSERT_EQ(5, list["pageCount"].GetInt());
    ASSERT_EQ(0, list["index"].GetInt());
    ASSERT_EQ(true, list["allowForward"].GetBool());
    ASSERT_EQ(false, list["allowBackwards"].GetBool());

    // Insert few and check
    myArray->insert(0, "8");
    myArray->insert(1, "10.5");
    root->clearPending();

    ASSERT_TRUE(CheckChildLaidOut(component, 0, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {1, 3}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {4, 5}, false));

    ASSERT_TRUE(component->isVisualContextDirty());
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_FALSE(component->isVisualContextDirty());

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("pager", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("pager"));
    list = tags["pager"];
    ASSERT_EQ(7, list["pageCount"].GetInt());
    ASSERT_EQ(2, list["index"].GetInt());
    ASSERT_EQ(true, list["allowForward"].GetBool());
    ASSERT_EQ(true, list["allowBackwards"].GetBool());

    // Update position and remove one before it.
    component->update(kUpdatePagerPosition, 3);
    root->clearPending();
    ASSERT_TRUE(CheckChildLaidOutDirtyFlags(component, 4));
    ASSERT_TRUE(CheckChildLaidOut(component, 0, false));
    ASSERT_TRUE(CheckChildrenLaidOut(component, {1, 4}, true));
    ASSERT_TRUE(CheckChildLaidOut(component, 5, false));

    myArray->remove(2, 1);
    root->clearPending();

    ASSERT_TRUE(component->isVisualContextDirty());
    context = root->serializeVisualContext(document.GetAllocator());
    ASSERT_FALSE(component->isVisualContextDirty());

    ASSERT_TRUE(context.HasMember("tags"));
    tags = context["tags"];
    ASSERT_STREQ("pager", context["id"].GetString());
    ASSERT_TRUE(tags.HasMember("pager"));
    list = tags["pager"];
    ASSERT_EQ(6, list["pageCount"].GetInt());
    ASSERT_EQ(2, list["index"].GetInt());
    ASSERT_EQ(true, list["allowForward"].GetBool());
    ASSERT_EQ(true, list["allowBackwards"].GetBool());
}

static const char *LIVE_WRAP_PAGER = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "id": "pager",
      "navigation": "wrap",
      "data": "${TestArray}",
      "items": {
        "type": "Text",
        "text": "data",
        "color": "black",
        "width": 100,
        "height": 100
      }
    }
  },
  "onMount": {
    "type": "Sequential",
    "sequencer": "PAGE_ADVANCER",
    "repeatCount": 1,
    "commands": [
      {
        "type": "SetPage",
        "componentId": "pager",
        "position": "relative",
        "value": 1
      }
    ]
  }
})";

TEST_F(LiveArrayRebuildTest, PagerWrap)
{
    auto myArray = LiveArray::create(ObjectArray{"10", "11"});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_WRAP_PAGER);

    ASSERT_EQ(kComponentTypePager, component->getType());
    ASSERT_EQ(kNavigationWrap, static_cast<Navigation>(component->getCalculated(kPropertyNavigation).getInteger()));
    ASSERT_EQ(2, component->getChildCount());

    ASSERT_EQ(0, component->pagePosition());
    advanceTime(100);
    advanceTime(500);
    ASSERT_EQ(1, component->pagePosition());

    advanceTime(200);
    advanceTime(600);
    ASSERT_EQ(0, component->pagePosition());
}

static const char *LAYOUT_DEPENDENCY = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "layouts": {
    "square": {
      "parameters": [
        "color",
        "text"
      ],
      "item": {
        "type": "Frame",
        "width": 100,
        "height": 100,
        "id": "frame-${text}",
        "backgroundColor": "${color}",
        "item": {
          "type": "Text",
          "text": "${text}",
          "color": "lime",
          "width": 100,
          "height": 100
        }
      }
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "height": 300,
      "data": "${TestArray}",
      "items": {
        "type": "square",
        "index": "${index}",
        "color": "${data.color}",
        "text": "${data.text}"
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, DeepComponentUpdate)
{
    auto initMap = ObjectMap{{"text", "init"}, {"color", "white"}};
    auto myArray = LiveArray::create(ObjectArray{Object(std::make_shared<ObjectMap>(initMap))});
    config->liveData("TestArray", myArray);

    loadDocument(LAYOUT_DEPENDENCY);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());

    ASSERT_EQ("init", component->getChildAt(0)->getChildAt(0)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Color(0xFFFFFFFF), component->getChildAt(0)->getCalculated(kPropertyBackgroundColor).getColor());

    auto updateMap = ObjectMap{{"text", "update"}, {"color", "blue"}};
    myArray->update(0, Object(std::make_shared<ObjectMap>(updateMap)));
    root->clearPending();

    ASSERT_EQ(1, component->getChildCount());

    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(component->getChildAt(0)->getChildAt(0), kPropertyText,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyBackgroundColor, kPropertyVisualHash));

    ASSERT_EQ("update", component->getChildAt(0)->getChildAt(0)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Color(0x0000FFFF), component->getChildAt(0)->getCalculated(kPropertyBackgroundColor).getColor());
}

static const char *SPACED_SEQUENCE = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "scrollDirection": "vertical",
      "data": "${TestArray}",
      "height": 200,
      "item": {
        "type": "Text",
        "text": "${data}",
        "color": "black",
        "spacing": 10,
        "width": 100,
        "height": 100
      }
    }
  }
})";

static ::testing::AssertionResult
CheckSpacing(const CoreComponentPtr& comp, float spacing) {
    float ypos = 0;
    for (int i = 0; i < comp->getChildCount(); i++) {
        auto rect = comp->getCoreChildAt(i)->getCalculated(kPropertyBounds).getRect();
        if (ypos != rect.getTop()) {
            return ::testing::AssertionFailure() << "Position wrong on: " << i
                                                 << " expected='" << ypos
                                                 << "' actual='" << rect.getTop() << "'";
        }
        ypos += (rect.getHeight() + spacing);
    }
    // Last one should end without spacing
    auto lastChildRect = comp->getCoreChildAt(comp->getChildCount() - 1)->getCalculated(kPropertyBounds).getRect();
    ypos -= spacing;
    if (ypos != lastChildRect.getBottom()) {
        return ::testing::AssertionFailure() << "Last child too big";
    }

    return ::testing::AssertionSuccess();
}

TEST_F(LiveArrayRebuildTest, SpacedSequence) {
    auto myArray = LiveArray::create(ObjectArray{0, 1});
    config->liveData("TestArray", myArray);
    config->sequenceChildCache(2);

    loadDocument(SPACED_SEQUENCE);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(2, component->getChildCount());

    ASSERT_TRUE(CheckSpacing(component, 10));
    ASSERT_EQ(0, component->getCalculated(kPropertyScrollPosition).getAbsoluteDimension());

    myArray->insert(2, 2);
    myArray->insert(3, 3);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(4, component->getChildCount());

    ASSERT_TRUE(CheckSpacing(component, 10));
    ASSERT_EQ(0, component->getCalculated(kPropertyScrollPosition).getAbsoluteDimension());

    ObjectArray blockInsert = {12,11};
    myArray->insert(0, blockInsert.begin(), blockInsert.end());
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(6, component->getChildCount());

    ASSERT_TRUE(CheckSpacing(component, 10));
    ASSERT_EQ(220, component->getCalculated(kPropertyScrollPosition).getAbsoluteDimension());

    myArray->insert(0, 13);
    myArray->insert(0, 14);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(8, component->getChildCount());

    ASSERT_TRUE(CheckSpacing(component, 10));
    ASSERT_EQ(440, component->getCalculated(kPropertyScrollPosition).getAbsoluteDimension());

    myArray->insert(1, 111);
    myArray->insert(3, 113);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(10, component->getChildCount());

    ASSERT_TRUE(CheckSpacing(component, 10));
    ASSERT_EQ(660, component->getCalculated(kPropertyScrollPosition).getAbsoluteDimension());

    myArray->remove(0, 4);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_EQ(6, component->getChildCount());

    ASSERT_TRUE(CheckSpacing(component, 10));
    ASSERT_EQ(220, component->getCalculated(kPropertyScrollPosition).getAbsoluteDimension());
}

static const char *SPACED_CONTAINER = R"({
  "type": "APL",
  "version": "1.3",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "container",
      "data": "${TestArray}",
      "item": {
        "type": "Text",
        "text": "${data}",
        "color": "black",
        "spacing": 10,
        "width": 100,
        "height": 100
      }
    }
  }
})";

TEST_F(LiveArrayRebuildTest, SpacedContainer) {
    auto myArray = LiveArray::create(ObjectArray{0, 1});
    config->liveData("TestArray", myArray);
    config->sequenceChildCache(2);

    loadDocument(SPACED_CONTAINER);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeContainer, component->getType());
    ASSERT_EQ(2, component->getChildCount());

    ASSERT_TRUE(CheckSpacing(component, 10));

    myArray->insert(2, 2);
    myArray->insert(3, 3);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(4, component->getChildCount());

    ASSERT_TRUE(CheckSpacing(component, 10));

    ObjectArray blockInsert = {12,11};
    myArray->insert(0, blockInsert.begin(), blockInsert.end());
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(6, component->getChildCount());

    ASSERT_TRUE(CheckSpacing(component, 10));

    myArray->insert(0, 13);
    myArray->insert(0, 14);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(8, component->getChildCount());

    ASSERT_TRUE(CheckSpacing(component, 10));

    myArray->insert(1, 111);
    myArray->insert(3, 113);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(10, component->getChildCount());

    ASSERT_TRUE(CheckSpacing(component, 10));
}

TEST_F(LiveArrayRebuildTest, SpacedContainerColumnReverse) {
    auto myArray = LiveArray::create(ObjectArray{0, 1});
    config->liveData("TestArray", myArray);

    loadDocument(SPACED_CONTAINER);
    auto c  = component->getChildAt(0);
    auto c2 = component->getChildAt(1);
    ASSERT_TRUE(expectBounds(c,    0, 0, 100, 100));
    ASSERT_TRUE(expectBounds(c2, 110, 0, 210, 100));

    component->setProperty(kPropertyDirection, "columnReverse");
    myArray->insert(0, 13);
    myArray->insert(0, 14);
    root->clearPending();

    c  = component->getChildAt(0);
    c2 = component->getChildAt(1);
    auto c3 = component->getChildAt(2);
    auto c4 = component->getChildAt(3);
    ASSERT_TRUE(expectBounds(c,  700, 0, 800, 100));
    ASSERT_TRUE(expectBounds(c2, 590, 0, 690, 100));
    ASSERT_TRUE(expectBounds(c3, 480, 0, 580, 100));
    ASSERT_TRUE(expectBounds(c4, 370, 0, 470, 100));
}

TEST_F(LiveArrayRebuildTest, SpacedContainerRow) {
    auto myArray = LiveArray::create(ObjectArray{0, 1});
    config->liveData("TestArray", myArray);

    loadDocument(SPACED_CONTAINER);
    auto c  = component->getChildAt(0);
    auto c2 = component->getChildAt(1);
    ASSERT_TRUE(expectBounds(c,    0, 0, 100, 100));
    ASSERT_TRUE(expectBounds(c2, 110, 0, 210, 100));

    component->setProperty(kPropertyDirection, "row");
    myArray->insert(0, 13);
    myArray->insert(0, 14);
    root->clearPending();

    c  = component->getChildAt(0);
    c2 = component->getChildAt(1);
    auto c3 = component->getChildAt(2);
    auto c4 = component->getChildAt(3);
    ASSERT_TRUE(expectBounds(c,  0,   0, 100, 100));
    ASSERT_TRUE(expectBounds(c2, 0, 110, 100, 210));
    ASSERT_TRUE(expectBounds(c3, 0, 220, 100, 320));
    ASSERT_TRUE(expectBounds(c4, 0, 330, 100, 430));
}

TEST_F(LiveArrayRebuildTest, SpacedContainerRowReverse) {
    auto myArray = LiveArray::create(ObjectArray{0, 1});
    config->liveData("TestArray", myArray);

    loadDocument(SPACED_CONTAINER);
    auto c  = component->getChildAt(0);
    auto c2 = component->getChildAt(1);
    ASSERT_TRUE(expectBounds(c,    0, 0, 100, 100));
    ASSERT_TRUE(expectBounds(c2, 110, 0, 210, 100));

    component->setProperty(kPropertyDirection, "rowReverse");
    myArray->insert(0, 13);
    myArray->insert(0, 14);
    root->clearPending();

    c  = component->getChildAt(0);
    c2 = component->getChildAt(1);
    auto c3 = component->getChildAt(2);
    auto c4 = component->getChildAt(3);
    ASSERT_TRUE(expectBounds(c,  0, 924, 100, 1024));
    ASSERT_TRUE(expectBounds(c2, 0, 814, 100,  914));
    ASSERT_TRUE(expectBounds(c3, 0, 704, 100,  804));
    ASSERT_TRUE(expectBounds(c4, 0, 594, 100,  694));
}

static const char * SPACED_CONTAINER_WITH_LAYOUTDIR = R"(
{
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "parameters": [ "containerDir", "layoutDir" ],
    "items":{
      "layoutDirection": "${layoutDir}",
      "type": "Frame",
      "backgroundColor": "red",
      "width": "1500px",
      "items": {
        "type": "Container",
        "id": "container",
        "direction": "${containerDir}",
        "width": "1500px",
        "items": {
          "type": "Frame",
          "id": "Frame+${index}",
          "width": "100",
          "backgroundColor": "blue",
          "spacing": 30,
          "items": {
            "type": "Text",
            "text": "* ${index}",
            "width": "100"
          }
        },
        "data": [1, 2, 3, 4]
      }
    }
  }
}
)";

/**
 * Verify the spacing is correct for row, rowReverse, column and columnReverse work with LTR layoutDirection
 */

TEST_F(LiveArrayRebuildTest, SpacedContainerRowFull) {
    loadDocument(SPACED_CONTAINER_WITH_LAYOUTDIR, "{\"containerDir\": \"row\", \"layoutDir\": \"LTR\"}");
    root->clearPending();

    auto cont = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("container"));
    auto c  = cont->getChildAt(0);
    auto c2 = cont->getChildAt(1);
    auto c3 = cont->getChildAt(2);
    auto c4 = cont->getChildAt(3);
    EXPECT_TRUE(expectBounds(c,  0, 0,   10, 100));
    EXPECT_TRUE(expectBounds(c2, 0, 130, 10, 230));
    EXPECT_TRUE(expectBounds(c3, 0, 260, 10, 360));
    EXPECT_TRUE(expectBounds(c4, 0, 390, 10, 490));
}

TEST_F(LiveArrayRebuildTest, SpacedContainerRowReverseFull) {
    loadDocument(SPACED_CONTAINER_WITH_LAYOUTDIR, "{\"containerDir\": \"rowReverse\", \"layoutDir\": \"LTR\"}");
    root->clearPending();

    auto cont = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("container"));
    auto c  = cont->getChildAt(0);
    auto c2 = cont->getChildAt(1);
    auto c3 = cont->getChildAt(2);
    auto c4 = cont->getChildAt(3);
    EXPECT_TRUE(expectBounds(c,  0, 1400, 10, 1500));
    EXPECT_TRUE(expectBounds(c2, 0, 1270, 10, 1370));
    EXPECT_TRUE(expectBounds(c3, 0, 1140, 10, 1240));
    EXPECT_TRUE(expectBounds(c4, 0, 1010, 10, 1110));
}

TEST_F(LiveArrayRebuildTest, SpacedContainerColumnFull) {
    loadDocument(SPACED_CONTAINER_WITH_LAYOUTDIR, "{\"containerDir\": \"column\", \"layoutDir\": \"LTR\"}");
    root->clearPending();

    auto cont = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("container"));
    auto c  = cont->getChildAt(0);
    auto c2 = cont->getChildAt(1);
    auto c3 = cont->getChildAt(2);
    auto c4 = cont->getChildAt(3);
    EXPECT_TRUE(expectBounds(c,  0,   0, 10, 100));
    EXPECT_TRUE(expectBounds(c2, 40,  0, 50, 100));
    EXPECT_TRUE(expectBounds(c3, 80,  0, 90, 100));
    EXPECT_TRUE(expectBounds(c4, 120, 0, 130, 100));
}

TEST_F(LiveArrayRebuildTest, SpacedContainerColumnReverseFull) {
    loadDocument(SPACED_CONTAINER_WITH_LAYOUTDIR, "{\"containerDir\": \"columnReverse\", \"layoutDir\": \"LTR\"}");
    root->clearPending();

    auto cont = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("container"));
    auto c  = cont->getChildAt(0);
    auto c2 = cont->getChildAt(1);
    auto c3 = cont->getChildAt(2);
    auto c4 = cont->getChildAt(3);
    EXPECT_TRUE(expectBounds(c,  120, 0, 130, 100));
    EXPECT_TRUE(expectBounds(c2, 80,  0,  90, 100));
    EXPECT_TRUE(expectBounds(c3, 40,  0,  50, 100));
    EXPECT_TRUE(expectBounds(c4, 0,   0,  10, 100));
}

/**
 * Verify the spacing is correct when we dynamically change layoutDirection and direction
 */
TEST_F(LiveArrayRebuildTest, SpacedContainerChangeDirection) {
    loadDocument(SPACED_CONTAINER_WITH_LAYOUTDIR, "{\"containerDir\": \"row\", \"layoutDir\": \"LTR\"}");
    root->clearPending();

    auto cont = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("container"));
    auto c  = cont->getChildAt(0);
    auto c2 = cont->getChildAt(1);
    auto c3 = cont->getChildAt(2);
    auto c4 = cont->getChildAt(3);
    EXPECT_TRUE(expectBounds(c,  0, 0,   10, 100));
    EXPECT_TRUE(expectBounds(c2, 0, 130, 10, 230));
    EXPECT_TRUE(expectBounds(c3, 0, 260, 10, 360));
    EXPECT_TRUE(expectBounds(c4, 0, 390, 10, 490));

    cont->setProperty(kPropertyDirection, "column");
    root->clearPending();

    EXPECT_TRUE(expectBounds(c,  0,   0, 10, 100));
    EXPECT_TRUE(expectBounds(c2, 40,  0, 50, 100));
    EXPECT_TRUE(expectBounds(c3, 80,  0, 90, 100));
    EXPECT_TRUE(expectBounds(c4, 120, 0, 130, 100));

    cont->setProperty(kPropertyDirection, "columnReverse");
    root->clearPending();

    EXPECT_TRUE(expectBounds(c,  120, 0, 130, 100));
    EXPECT_TRUE(expectBounds(c2, 80,  0,  90, 100));
    EXPECT_TRUE(expectBounds(c3, 40,  0,  50, 100));
    EXPECT_TRUE(expectBounds(c4, 0,   0,  10, 100));


    cont->setProperty(kPropertyDirection, "rowReverse");
    root->clearPending();

    EXPECT_TRUE(expectBounds(c,  0, 1400, 10, 1500));
    EXPECT_TRUE(expectBounds(c2, 0, 1270, 10, 1370));
    EXPECT_TRUE(expectBounds(c3, 0, 1140, 10, 1240));
    EXPECT_TRUE(expectBounds(c4, 0, 1010, 10, 1110));

    cont->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    EXPECT_TRUE(expectBounds(c,  0, 0,   10, 100));
    EXPECT_TRUE(expectBounds(c2, 0, 130, 10, 230));
    EXPECT_TRUE(expectBounds(c3, 0, 260, 10, 360));
    EXPECT_TRUE(expectBounds(c4, 0, 390, 10, 490));

    cont->setProperty(kPropertyDirection, "row");
    root->clearPending();

    EXPECT_TRUE(expectBounds(c,  0, 1400, 10, 1500));
    EXPECT_TRUE(expectBounds(c2, 0, 1270, 10, 1370));
    EXPECT_TRUE(expectBounds(c3, 0, 1140, 10, 1240));
    EXPECT_TRUE(expectBounds(c4, 0, 1010, 10, 1110));
}

/**
 * Verify the spacing is correct for row, rowReverse, column and columnReverse work with RTL layoutDirection
 */

TEST_F(LiveArrayRebuildTest, SpacedContainerRowFullRTL) {
    loadDocument(SPACED_CONTAINER_WITH_LAYOUTDIR, "{\"containerDir\": \"row\", \"layoutDir\": \"RTL\"}");
    root->clearPending();

    auto cont = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("container"));
    auto c  = cont->getChildAt(0);
    auto c2 = cont->getChildAt(1);
    auto c3 = cont->getChildAt(2);
    auto c4 = cont->getChildAt(3);
    EXPECT_TRUE(expectBounds(c,  0, 1400, 10, 1500));
    EXPECT_TRUE(expectBounds(c2, 0, 1270, 10, 1370));
    EXPECT_TRUE(expectBounds(c3, 0, 1140, 10, 1240));
    EXPECT_TRUE(expectBounds(c4, 0, 1010, 10, 1110));
}

TEST_F(LiveArrayRebuildTest, SpacedContainerRowReverseFullRTL) {
    loadDocument(SPACED_CONTAINER_WITH_LAYOUTDIR, "{\"containerDir\": \"rowReverse\", \"layoutDir\": \"RTL\"}");
    root->clearPending();

    auto cont = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("container"));
    auto c  = cont->getChildAt(0);
    auto c2 = cont->getChildAt(1);
    auto c3 = cont->getChildAt(2);
    auto c4 = cont->getChildAt(3);
    EXPECT_TRUE(expectBounds(c,  0, 0,   10, 100));
    EXPECT_TRUE(expectBounds(c2, 0, 130, 10, 230));
    EXPECT_TRUE(expectBounds(c3, 0, 260, 10, 360));
    EXPECT_TRUE(expectBounds(c4, 0, 390, 10, 490));
}

TEST_F(LiveArrayRebuildTest, SpacedContainerColumnFullRTL) {
    loadDocument(SPACED_CONTAINER_WITH_LAYOUTDIR, "{\"containerDir\": \"column\", \"layoutDir\": \"RTL\"}");
    root->clearPending();

    auto cont = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("container"));
    auto c  = cont->getChildAt(0);
    auto c2 = cont->getChildAt(1);
    auto c3 = cont->getChildAt(2);
    auto c4 = cont->getChildAt(3);
    EXPECT_TRUE(expectBounds(c,  0,   1400, 10,  1500));
    EXPECT_TRUE(expectBounds(c2, 40,  1400, 50,  1500));
    EXPECT_TRUE(expectBounds(c3, 80,  1400, 90,  1500));
    EXPECT_TRUE(expectBounds(c4, 120, 1400, 130, 1500));
}

TEST_F(LiveArrayRebuildTest, SpacedContainerColumnReverseFullRTL) {
    loadDocument(SPACED_CONTAINER_WITH_LAYOUTDIR, "{\"containerDir\": \"columnReverse\", \"layoutDir\": \"RTL\"}");
    root->clearPending();

    auto cont = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("container"));
    auto c  = cont->getChildAt(0);
    auto c2 = cont->getChildAt(1);
    auto c3 = cont->getChildAt(2);
    auto c4 = cont->getChildAt(3);
    EXPECT_TRUE(expectBounds(c,  120, 1400, 130, 1500));
    EXPECT_TRUE(expectBounds(c2, 80,  1400,  90, 1500));
    EXPECT_TRUE(expectBounds(c3, 40,  1400,  50, 1500));
    EXPECT_TRUE(expectBounds(c4, 0,   1400,  10, 1500));
}

auto SPACED_SEQUENCE_WITH_LAYOUTDIR = R"apl(
{
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "parameters": [ "scrollDir", "layoutDir" ],
    "items":{
      "layoutDirection": "LTR",
      "type": "Frame",
      "backgroundColor": "red",
      "width": "1500px",
      "items": {
        "type": "Sequence",
        "id": "sequence",
        "scrollDirection": "${scrollDir}",
        "width": "1500px",
        "items": {
          "type": "Frame",
          "id": "Frame+${index}",
          "width": "100",
          "backgroundColor": "blue",
          "spacing": 30,
          "items": {
            "type": "Text",
            "text": "* ${index}",
            "width": "100"
          }
        },
        "data": [1, 2, 3, 4]
      }
    }
  }
}
)apl";

/**
 * Verify spacing also works correctly on sequences with horizontal scrolling
 */
TEST_F(LiveArrayRebuildTest, SpacedSequenceChangeDirectionHorizontal) {
    loadDocument(SPACED_SEQUENCE_WITH_LAYOUTDIR, "{\"scrollDir\": \"horizontal\", \"layoutDir\": \"LTR\"}");
    root->clearPending();

    auto cont = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("sequence"));
    auto c  = cont->getChildAt(0);
    auto c2 = cont->getChildAt(1);
    auto c3 = cont->getChildAt(2);
    auto c4 = cont->getChildAt(3);
    EXPECT_TRUE(expectBounds(c,  0, 0,   10, 100));
    EXPECT_TRUE(expectBounds(c2, 0, 130, 10, 230));
    EXPECT_TRUE(expectBounds(c3, 0, 260, 10, 360));
    EXPECT_TRUE(expectBounds(c4, 0, 390, 10, 490));

    // Verify we respond correctly to layoutDirection changes
    cont->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    EXPECT_TRUE(expectBounds(c,  0, 1400, 10, 1500));
    EXPECT_TRUE(expectBounds(c2, 0, 1300, 10, 1400));
    EXPECT_TRUE(expectBounds(c3, 0, 1170, 10, 1270));
    EXPECT_TRUE(expectBounds(c4, 0, 1040, 10, 1140));

    cont->setProperty(kPropertyLayoutDirectionAssigned, "LTR");
    root->clearPending();

    EXPECT_TRUE(expectBounds(c,  0, 0, 10, 100));
    EXPECT_TRUE(expectBounds(c2, 0, 130, 10, 230));
    EXPECT_TRUE(expectBounds(c3, 0, 260, 10, 360));
    EXPECT_TRUE(expectBounds(c4, 0, 390, 10, 490));
}

/**
 * Verify spacing also works correctly on sequences with vertical scrolling
 */
TEST_F(LiveArrayRebuildTest, SpacedSequenceChangeDirectionVertical) {
    loadDocument(SPACED_SEQUENCE_WITH_LAYOUTDIR, "{\"scrollDir\": \"vertical\", \"layoutDir\": \"LTR\"}");
    root->clearPending();

    auto cont = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("sequence"));
    auto c  = cont->getChildAt(0);
    auto c2 = cont->getChildAt(1);
    auto c3 = cont->getChildAt(2);
    auto c4 = cont->getChildAt(3);
    EXPECT_TRUE(expectBounds(c,  0,   0, 10,  100));
    EXPECT_TRUE(expectBounds(c2, 40,  0, 50,  100));
    EXPECT_TRUE(expectBounds(c3, 80,  0, 90,  100));
    EXPECT_TRUE(expectBounds(c4, 120, 0, 130, 100));

    // Verify we respond correctly to layoutDirection changes
    cont->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    EXPECT_TRUE(expectBounds(c,  0,   1400,  10, 1500));
    EXPECT_TRUE(expectBounds(c2, 40,  1400,  50, 1500));
    EXPECT_TRUE(expectBounds(c3, 80,  1400,  90, 1500));
    EXPECT_TRUE(expectBounds(c4, 120, 1400, 130, 1500));

    cont->setProperty(kPropertyLayoutDirectionAssigned, "LTR");
    root->clearPending();

    EXPECT_TRUE(expectBounds(c,    0, 0, 10,  100));
    EXPECT_TRUE(expectBounds(c2,  40, 0, 50,  100));
    EXPECT_TRUE(expectBounds(c3,  80, 0, 90,  100));
    EXPECT_TRUE(expectBounds(c4, 120, 0, 130, 100));
}

} // namespace apl
