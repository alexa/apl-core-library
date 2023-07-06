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

#include "apl/component/textcomponent.h"

#include "../testeventloop.h"

using namespace apl;

static const char *DEFAULT_INSERT = R"(
    {
      "type": "Text",
      "id": "newArrival",
      "text": "I have arrived!"
    })";

class CommandInsertItemTest : public CommandTest {
public:
    ActionPtr
    executeInsertItem(const std::string& componentId, const int index = 0, const std::string& item = DEFAULT_INSERT) {
        std::map<std::string, Object> properties = {};

        rapidjson::Document itemDoc;
        itemDoc.Parse(item.c_str());

        properties.emplace("componentId", componentId);
        properties.emplace("item", itemDoc);

        if (index != INT_MAX)
            properties.emplace("at", index);

        return executeCommand("InsertItem", properties, false);
    }

    void
    validateInsert(
        const CoreComponentPtr& target,
        const CoreComponentPtr& child,
        const int initialChildCount,
        const int expectedIndex) {

        ASSERT_FALSE(session->checkAndClear());
        ASSERT_TRUE(root->isDirty());
        ASSERT_EQ(target->getChildCount(), initialChildCount + 1);

        ASSERT_TRUE(child);
        ASSERT_EQ(target->getChildIndex(child), expectedIndex);
        ASSERT_TRUE(CheckDirtyAtLeast(root, target, child));
        ASSERT_EQ(child->getParent()->getId(), target->getId());
        ASSERT_EQ(child->getPathObject().toString(), "_virtual");
        ASSERT_EQ(child->getContext()->parent(), target->getContext());
    }

    void
    validateNonInsert(
        const std::string& expectedSessionMessage,
        const CoreComponentPtr& target = nullptr,
        const int expectedChildCount = -1, // expected to pass non-negative value "if target"
        const std::string& missingChild = "newArrival") {

        if (target) {
            ASSERT_EQ(target->getChildCount(), expectedChildCount);
        }

        ASSERT_TRUE(session->checkAndClear(expectedSessionMessage));
        ASSERT_FALSE(root->isDirty());
        ASSERT_FALSE(root->findComponentById(missingChild));
    }
};

static const char *INSERT_ITEM = R"(
    {
      "type": "APL",
      "version": "2023.1",
      "mainTemplate": {
        "parameters": [],
        "item": {
          "id": "main",
          "type": "Container",
          "items": [
            {
              "type": "Text",
              "id": "cannotHaveChildren",
              "text": "Hello, World!"
            },
            {
              "type": "Frame",
              "id": "hasNoChildren",
              "bind": [
                { "name": "Color", "value": "blue" }
              ]
            },
            {
              "type": "Frame",
              "id": "alreadyHasAChild",
              "item": {
                "type": "Text",
                "id": "onlyChild",
                "text": "There can only be one!"
              }
            },
            {
              "type": "Container",
              "id": "multiChild",
              "firstItem": {
                "type": "Text",
                "id": "firstChild",
                "text": "The Original"
              },
              "items":[{
                "type": "Text",
                "id": "middleChild",
                "text": "Definitive Edition"
              }],
              "lastItem": {
                "type": "Text",
                "id": "lastChild",
                "text": "The Remix"
              }
            }
          ]
        }
      }
    })";

TEST_F(CommandInsertItemTest, InsertItemWhenComponentIdMissing)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    executeCommand(
        "InsertItem",
        {
            { "at", 0 },
            // missing componentId property
            { "item", INSERT_ITEM }},
        false);

    validateNonInsert("Missing required property 'componentId' for InsertItem");
}

TEST_F(CommandInsertItemTest, InsertItemWhenTargetDoesNotExist)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    auto target = root->findComponentById("missingTargetComponent");
    ASSERT_FALSE(target);

    executeInsertItem("missingTargetComponent");

    validateNonInsert( "Illegal command InsertItem - need to specify a target componentId");
}

TEST_F(CommandInsertItemTest, InsertInvalidItem)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    auto target = CoreComponent::cast(root->findComponentById("hasNoChildren"));
    ASSERT_TRUE(target);
    const int initialChildCount = (int) target->getChildCount();
    ASSERT_EQ(initialChildCount, 0);
    ASSERT_TRUE(target->canInsertChild());

    executeInsertItem(
        "hasNoChildren",
        0,
        // The json below cannot be inflated as a Component because it is missing the "type" property
        R"({"id":"newArrival","text":"I have arrived!"})");

    validateNonInsert("Could not inflate item to be inserted", target, initialChildCount);
}

