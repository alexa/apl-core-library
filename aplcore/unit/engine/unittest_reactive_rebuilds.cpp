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

class ReactiveRebuilds : public DocumentWrapper {};

inline
   ::testing::AssertionResult
   VerifyChild(const CoreComponentPtr& component, int index, uint32_t expectedColor, int expectedIndex, int expectedOrdinal = -1) {
   auto color = component->getChildAt(index)->getCalculated(kPropertyBackground).getColor();
   if (color != expectedColor)
       return ::testing::AssertionFailure() << "Color mismatch: " << color << " != " << expectedColor;

   auto actualIndex = component->getCoreChildAt(index)->getContext()->opt("index").asInt();
   if (expectedIndex >= 0 && actualIndex != expectedIndex)
       return ::testing::AssertionFailure() << "Index mismatch: " << actualIndex << " != " << expectedIndex;

   auto actualOrdinal = component->getCoreChildAt(index)->getContext()->opt("ordinal").asInt();
   if (expectedOrdinal >= 0 && actualOrdinal != expectedOrdinal)
       return ::testing::AssertionFailure() << "Ordinal mismatch: " << actualOrdinal << " != " << expectedOrdinal;

   return ::testing::AssertionSuccess();
}

static const char *DYNAMIC_CONDITIONAL_ITEMS = R"({
 "type": "APL",
 "version": "2024.2",
 "settings": {
   "-experimentalIsReactive": true
 },
 "mainTemplate": {
   "parameters": ["Item0", "Item1", "Item2", "Item3", "Item4"],
   "item": {
     "type": "Container",
     "height": "100%",
     "width": "100%",
     "bind": [
       { "name": "SelectorItem0", "value": "${Item0}" },
       { "name": "SelectorItem1", "value": "${Item1}" },
       { "name": "SelectorItem2", "value": "${Item2}" },
       { "name": "SelectorItem3", "value": "${Item3}" },
       { "name": "SelectorItem4", "value": "${Item4}" }
     ],
     "numbered": true,
     "items": [
       {
         "when": "${SelectorItem0 == 1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${SelectorItem0 == 2}",
         "type": "Frame",
         "background": "green"
       },
       {
         "when": "${SelectorItem1 == 1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${SelectorItem2 == 1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${SelectorItem3 == 1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${SelectorItem3 == 2}",
         "type": "Frame",
         "background": "green"
       },
       {
         "when": "${SelectorItem4 == 1}",
         "type": "Frame",
         "background": "red"
       }
     ]
   }
 }
})";

TEST_F(ReactiveRebuilds, DynamicConditionalItems) {
   loadDocument(DYNAMIC_CONDITIONAL_ITEMS, R"({
       "Item0": 1,
       "Item1": 1,
       "Item2": 0,
       "Item3": 2,
       "Item4": 1
   })");

   auto initialDependentCount = getAliveCountersFor("Dependant");
   auto initialContextCount = getAliveCountersFor("Context");

   ASSERT_TRUE(component);
   ASSERT_EQ(4, component->getChildCount());

   ASSERT_TRUE(VerifyChild(component, 0, 0xFF0000FF, 0, 1));
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 1, 2));
   ASSERT_TRUE(VerifyChild(component, 2, 0x008000FF, 2, 3));
   ASSERT_TRUE(VerifyChild(component, 3, 0xFF0000FF, 3, 4));

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem0" },
           { "value", 2 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem3" },
           { "value", 1 }
       },
       false);
   advanceTime(1);

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(4, component->getChildCount());

   clearDirty();
   root->clearVisualContextDirty();

   ASSERT_TRUE(VerifyChild(component, 0, 0x008000FF, 0, 1));
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 1, 2));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 2, 3));
   ASSERT_TRUE(VerifyChild(component, 3, 0xFF0000FF, 3, 4));

   initialDependentCount -= 2;// Two links to params broken
   initialContextCount += 1; // Command action ctx

   ASSERT_TRUE(CheckAliveCountersNotChanged("Dependant", initialDependentCount));
   ASSERT_TRUE(CheckAliveCountersNotChanged("Context", initialContextCount));

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem0" },
           { "value", 1 }
       },
       false);

   advanceTime(1);

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(4, component->getChildCount());

   clearDirty();
   root->clearVisualContextDirty();

   ASSERT_TRUE(VerifyChild(component, 0, 0xFF0000FF, 0, 1));
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 1, 2));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 2, 3));
   ASSERT_TRUE(VerifyChild(component, 3, 0xFF0000FF, 3, 4));

   ASSERT_TRUE(CheckAliveCountersNotChanged("Dependant", initialDependentCount));
   ASSERT_TRUE(CheckAliveCountersNotChanged("Context", initialContextCount));
}

TEST_F(ReactiveRebuilds, DynamicConditionalItemsDissapear) {
   loadDocument(DYNAMIC_CONDITIONAL_ITEMS, R"({
       "Item0": 1,
       "Item1": 1,
       "Item2": 0,
       "Item3": 2,
       "Item4": 1
   })");
   ASSERT_TRUE(component);
   ASSERT_EQ(4, component->getChildCount());

   ASSERT_TRUE(VerifyChild(component, 0, 0xFF0000FF, 0, 1));
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 1, 2));
   ASSERT_TRUE(VerifyChild(component, 2, 0x008000FF, 2, 3));
   ASSERT_TRUE(VerifyChild(component, 3, 0xFF0000FF, 3, 4));

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem0" },
           { "value", 0 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem3" },
           { "value", 0 }
       },
       false);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(2, component->getChildCount());

   ASSERT_TRUE(VerifyChild(component, 0, 0xFF0000FF, 0, 1));
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 1, 2));
}

TEST_F(ReactiveRebuilds, DynamicConditionalItemsAppear) {
   loadDocument(DYNAMIC_CONDITIONAL_ITEMS, R"({
       "Item0": 0,
       "Item1": 1,
       "Item2": 0,
       "Item3": 2,
       "Item4": 1
   })");
   ASSERT_TRUE(component);
   ASSERT_EQ(3, component->getChildCount());

   ASSERT_TRUE(VerifyChild(component, 0, 0xFF0000FF, 0, 1));
   ASSERT_TRUE(VerifyChild(component, 1, 0x008000FF, 1, 2));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 2, 3));

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem0" },
           { "value", 2 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem2" },
           { "value", 1 }
       },
       false);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(5, component->getChildCount());

   ASSERT_TRUE(VerifyChild(component, 0, 0x008000FF, 0, 1));
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 1, 2));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 2, 3));
   ASSERT_TRUE(VerifyChild(component, 3, 0x008000FF, 3, 4));
   ASSERT_TRUE(VerifyChild(component, 4, 0xFF0000FF, 4, 5));
}

static const char *DYNAMIC_CONDITIONAL_FIRST_LAST = R"({
 "type": "APL",
 "version": "2024.2",
 "settings": {
   "-experimentalIsReactive": true
 },
 "mainTemplate": {
   "parameters": [ "First", "Item0", "Item1", "Item2", "Item3", "Item4", "Last" ],
   "item": {
     "type": "Container",
     "height": "100%",
     "width": "100%",
     "bind": [
       { "name": "SelectorFirst", "value": "${First}" },
       { "name": "SelectorItem0", "value": "${Item0}" },
       { "name": "SelectorItem1", "value": "${Item1}" },
       { "name": "SelectorItem2", "value": "${Item2}" },
       { "name": "SelectorItem3", "value": "${Item3}" },
       { "name": "SelectorItem4", "value": "${Item4}" },
       { "name": "SelectorLast", "value": "${Last}" }
     ],
     "firstItem": [
       {
         "when": "${SelectorFirst == 1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${SelectorFirst == 2}",
         "type": "Frame",
         "background": "green"
       }
     ],
     "items": [
       {
         "when": "${SelectorItem0 == 1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${SelectorItem0 == 2}",
         "type": "Frame",
         "background": "green"
       },
       {
         "when": "${SelectorItem1 == 1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${SelectorItem2 == 1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${SelectorItem3 == 1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${SelectorItem3 == 2}",
         "type": "Frame",
         "background": "green"
       },
       {
         "when": "${SelectorItem4 == 1}",
         "type": "Frame",
         "background": "red"
       }
     ],
     "lastItem": [
       {
         "when": "${SelectorLast == 1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${SelectorLast == 2}",
         "type": "Frame",
         "background": "green"
       }
     ]
   }
 }
})";

TEST_F(ReactiveRebuilds, DynamicConditionalFirstLast) {
   loadDocument(DYNAMIC_CONDITIONAL_FIRST_LAST, R"({
       "First": 1,
       "Item0": 1,
       "Item1": 0,
       "Item2": 0,
       "Item3": 1,
       "Item4": 0,
       "Last": 1
   })");

   auto initialDependentCount = getAliveCountersFor("Dependant");
   auto initialContextCount = getAliveCountersFor("Context");

   ASSERT_TRUE(component);
   ASSERT_EQ(4, component->getChildCount());

   ASSERT_EQ(0xFF0000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 1));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem0" },
           { "value", 2 }
       },
       false);
   advanceTime(1);

   initialDependentCount -= 1;// One link to params broken
   initialContextCount += 1; // Command action ctx

   ASSERT_TRUE(CheckAliveCountersNotChanged("Dependant", initialDependentCount));
   ASSERT_TRUE(CheckAliveCountersNotChanged("Context", initialContextCount));

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0x008000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 1));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorFirst" },
           { "value", 2 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorLast" },
           { "value", 2 }
       },
       false);
   advanceTime(1);

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   clearDirty();
   root->clearVisualContextDirty();

   ASSERT_EQ(0x008000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_EQ(0x008000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());

   initialDependentCount -= 2;// Two links to params broken

   ASSERT_TRUE(CheckAliveCountersNotChanged("Dependant", initialDependentCount));
   ASSERT_TRUE(CheckAliveCountersNotChanged("Context", initialContextCount));

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorFirst" },
           { "value", 1 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorLast" },
           { "value", 1 }
       },
       false);
   advanceTime(1);

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   clearDirty();
   root->clearVisualContextDirty();

   ASSERT_EQ(0xFF0000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_EQ(0xFF0000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());

   ASSERT_TRUE(CheckAliveCountersNotChanged("Dependant", initialDependentCount));
   ASSERT_TRUE(CheckAliveCountersNotChanged("Context", initialContextCount));
}

TEST_F(ReactiveRebuilds, DynamicConditionalFirstLastDisappear) {
   loadDocument(DYNAMIC_CONDITIONAL_FIRST_LAST, R"({
       "First": 1,
       "Item0": 1,
       "Item1": 1,
       "Item2": 0,
       "Item3": 2,
       "Item4": 0,
       "Last": 1
   })");
   ASSERT_TRUE(component);
   ASSERT_EQ(5, component->getChildCount());

   ASSERT_EQ(0xFF0000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 1));
   ASSERT_TRUE(VerifyChild(component, 3, 0x008000FF, 2));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(4)->getCalculated(kPropertyBackground).getColor());

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem1" },
           { "value", 0 }
       },
       false);
   advanceTime(1);

   ASSERT_EQ(4, component->getChildCount());
   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0x008000FF, 1));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorFirst" },
           { "value", 0 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorLast" },
           { "value", 0 }
       },
       false);
   advanceTime(1);

   ASSERT_EQ(2, component->getChildCount());
   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_TRUE(VerifyChild(component, 0, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 1, 0x008000FF, 1));
}

TEST_F(ReactiveRebuilds, DynamicConditionalFirstLastAppear) {
   loadDocument(DYNAMIC_CONDITIONAL_FIRST_LAST, R"({
       "First": 0,
       "Item0": 0,
       "Item1": 0,
       "Item2": 0,
       "Item3": 2,
       "Item4": 0,
       "Last": 0
   })");
   ASSERT_TRUE(component);
   ASSERT_EQ(1, component->getChildCount());
   ASSERT_TRUE(VerifyChild(component, 0, 0x008000FF, 0));

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem2" },
           { "value", 1 }
       },
       false);
   advanceTime(1);

   ASSERT_EQ(2, component->getChildCount());
   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_TRUE(VerifyChild(component, 0, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 1, 0x008000FF, 1));

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorFirst" },
           { "value", 1 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorLast" },
           { "value", 2 }
       },
       false);
   advanceTime(1);

   ASSERT_EQ(4, component->getChildCount());
   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0x008000FF, 1));
   ASSERT_EQ(0x008000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());
}