TEST_F(CommandInsertItemTest, InsertItemWithFalseWhenClause)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    auto target = CoreComponent::cast(root->findComponentById("hasNoChildren"));
    ASSERT_TRUE(target);
    const int initialChildCount = (int) target->getChildCount();
    ASSERT_EQ(initialChildCount, 0);
    ASSERT_TRUE(target->canInsertChild());

    // when evaluates to false
    executeInsertItem(
        "hasNoChildren",
        0,
        R"({
            "when": "${viewport.shape == 'round'}",
            "type": "Text",
            "id": "newArrival",
            "text": "I have arrived!"
        })");

    validateNonInsert("Could not inflate item to be inserted", target, initialChildCount);
}

TEST_F(CommandInsertItemTest, InsertItemSkippingFalseWhenClause)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    auto target = CoreComponent::cast(root->findComponentById("hasNoChildren"));
    ASSERT_TRUE(target);
    const int initialChildCount = (int) target->getChildCount();
    ASSERT_EQ(initialChildCount, 0);
    ASSERT_TRUE(target->canInsertChild());

    executeInsertItem(
        "hasNoChildren",
        0,
        R"([{
            "when": "${viewport.shape == 'round'}",
            "type": "Text",
            "id": "whenIsFalse",
            "text": "I won't inflate"
          },
          {
            "type": "Text",
            "id": "newArrival",
            "text": "I have arrived!"
          },
          {
             "type": "Text",
             "id": "unreachable",
             "text": "I never had a chance!"
        }])");

    auto child = CoreComponent::cast(root->findComponentById("newArrival"));
    validateInsert(target, child, initialChildCount, 0);
    ASSERT_FALSE(root->findComponentById("whenIsFalse"));
    ASSERT_FALSE(root->findComponentById("unreachable"));
}

TEST_F(CommandInsertItemTest, InsertItemWhenTargetCannotHaveChildren)
{
   loadDocument(INSERT_ITEM);
   root->clearPending();

   auto target = CoreComponent::cast(root->findComponentById("cannotHaveChildren"));
   ASSERT_TRUE(target);
   const int initialChildCount = (int) target->getChildCount();
   ASSERT_EQ(initialChildCount, 0);
   ASSERT_FALSE(target->canInsertChild());

   executeInsertItem("cannotHaveChildren");

   validateNonInsert("Could not insert child into 'cannotHaveChildren'", target, initialChildCount);
}

TEST_F(CommandInsertItemTest, InsertItemWhenTargetAlreadyHasOnlyChild)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    auto target = CoreComponent::cast(root->findComponentById("alreadyHasAChild"));
    ASSERT_TRUE(target);
    const int initialChildCount = (int) target->getChildCount();
    ASSERT_EQ(initialChildCount, 1);
    ASSERT_FALSE(target->canInsertChild());

    executeInsertItem("alreadyHasAChild");

    validateNonInsert("Could not insert child into 'alreadyHasAChild'", target, initialChildCount);
}

TEST_F(CommandInsertItemTest, InsertItem)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    auto target = CoreComponent::cast(root->findComponentById("hasNoChildren"));
    ASSERT_TRUE(target);
    const int initialChildCount = (int) target->getChildCount();
    ASSERT_EQ(initialChildCount, 0);
    ASSERT_TRUE(target->canInsertChild());

    executeInsertItem(
        "hasNoChildren",
        0,
        R"({
            "type": "Text",
            "id": "newArrival",
            "text": "${Color}"
        })");

    auto child = CoreComponent::cast(root->findComponentById("newArrival"));
    validateInsert(target, child, initialChildCount, 0);
    ASSERT_EQ(TextComponent::cast(child)->getValue().asString(), "blue");
}

TEST_F(CommandInsertItemTest, InsertItems)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    auto target = CoreComponent::cast(root->findComponentById("hasNoChildren"));
    ASSERT_TRUE(target);
    const int initialChildCount = (int) target->getChildCount();
    ASSERT_EQ(initialChildCount, 0);
    ASSERT_TRUE(target->canInsertChild());

    rapidjson::Document itemDoc;
    itemDoc.Parse(DEFAULT_INSERT);

    executeCommand(
        "InsertItem",
        {
            {"at", 0},
            {"componentId", "hasNoChildren"},
            {"items", itemDoc}}, // "items" instead of "item"
        false);

    auto child = CoreComponent::cast(root->findComponentById("newArrival"));
    validateInsert(target, child, initialChildCount, 0);
}

TEST_F(CommandInsertItemTest, InsertItemDefaultAtAppends)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    auto target = CoreComponent::cast(root->findComponentById("main"));
    ASSERT_TRUE(target);
    const int initialChildCount = (int) target->getChildCount();
    ASSERT_TRUE(initialChildCount > 0);
    ASSERT_TRUE(target->canInsertChild());

    executeInsertItem("main",INT_MAX);

    auto child = CoreComponent::cast(root->findComponentById("newArrival"));
    validateInsert(target, child, initialChildCount, initialChildCount);
}

TEST_F(CommandInsertItemTest, InsertItemNegativeInsertsFromEnd)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    auto target = CoreComponent::cast(root->findComponentById("multiChild"));
    ASSERT_TRUE(target);
    const int initialChildCount = (int) target->getChildCount();
    ASSERT_TRUE(initialChildCount > 1);
    ASSERT_TRUE(target->canInsertChild());

    executeInsertItem("multiChild",-1);

    auto child = CoreComponent::cast(root->findComponentById("newArrival"));
    validateInsert(target, child, initialChildCount, initialChildCount - 1);
}

TEST_F(CommandInsertItemTest, InsertItemNegativeWalksOffLeftEnd)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    auto target = CoreComponent::cast(root->findComponentById("multiChild"));
    ASSERT_TRUE(target);
    const int initialChildCount = (int) target->getChildCount();
    ASSERT_TRUE(initialChildCount > 1);
    ASSERT_TRUE(target->canInsertChild());

    executeInsertItem("multiChild", -1 * (initialChildCount + 1));

    auto child = CoreComponent::cast(root->findComponentById("newArrival"));
    validateInsert(target, child, initialChildCount, 0);
}

TEST_F(CommandInsertItemTest, InsertItemMultiChildZeroIsBeforeFirstItem)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    auto target = CoreComponent::cast(root->findComponentById("multiChild"));
    ASSERT_TRUE(target);
    const int initialChildCount = (int) target->getChildCount();
    ASSERT_TRUE(initialChildCount > 1);
    ASSERT_TRUE(target->canInsertChild());

    executeInsertItem("multiChild");

    auto child = CoreComponent::cast(root->findComponentById("newArrival"));
    validateInsert(target, child, initialChildCount, 0);
    ASSERT_EQ(target->getChildAt(1)->getId(), "firstChild");
}

TEST_F(CommandInsertItemTest, InsertItemMultiChildAppendIsAfterLastItem)
{
    loadDocument(INSERT_ITEM);
    root->clearPending();

    auto target = CoreComponent::cast(root->findComponentById("multiChild"));
    ASSERT_TRUE(target);
    const int initialChildCount = (int) target->getChildCount();
    ASSERT_TRUE(initialChildCount > 1);
    ASSERT_TRUE(target->canInsertChild());

    executeInsertItem("multiChild", INT_MAX);

    auto child = CoreComponent::cast(root->findComponentById("newArrival"));
    validateInsert(target, child, initialChildCount, initialChildCount);
    ASSERT_EQ(target->getChildAt(initialChildCount - 1)->getId(), "lastChild");
}

TEST_F(CommandInsertItemTest, InsertItemWhenTargetUsesArrayDataInflation)
{
    loadDocument(
        R"(
          {
            "type": "APL",
            "version": "2023.1",
            "mainTemplate": {
              "parameters": [],
              "item": {
                "id": "main",
                "type": "Container",
                "item": {
                  "type": "Text",
                  "text": "${index+1}. ${data}"
                },
                "data": [
                  "Some data",
                  "Some other data"
                ]
              }
            }
          })");
    root->clearPending();

    auto target = CoreComponent::cast(root->findComponentById("main"));
    ASSERT_TRUE(target);
    const int initialChildCount = (int) target->getChildCount();
    ASSERT_TRUE(initialChildCount > 0);
    ASSERT_FALSE(target->canInsertChild());

    executeInsertItem("main");

    validateNonInsert("Could not insert child into 'main'", target, initialChildCount);
}