static const char *DYNAMIC_CONDITIONAL_LAYOUT_FIRST_LAST = R"({
 "type": "APL",
 "version": "2024.2",
 "settings": {
   "-experimentalIsReactive": true
 },
 "layouts": {
   "Semaphore": {
     "parameters": [
       {
         "name": "Selector",
         "type": "number"
       }
     ],
     "items": [
       {
         "when": "${Selector == 1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${Selector == 2}",
         "type": "Frame",
         "background": "green"
       }
     ]
   }
 },
 "mainTemplate": {
   "parameters": [ "First", "Item0", "Item1", "Item2", "Item3", "Item4", "Last" ],
   "item": {
     "type": "Container",
     "height": "100%",
     "width": "100%",
     "bind": [
       { "name": "SelectorFirst", "value": "${First}" },
       { "name": "SelectorItem0", "value": "${Item0}" },
       { "name": "SelectorItem1", "value": "${Item1}" },
       { "name": "SelectorItem2", "value": "${Item2}" },
       { "name": "SelectorItem3", "value": "${Item3}" },
       { "name": "SelectorItem4", "value": "${Item4}" },
       { "name": "SelectorLast", "value": "${Last}" }
     ],
     "firstItem": [
       {
         "type": "Semaphore",
         "Selector": "${SelectorFirst}"
       }
     ],
     "items": [
       {
         "type": "Semaphore",
         "Selector": "${SelectorItem0}"
       },
       {
         "type": "Semaphore",
         "Selector": "${SelectorItem1}"
       },
       {
         "type": "Semaphore",
         "Selector": "${SelectorItem2}"
       },
       {
         "type": "Semaphore",
         "Selector": "${SelectorItem3}"
       },
       {
         "type": "Semaphore",
         "Selector": "${SelectorItem4}"
       }
     ],
     "lastItem": [
       {
         "type": "Semaphore",
         "Selector": "${SelectorLast}"
       }
     ]
   }
 }
})";

TEST_F(ReactiveRebuilds, DynamicConditionalLayoutsFirstLast) {
   loadDocument(DYNAMIC_CONDITIONAL_LAYOUT_FIRST_LAST, R"({
       "First": 1,
       "Item0": 1,
       "Item1": 0,
       "Item2": 0,
       "Item3": 1,
       "Item4": 0,
       "Last": 1
   })");
   ASSERT_TRUE(component);
   ASSERT_EQ(4, component->getChildCount());

   ASSERT_EQ(0xFF0000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 1));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem0" },
           { "value", 2 }
       },
       false);
   advanceTime(1);

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0x008000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 1));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorFirst" },
           { "value", 2 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorLast" },
           { "value", 2 }
       },
       false);
   advanceTime(1);

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(0x008000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0x008000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 1));
   ASSERT_EQ(0x008000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());
}

TEST_F(ReactiveRebuilds, DynamicConditionalLayoutsDisappear) {
   loadDocument(DYNAMIC_CONDITIONAL_LAYOUT_FIRST_LAST, R"({
       "First": 1,
       "Item0": 1,
       "Item1": 0,
       "Item2": 0,
       "Item3": 1,
       "Item4": 0,
       "Last": 1
   })");
   ASSERT_TRUE(component);
   ASSERT_EQ(4, component->getChildCount());

   ASSERT_EQ(0xFF0000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 1));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem0" },
           { "value", 0 }
       },
       false);
   advanceTime(1);

   ASSERT_EQ(3, component->getChildCount());
   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 0));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(2)->getCalculated(kPropertyBackground).getColor());

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorFirst" },
           { "value", 0 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorLast" },
           { "value", 0 }
       },
       false);
   advanceTime(1);

   ASSERT_EQ(1, component->getChildCount());
   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_TRUE(VerifyChild(component, 0, 0xFF0000FF, 0));
}

TEST_F(ReactiveRebuilds, DynamicConditionalLayoutsAppear) {
   loadDocument(DYNAMIC_CONDITIONAL_LAYOUT_FIRST_LAST, R"({
       "First": 0,
       "Item0": 0,
       "Item1": 0,
       "Item2": 0,
       "Item3": 1,
       "Item4": 0,
       "Last": 0
   })");
   ASSERT_TRUE(component);
   ASSERT_EQ(1, component->getChildCount());

   ASSERT_TRUE(VerifyChild(component, 0, 0xFF0000FF, 0));

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem0" },
           { "value", 1 }
       },
       false);
   advanceTime(1);

   ASSERT_EQ(2, component->getChildCount());
   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_TRUE(VerifyChild(component, 0, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 1));

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorFirst" },
           { "value", 1 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorLast" },
           { "value", 1 }
       },
       false);
   advanceTime(1);

   ASSERT_EQ(4, component->getChildCount());
   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 1));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());
}

static const char *DYNAMIC_CONDITIONAL_DEEP_LAYOUT = R"({
 "type": "APL",
 "version": "2024.2",
 "settings": {
   "-experimentalIsReactive": true
 },
 "layouts": {
   "SemaphoreDeepNegative": {
     "parameters": [
       {
         "name": "SelectorDeep",
         "type": "number"
       }
     ],
     "items": [
       {
         "when": "${SelectorDeep == -1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${SelectorDeep == -2}",
         "type": "Frame",
         "background": "green"
       }
     ]
   },
   "SemaphoreDeepPositive": {
     "parameters": [
       {
         "name": "SelectorDeep",
         "type": "number"
       }
     ],
     "items": [
       {
         "when": "${SelectorDeep == 1}",
         "type": "Frame",
         "background": "blue"
       },
       {
         "when": "${SelectorDeep == 2}",
         "type": "Frame",
         "background": "yellow"
       }
     ]
   },
   "Semaphore": {
     "parameters": [
       {
         "name": "Selector",
         "type": "number"
       }
     ],
     "items": [
       {
         "when": "${Selector < 0}",
         "type": "SemaphoreDeepNegative",
         "SelectorDeep": "${Selector}"
       },
       {
         "when": "${Selector > 0}",
         "type": "SemaphoreDeepPositive",
         "SelectorDeep": "${Selector}"
       }
     ]
   }
 },
 "mainTemplate": {
   "parameters": [ "First", "Item0", "Item1", "Last" ],
   "item": {
     "type": "Container",
     "height": "100%",
     "width": "100%",
     "bind": [
       { "name": "SelectorFirst", "value": "${First}" },
       { "name": "Selector0", "value": "${Item0}" },
       { "name": "Selector1", "value": "${Item1}" },
       { "name": "SelectorLast", "value": "${Last}" }
     ],
     "firstItem": [
       {
         "type": "Semaphore",
         "Selector": "${SelectorFirst}"
       }
     ],
     "items": [
       {
         "type": "Semaphore",
         "Selector": "${Selector0}"
       },
       {
         "type": "Semaphore",
         "Selector": "${Selector1}"
       }
     ],
     "lastItem": [
       {
         "type": "Semaphore",
         "Selector": "${SelectorLast}"
       }
     ]
   }
 }
})";

TEST_F(ReactiveRebuilds, DynamicConditionalDeepLayout) {
   loadDocument(DYNAMIC_CONDITIONAL_DEEP_LAYOUT, R"({
       "First": -1,
       "Item0": -1,
       "Item1": -1,
       "Last": -1
   })");
   ASSERT_TRUE(component);
   ASSERT_EQ(0xFF0000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 1));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());

   auto initialDependentCount = getAliveCountersFor("Dependant");

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorFirst" },
           { "value", -2 }
       },
       false);
   advanceTime(1);

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(0x008000FF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 1));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());

   clearDirty();
   root->clearVisualContextDirty();
   initialDependentCount -= 1;
   ASSERT_TRUE(CheckAliveCountersNotChanged("Dependant", initialDependentCount));


   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorFirst" },
           { "value", 1 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "Item0" },
           { "value", 1 }
       },
       false);
   advanceTime(1);

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(0x0000FFFF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0x0000FFFF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 1));
   ASSERT_EQ(0xFF0000FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());

   clearDirty();
   root->clearVisualContextDirty();
   ASSERT_TRUE(CheckAliveCountersNotChanged("Dependant", initialDependentCount));

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorLast" },
           { "value", 2 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "Item1" },
           { "value", 2 }
       },
       false);
   advanceTime(1);

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_EQ(0x0000FFFF, component->getChildAt(0)->getCalculated(kPropertyBackground).getColor());
   ASSERT_TRUE(VerifyChild(component, 1, 0x0000FFFF, 0));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFFFF00FF, 1));
   ASSERT_EQ(0xFFFF00FF, component->getChildAt(3)->getCalculated(kPropertyBackground).getColor());

   clearDirty();
   root->clearVisualContextDirty();
   initialDependentCount -= 1;
   ASSERT_TRUE(CheckAliveCountersNotChanged("Dependant", initialDependentCount));
}

static const char *SEMI_DYNAMIC_CONTAINER = R"({
 "type": "APL",
 "version": "2024.2",
 "settings": {
   "-experimentalIsReactive": true
 },
 "layouts": {
   "Semaphore": {
     "parameters": [
       {
         "name": "Selector",
         "type": "number"
       }
     ],
     "items": [
       {
         "when": "${Selector == 1}",
         "type": "Frame",
         "background": "red"
       },
       {
         "when": "${Selector >= 2}",
         "type": "Frame",
         "background": "green"
       }
     ]
   }
 },
 "mainTemplate": {
   "parameters": [ "Item0", "Item1" ],
   "item": {
     "type": "Container",
     "height": "100%",
     "width": "100%",
     "bind": [
       { "name": "SelectorItem0", "value": "${Item0}" },
       { "name": "SelectorItem1", "value": "${Item1}" }
     ],
     "items": [
       {
         "type": "Semaphore",
         "Selector": "${SelectorItem0}"
       },
       {
         "type": "Semaphore",
         "Selector": "${SelectorItem1}"
       },
       {
         "type": "Semaphore",
         "Selector": 1
       },
       {
         "type": "Semaphore",
         "Selector": 1
       },
       {
         "type": "Semaphore",
         "Selector": 1
       }
     ]
   }
 }
})";

TEST_F(ReactiveRebuilds, SemiDynamicOptimizationNoChange) {
   loadDocument(SEMI_DYNAMIC_CONTAINER, R"({
       "Item0": 2,
       "Item1": 2
   })");

   ASSERT_TRUE(component);
   ASSERT_EQ(5, component->getChildCount());

   ASSERT_TRUE(VerifyChild(component, 0, 0x008000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 1, 0x008000FF, 1));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 2));
   ASSERT_TRUE(VerifyChild(component, 3, 0xFF0000FF, 3));
   ASSERT_TRUE(VerifyChild(component, 4, 0xFF0000FF, 4));

   auto child0 = component->getChildAt(0);
   auto child1 = component->getChildAt(1);
   auto child2 = component->getChildAt(2);
   auto child3 = component->getChildAt(3);
   auto child4 = component->getChildAt(4);

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem1" },
           { "value", 3 }
       },
       false);
   advanceTime(1);

   ASSERT_FALSE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_TRUE(VerifyChild(component, 0, 0x008000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 1, 0x008000FF, 1));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 2));
   ASSERT_TRUE(VerifyChild(component, 3, 0xFF0000FF, 3));
   ASSERT_TRUE(VerifyChild(component, 4, 0xFF0000FF, 4));

   ASSERT_EQ(child0, component->getChildAt(0));
   ASSERT_EQ(child1, component->getChildAt(1));
   ASSERT_EQ(child2, component->getChildAt(2));
   ASSERT_EQ(child3, component->getChildAt(3));
   ASSERT_EQ(child4, component->getChildAt(4));
}

TEST_F(ReactiveRebuilds, SemiDynamicOptimizationReorder) {
   loadDocument(SEMI_DYNAMIC_CONTAINER, R"({
       "Item0": 2,
       "Item1": 0
   })");

   ASSERT_TRUE(component);
   ASSERT_EQ(4, component->getChildCount());

   ASSERT_TRUE(VerifyChild(component, 0, 0x008000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 1));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 2));
   ASSERT_TRUE(VerifyChild(component, 3, 0xFF0000FF, 3));

   auto child1 = component->getChildAt(1);
   auto child2 = component->getChildAt(2);
   auto child3 = component->getChildAt(3);

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem0" },
           { "value", 0 }
       },
       false);
   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "SelectorItem1" },
           { "value", 1 }
       },
       false);
   advanceTime(1);

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
   ASSERT_TRUE(VerifyChild(component, 0, 0xFF0000FF, 0));
   ASSERT_TRUE(VerifyChild(component, 1, 0xFF0000FF, 1));
   ASSERT_TRUE(VerifyChild(component, 2, 0xFF0000FF, 2));
   ASSERT_TRUE(VerifyChild(component, 3, 0xFF0000FF, 3));

   ASSERT_EQ(child1, component->getChildAt(1));
   ASSERT_EQ(child2, component->getChildAt(2));
   ASSERT_EQ(child3, component->getChildAt(3));
}

static const char *FRAME_CONDITIONAL_CHILD_OLD = R"({
  "type": "APL",
  "version": "2024.2",
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "bind": [
        { "name": "Selector", "value": false }
      ],
      "item": [
        {
          "when": "${Selector}",
          "type": "Text",
          "text": "Selected"
        },
        {
          "type": "Text",
          "text": "Selector: ${Selector}"
        }
      ]
    }
  }
})";

TEST_F(ReactiveRebuilds, FrameConditionalChildOld)
{
   loadDocument(FRAME_CONDITIONAL_CHILD_OLD);

   ASSERT_EQ("Selector: false", component->getChildAt(0)->getCalculated(kPropertyText).asString());

   executeCommand("SetValue", { { "componentId", ":root" }, { "property", "Selector"}, { "value", true } }, false);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ("Selector: true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
}

static const char *FRAME_CONDITIONAL_CHILD = R"({
  "type": "APL",
  "version": "2024.2",
  "settings": {
    "-experimentalIsReactive": true
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "bind": [
        { "name": "Selector", "value": false }
      ],
      "item": [
        {
          "when": "${Selector}",
          "type": "Text",
          "text": "Selected"
        },
        {
          "type": "Text",
          "text": "Selector: ${Selector}"
        }
      ]
    }
  }
})";

TEST_F(ReactiveRebuilds, FrameConditionalChild)
{
   loadDocument(FRAME_CONDITIONAL_CHILD);

   ASSERT_EQ("Selector: false", component->getChildAt(0)->getCalculated(kPropertyText).asString());

   executeCommand("SetValue", { { "componentId", ":root" }, { "property", "Selector"}, { "value", true } }, false);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ("Selected", component->getChildAt(0)->getCalculated(kPropertyText).asString());
}

static const char *FRAME_CONDITIONAL_LAYOUT = R"({
  "type": "APL",
  "version": "2024.2",
  "settings": {
    "-experimentalIsReactive": true
  },
  "layouts": {
    "Selected": {
      "items": [
        {
          "type": "Text",
          "text": "Selected"
        }
      ]
    },
    "KindaSelected": {
      "parameters": [
        {
          "name": "Selector",
          "type": "boolean"
        }
      ],
      "items": [
        {
          "type": "Text",
          "text": "Selector: ${Selector}"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "bind": [
        { "name": "Input", "value": false }
      ],
      "item": [
        {
          "when": "${Input}",
          "type": "Selected"
        },
        {
          "type": "KindaSelected",
          "Selector": "${Input}"
        }
      ]
    }
  }
})";

TEST_F(ReactiveRebuilds, FrameConditionalLayout)
{
   loadDocument(FRAME_CONDITIONAL_LAYOUT);

   ASSERT_EQ("Selector: false", component->getChildAt(0)->getCalculated(kPropertyText).asString());

   executeCommand("SetValue", { { "componentId", ":root" }, { "property", "Input"}, { "value", true } }, false);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ("Selected", component->getChildAt(0)->getCalculated(kPropertyText).asString());
}

static const char *FRAME_CONDITIONAL_EMPTY = R"({
  "type": "APL",
  "version": "2024.2",
  "settings": {
    "-experimentalIsReactive": true
  },
  "mainTemplate": {
    "parameters": [ "InputSelector" ],
    "items": {
      "type": "Frame",
      "bind": [
        { "name": "Selector", "value": "${InputSelector}" }
      ],
      "item": [
        {
          "when": "${Selector}",
          "type": "Text",
          "text": "Selected"
        }
      ]
    }
  }
})";

TEST_F(ReactiveRebuilds, FrameConditionalChildDisappear)
{
   loadDocument(FRAME_CONDITIONAL_EMPTY, R"({ "InputSelector": true })");

   ASSERT_EQ(1, component->getChildCount());

   executeCommand("SetValue", { { "componentId", ":root" }, { "property", "Selector"}, { "value", false } }, false);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ(0, component->getChildCount());
}

TEST_F(ReactiveRebuilds, FrameConditionalChildAppear)
{
   loadDocument(FRAME_CONDITIONAL_EMPTY, R"({ "InputSelector": false })");

   ASSERT_EQ(0, component->getChildCount());

   executeCommand("SetValue", { { "componentId", ":root" }, { "property", "Selector"}, { "value", true } }, false);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ(1, component->getChildCount());
}

static const char *FRAME_CONDITIONAL_DEEP_LAYOUT = R"({
  "type": "APL",
  "version": "2024.2",
  "settings": {
    "-experimentalIsReactive": true
  },
  "layouts": {
    "Selector": {
      "parameters": [
        {
          "name": "Selector",
          "type": "boolean"
        }
      ],
      "items": [
        {
          "when": "${Selector}",
          "type": "Text",
          "text": "Selected"
        },
        {
          "type": "Text",
          "text": "Selector: ${Selector}"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "bind": [
        { "name": "Input", "value": false }
      ],
      "item": [
        {
          "type": "Selector",
          "Selector": "${Input}"
        }
      ]
    }
  }
})";

TEST_F(ReactiveRebuilds, FrameConditionalDeepLayout)
{
   loadDocument(FRAME_CONDITIONAL_DEEP_LAYOUT);

   ASSERT_EQ("Selector: false", component->getChildAt(0)->getCalculated(kPropertyText).asString());

   executeCommand("SetValue", { { "componentId", ":root" }, { "property", "Input"}, { "value", true } }, false);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ("Selected", component->getChildAt(0)->getCalculated(kPropertyText).asString());
}

static const char *FRAME_CONDITIONAL_NO_CHANGE = R"({
  "type": "APL",
  "version": "2024.2",
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "bind": [
        { "name": "Selector", "value": 1 }
      ],
      "item": [
        {
          "when": "${Selector > 0}",
          "type": "Text",
          "text": "Selected"
        },
        {
          "type": "Text",
          "text": "Selector: ${Selector}"
        }
      ]
    }
  }
})";

TEST_F(ReactiveRebuilds, FrameConditionalNoChange)
{
   loadDocument(FRAME_CONDITIONAL_NO_CHANGE);

   ASSERT_EQ("Selected", component->getChildAt(0)->getCalculated(kPropertyText).asString());

   executeCommand("SetValue", { { "componentId", ":root" }, { "property", "Selector"}, { "value", 2 } }, false);
   root->clearPending();

   ASSERT_FALSE(CheckDirty(component, kPropertyNotifyChildrenChanged));
}

static const char *FRAME_CONDITIONAL_LAYOUT_NO_CHANGE = R"({
  "type": "APL",
  "version": "2024.2",
  "settings": {
    "-experimentalIsReactive": true
  },
  "layouts": {
    "Selector": {
      "parameters": [
        {
          "name": "Selector",
          "type": "number"
        }
      ],
      "items": [
        {
          "when": "${Selector > 0}",
          "type": "Text",
          "text": "Selected"
        },
        {
          "type": "Text",
          "text": "Selector: ${Selector}"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "bind": [
        { "name": "Input", "value": 1 }
      ],
      "item": [
        {
          "type": "Selector",
          "Selector": "${Input}"
        }
      ]
    }
  }
})";

TEST_F(ReactiveRebuilds, FrameConditionalLayoutNoChange)
{
    loadDocument(FRAME_CONDITIONAL_LAYOUT_NO_CHANGE);

    ASSERT_EQ("Selected", component->getChildAt(0)->getCalculated(kPropertyText).asString());

    executeCommand("SetValue", { { "componentId", ":root" }, { "property", "Input"}, { "value", 2 } }, false);
    root->clearPending();

    ASSERT_FALSE(CheckDirty(component, kPropertyNotifyChildrenChanged));
}

static const char *FRAME_CONDITIONAL_LAYOUT_EXISTENCE = R"({
  "type": "APL",
  "version": "2024.2",
  "settings": {
    "-experimentalIsReactive": true
  },
  "layouts": {
    "Selector": {
      "parameters": [
        {
          "name": "Selector",
          "type": "boolean"
        }
      ],
      "items": [
        {
          "when": "${Selector}",
          "type": "Text",
          "text": "Selected"
        }
      ]
    }
  },
  "mainTemplate": {
    "parameters": [ "InputParameter" ],
    "items": {
      "type": "Frame",
      "bind": [
        { "name": "Input", "value": "${InputParameter}" }
      ],
      "item": [
        {
          "type": "Selector",
          "Selector": "${Input}"
        }
      ]
    }
  }
})";

TEST_F(ReactiveRebuilds, FrameConditionalLayoutAppear)
{
   loadDocument(FRAME_CONDITIONAL_LAYOUT_EXISTENCE, R"( { "InputParameter": false } )");

   ASSERT_EQ(0, component->getChildCount());

   executeCommand("SetValue", { { "componentId", ":root" }, { "property", "Input"}, { "value", true } }, false);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ("Selected", component->getChildAt(0)->getCalculated(kPropertyText).asString());
}

TEST_F(ReactiveRebuilds, FrameConditionalLayoutDisappear)
{
   loadDocument(FRAME_CONDITIONAL_LAYOUT_EXISTENCE, R"( { "InputParameter": true } )");

   ASSERT_EQ("Selected", component->getChildAt(0)->getCalculated(kPropertyText).asString());

   executeCommand("SetValue", { { "componentId", ":root" }, { "property", "Input"}, { "value", false } }, false);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ(0, component->getChildCount());
}

static const char *SAME_ITEM_NOT_RECREATED = R"apl({
 "type": "APL",
 "version": "2024.1",
 "settings": {
   "-experimentalIsReactive": true
 },
 "mainTemplate": {
   "item": {
     "type": "Frame",
     "bind": [
       { "name": "Flag", "value": 1 }
     ],
     "height": 300,
     "width": 200,
     "item": [
       {
         "when": "${Flag > 0}",
         "type": "Frame",
         "height": "100%",
         "width": "100%",
         "background": "red"

       },
       {
         "type": "Frame",
         "height": "100%",
         "width": "100%",
         "background": "blue"
       }
     ]
   }
 }
})apl";

TEST_F(ReactiveRebuilds, SameItemNotRecalculate) {
   loadDocument(SAME_ITEM_NOT_RECREATED);

   ASSERT_TRUE(component);

   auto childId = component->getCoreChildAt(0)->getUniqueId();

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "Flag" },
           { "value", 2 }
       },
       false);

   advanceTime(17);

   ASSERT_EQ(childId, component->getCoreChildAt(0)->getUniqueId());
}

static const char *DYNAMIC_CONDITIONAL_OLD = R"({
  "type": "APL",
  "version": "2024.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "items": [
        {
          "when": "${data > 1}",
          "type": "Text",
          "text": "Definitely more than 1 : ${data > 1}"
        },
        {
          "type": "Text",
          "text": "Maybe 1 : ${data == 1}"
        }
      ]
    }
  }
})";

TEST_F(ReactiveRebuilds, DynamicConditionalOld) {
   auto myArray = LiveArray::create(ObjectArray{1, 101, 1});
   config->liveData("TestArray", myArray);

   loadDocument(DYNAMIC_CONDITIONAL_OLD);
   ASSERT_TRUE(component);
   ASSERT_EQ(3, component->getChildCount());

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : true", component->getChildAt(1)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(2)->getCalculated(kPropertyText).asString());

   myArray->update(1, 1);
   myArray->update(2, 101);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component));

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : false", component->getChildAt(1)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : false", component->getChildAt(2)->getCalculated(kPropertyText).asString());
}

static const char *DYNAMIC_CONDITIONAL_DEEP_OLD = R"({
  "type": "APL",
  "version": "2024.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "items": [
        {
          "type": "Frame",
          "item": [
            {
              "when": "${data > 1}",
              "type": "Text",
              "text": "Definitely more than 1 : ${data > 1}"
            },
            {
              "type": "Text",
              "text": "Maybe 1 : ${data == 1}"
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(ReactiveRebuilds, DynamicConditionalDeepOld) {
   auto myArray = LiveArray::create(ObjectArray{1, 101, 1});
   config->liveData("TestArray", myArray);

   loadDocument(DYNAMIC_CONDITIONAL_DEEP_OLD);
   ASSERT_TRUE(component);
   ASSERT_EQ(3, component->getChildCount());

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : true", component->getChildAt(1)->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(2)->getChildAt(0)->getCalculated(kPropertyText).asString());

   myArray->update(1, 1);
   myArray->update(2, 101);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component));

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : false", component->getChildAt(1)->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : false", component->getChildAt(2)->getChildAt(0)->getCalculated(kPropertyText).asString());
}

static const char *DYNAMIC_CONDITIONAL_CHANGE = R"({
  "type": "APL",
  "version": "2024.2",
  "settings": {
    "-experimentalIsReactive": true
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "items": [
        {
          "when": "${data > 1}",
          "type": "Text",
          "text": "Definitely more than 1 : ${data > 1}"
        },
        {
          "when": "${data == 1}",
          "type": "Text",
          "text": "Maybe 1 : ${data == 1}"
        }
      ]
    }
  }
})";

TEST_F(ReactiveRebuilds, DynamicConditionalNew) {
   auto myArray = LiveArray::create(ObjectArray{1, 101, 1});
   config->liveData("TestArray", myArray);

   loadDocument(DYNAMIC_CONDITIONAL_CHANGE);
   ASSERT_TRUE(component);
   ASSERT_EQ(3, component->getChildCount());

   auto unchangedUid = component->getChildAt(0)->getUniqueId();

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : true", component->getChildAt(1)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(2)->getCalculated(kPropertyText).asString());

   myArray->update(1, 1);
   myArray->update(2, 101);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ(unchangedUid, component->getChildAt(0)->getUniqueId());

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(1)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : true", component->getChildAt(2)->getCalculated(kPropertyText).asString());

   myArray->update(1, 101);
   myArray->update(2, 1);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ(unchangedUid, component->getChildAt(0)->getUniqueId());

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : true", component->getChildAt(1)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(2)->getCalculated(kPropertyText).asString());
}

TEST_F(ReactiveRebuilds, DynamicConditionalDisappear) {
   auto myArray = LiveArray::create(ObjectArray{1, 101, 1});
   config->liveData("TestArray", myArray);

   loadDocument(DYNAMIC_CONDITIONAL_CHANGE);
   ASSERT_TRUE(component);
   ASSERT_EQ(3, component->getChildCount());

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : true", component->getChildAt(1)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(2)->getCalculated(kPropertyText).asString());

   myArray->update(1, 0);
   myArray->update(2, 101);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ(2, component->getChildCount());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : true", component->getChildAt(1)->getCalculated(kPropertyText).asString());
}

TEST_F(ReactiveRebuilds, DynamicConditionalAppear) {
   auto myArray = LiveArray::create(ObjectArray{1, 0, 101});
   config->liveData("TestArray", myArray);

   loadDocument(DYNAMIC_CONDITIONAL_CHANGE);
   ASSERT_TRUE(component);
   ASSERT_EQ(2, component->getChildCount());

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : true", component->getChildAt(1)->getCalculated(kPropertyText).asString());

   myArray->update(1, 1);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ(3, component->getChildCount());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(1)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : true", component->getChildAt(2)->getCalculated(kPropertyText).asString());
}

static const char *DYNAMIC_CONDITIONAL_LAYOUT = R"({
  "type": "APL",
  "version": "2024.2",
  "settings": {
    "-experimentalIsReactive": true
  },
  "layouts": {
    "TestLayout": {
      "parameters": [
        {
          "name": "MaybeOne",
          "type": "number"
        }
      ],
      "bind": [ { "name": "Moar", "value": "${MaybeOne > 1}" } ],
      "item": [
        {
          "when": "${Moar}",
          "type": "Text",
          "text": "Definitely more than 1 : ${Moar}"
        },
        {
          "type": "Text",
          "text": "Maybe 1 : ${MaybeOne == 1}"
        }
      ]
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "items": {
        "type": "TestLayout",
        "MaybeOne": "${data}"
      }
    }
  }
})";

TEST_F(ReactiveRebuilds, DynamicConditionalLayout) {
   auto myArray = LiveArray::create(ObjectArray{1, 101, 1});
   config->liveData("TestArray", myArray);

   loadDocument(DYNAMIC_CONDITIONAL_LAYOUT);
   ASSERT_TRUE(component);
   ASSERT_EQ(3, component->getChildCount());

   auto unchangedUid = component->getChildAt(0)->getUniqueId();

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : true", component->getChildAt(1)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(2)->getCalculated(kPropertyText).asString());

   myArray->update(1, 1);
   myArray->update(2, 101);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ(unchangedUid, component->getChildAt(0)->getUniqueId());

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(1)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : true", component->getChildAt(2)->getCalculated(kPropertyText).asString());
}

static const char *DYNAMIC_CONDITIONAL_EXTERNALLY_DEPENDANT = R"({
  "type": "APL",
  "version": "2024.2",
  "settings": {
    "-experimentalIsReactive": true
  },
  "mainTemplate": {
    "bind": [
      { "name": "Selector", "value": true }
    ],
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "items": [
        {
          "when": "${data > 1 && Selector}",
          "type": "Text",
          "text": "Definitely more than 1 : ${data > 1}"
        },
        {
          "when": "${data >= 1}",
          "type": "Text",
          "text": "Maybe 1 : ${data == 1}"
        }
      ]
    }
  }
})";

TEST_F(ReactiveRebuilds, DynamicConditionalExternallyDependant) {
   auto myArray = LiveArray::create(ObjectArray{1, 101, 1});
   config->liveData("TestArray", myArray);

   loadDocument(DYNAMIC_CONDITIONAL_EXTERNALLY_DEPENDANT);
   ASSERT_TRUE(component);
   ASSERT_EQ(3, component->getChildCount());

   auto unchangedUid = component->getChildAt(0)->getUniqueId();

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Definitely more than 1 : true", component->getChildAt(1)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(2)->getCalculated(kPropertyText).asString());

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "Selector" },
           { "value", false}
       },
       false);
   root->clearPending();

   ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

   ASSERT_EQ(unchangedUid, component->getChildAt(0)->getUniqueId());

   ASSERT_EQ("Maybe 1 : true", component->getChildAt(0)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : false", component->getChildAt(1)->getCalculated(kPropertyText).asString());
   ASSERT_EQ("Maybe 1 : true", component->getChildAt(2)->getCalculated(kPropertyText).asString());
}

static const char *PROPERTY_PRESERVE_SINGLE = R"apl({
 "type": "APL",
 "version": "2024.1",
 "settings": {
   "-experimentalIsReactive": true
 },
 "mainTemplate": {
   "item": {
     "type": "Frame",
     "bind": [
       { "name": "SequenceItems", "value": [ "red", "blue", "green", "yellow", "purple", "cyan" ] },
       { "name": "VerticalOrientation", "value": true }
     ],
     "height": 300,
     "width": 200,
     "item": [
       {
         "when": "${VerticalOrientation}",
         "type": "Sequence",
         "id": "Scrollable",
         "preserve": ["scrollOffset"],
         "scrollDirection": "vertical",
         "height": "100%",
         "width": "100%",
         "data": "${SequenceItems}",
         "item": {
           "type": "Frame",
           "width": "100%",
           "height": 100,
           "background": "${data}",
           "item": {
             "type": "Text",
             "id": "Indicator${index}",
             "width": "100%",
             "height": "100%",
             "text": "${data} : ${index}"
           }
         }
       },
       {
         "type": "Sequence",
         "id": "Scrollable",
         "preserve": ["scrollOffset"],
         "scrollDirection": "horizontal",
         "height": "100%",
         "width": "100%",
         "data": "${SequenceItems}",
         "item": {
           "type": "Frame",
           "width": 100,
           "height": "100%",
           "background": "${data}",
           "item": {
             "type": "Text",
             "id": "Indicator${index}",
             "width": "100%",
             "height": "100%",
             "text": "${data} : ${index}"
           }
         }
       }
     ]
   }
 }
})apl";

TEST_F(ReactiveRebuilds, PropertyReserveSingle) {
   loadDocument(PROPERTY_PRESERVE_SINGLE);

   ASSERT_TRUE(component);

   auto sequence = component->getCoreChildAt(0);
   ASSERT_EQ(apl::kScrollDirectionVertical, sequence->getCalculated(apl::kPropertyScrollDirection).asInt());
   ASSERT_EQ(0, sequence->scrollPosition().getY());

   executeCommand(
       "Scroll",
       {
           {"componentId", "Scrollable"},
           {"distance", 1},
           {"screenLock",  true}
       },
       false);
   advanceTime(2000);
   ASSERT_EQ(300, sequence->scrollPosition().getY());

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "VerticalOrientation" },
           { "value", false }
       },
       false);

   // Needs two frames. First processes any changes, another one performs layout. Why? To avoid
   // infinite layout cycle in one frame.
   advanceTime(17);
   advanceTime(17);

   sequence = component->getCoreChildAt(0);
   ASSERT_EQ(apl::kScrollDirectionHorizontal, sequence->getCalculated(apl::kPropertyScrollDirection).asInt());
   ASSERT_EQ(300, sequence->scrollPosition().getX());
}

static const char *PROPERTY_PRESERVE_DATA = R"apl({
  "type": "APL",
  "version": "2024.1",
  "settings": {
    "-experimentalIsReactive": true
  },
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "bind": [
          { "name": "Frameless", "value": true }
        ],
      "id": "Container",
      "scrollDirection": "vertical",
      "height": 500,
      "width": 500,
      "data": [
            "red",
            "blue",
            "green",
            "yellow",
            "purple"
          ],
      "items": [
        {
          "when": "${Frameless}",
          "type": "Text",
          "preserve": ["text"],
          "id": "Indicator${index}",
          "width": "100%",
          "height": 100,
          "text": "${data} : ${index}"
        },
        {
          "type": "Frame",
          "width": "100%",
          "height": 100,
          "background": "${data}",
          "item": {
            "type": "Text",
            "preserve": ["text"],
            "id": "Indicator${index}",
            "width": "100%",
            "height": "100%",
            "text": "${data} : ${index}"
          }
        }
      ]
    }
  }
})apl";

TEST_F(ReactiveRebuilds, PropertyReserveData) {
   loadDocument(PROPERTY_PRESERVE_DATA);

   ASSERT_TRUE(component);

   auto comp = component->findComponentById("Indicator2", false);
   auto uid = comp->getUniqueId();
   ASSERT_EQ("green : 2", comp->getCalculated(apl::kPropertyText).asString());

   executeCommand(
       "SetValue",
       {
           { "componentId", "Indicator2" },
           { "property", "text" },
           { "value", "Replaced" }
       },
       false);

   advanceTime(17);

   comp = component->findComponentById("Indicator2", false);
   ASSERT_EQ(uid, comp->getUniqueId());
   ASSERT_EQ("Replaced", comp->getCalculated(apl::kPropertyText).asString());

   executeCommand(
       "SetValue",
       {
           { "componentId", ":root" },
           { "property", "Frameless" },
           { "value", false }
       },
       false);

   advanceTime(17);

   comp = component->findComponentById("Indicator2", false);
   ASSERT_NE(uid, comp->getUniqueId());
   ASSERT_EQ("Replaced", comp->getCalculated(apl::kPropertyText).asString());
}
