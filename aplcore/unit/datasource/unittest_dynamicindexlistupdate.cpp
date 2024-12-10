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

class DynamicIndexListUpdateTest : public DynamicIndexListTest {};

static const char *RESTRICTED_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 10,
    "minimumInclusiveIndex": 10,
    "maximumExclusiveIndex": 15,
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

TEST_F(DynamicIndexListUpdateTest, ShrinkData)
{
    loadDocument(BASIC, SHRINKABLE_DATA);
    advanceTime(10);
    ASSERT_TRUE(CheckBounds(10, 15));
    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, {0, 4}, true));
}

static const char *BASIC_CRUD_SERIES = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "listVersion": 1,
  "operations": [
    {
      "type": "InsertListItem",
      "index": 11,
      "item": 111
    },
    {
      "type": "ReplaceListItem",
      "index": 13,
      "item": 113
    },
    {
      "type": "DeleteListItem",
      "index": 12
    }
  ]
})";

TEST_F(DynamicIndexListUpdateTest, CrudBasicSeries)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(10, 15));

    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));

    ASSERT_TRUE(ds->processUpdate(BASIC_CRUD_SERIES));
    root->clearPending();

    ASSERT_TRUE(CheckChildren({10, 111, 113, 13, 14}));
}

static const char *BROKEN_CRUD_SERIES = R"({
 "presentationToken": "presentationToken",
 "listId": "vQdpOESlok",
 "listVersion": 1,
 "operations": [
   {
     "type": "InsertListItem",
     "index": 11,
     "item": 111
   },
   {
     "type": "InsertListItem",
     "index": 27,
     "item": 27
   },
   {
     "type": "ReplaceListItem",
     "index": 13,
     "item": 113
   },
   {
     "type": "DeleteListItem",
     "index": 27,
     "item": 27
   },
   {
     "type": "DeleteListItem",
     "index": 12
   }
 ]
})";

TEST_F(DynamicIndexListUpdateTest, CrudInvalidInbetweenSeries)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckBounds(10, 15));

    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));

    ASSERT_FALSE(ds->processUpdate(BROKEN_CRUD_SERIES));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));
    root->clearPending();

    ASSERT_TRUE(CheckChildren({10, 111, 11, 12, 13, 14}));
}

static const char *STARTING_BOUNDS_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": -5,
    "minimumInclusiveIndex": -5,
    "maximumExclusiveIndex": 5,
    "items": [ -5, -4, -3, -2, -1, 0, 1, 2, 3, 4 ]
  }
})";

TEST_F(DynamicIndexListUpdateTest, CrudBoundsVerification)
{
    loadDocument(BASIC, STARTING_BOUNDS_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));

    ASSERT_TRUE(CheckBounds(-5, 5));

    // Negative insert
    ASSERT_TRUE(ds->processUpdate(createInsert(1, -3, -103)));
    root->clearPending();
    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 6));
    ASSERT_TRUE(CheckChildren({-5, -4, -103, -3, -2, -1, 0, 1, 2, 3, 4}));

    // Positive insert
    ASSERT_TRUE(ds->processUpdate(createInsert(2, 3, 103)));
    root->clearPending();
    ASSERT_EQ(12, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 7));
    ASSERT_TRUE(CheckChildren({-5, -4, -103, -3, -2, -1, 0, 1, 103, 2, 3, 4}));

    // Insert on 0
    ASSERT_TRUE(ds->processUpdate(createInsert(3, 0, 100)));
    root->clearPending();
    ASSERT_EQ(13, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 8));
    ASSERT_TRUE(CheckChildren({-5, -4, -103, -3, -2, 100, -1, 0, 1, 103, 2, 3, 4}));

    // Negative delete
    ASSERT_TRUE(ds->processUpdate(createDelete(4, -5)));
    root->clearPending();
    ASSERT_EQ(12, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 7));
    ASSERT_TRUE(CheckChildren({-4, -103, -3, -2, 100, -1, 0, 1, 103, 2, 3, 4}));

    // Positive delete
    ASSERT_TRUE(ds->processUpdate(createDelete(5, 3)));
    root->clearPending();
    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 6));
    ASSERT_TRUE(CheckChildren({-4, -103, -3, -2, 100, -1, 0, 1, 2, 3, 4}));

    // Delete on 0
    ASSERT_TRUE(ds->processUpdate(createDelete(6, 0)));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 5));
    ASSERT_TRUE(CheckChildren({-4, -103, -3, -2, 100, 0, 1, 2, 3, 4}));
}

TEST_F(DynamicIndexListUpdateTest, CrudPayloadGap)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Insert with gap
    ASSERT_FALSE(ds->processUpdate(createInsert(1, 17, 17)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListUpdateTest, CrudPayloadInsertOOB)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Insert out of bounds
    ASSERT_FALSE(ds->processUpdate(createInsert(1, 21, 21)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListUpdateTest, CrudPayloadRemoveOOB)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Remove out of bounds
    ASSERT_FALSE(ds->processUpdate(createDelete(1, 21)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListUpdateTest, CrudPayloadReplaceOOB)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Replace out of bounds
    ASSERT_FALSE(ds->processUpdate(createReplace(1, 21, 1000)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

static const char *WRONG_TYPE_CRUD = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "listVersion": 1,
  "operations": [
    {
      "type": "7",
      "index": 10,
      "item": 101
    }
  ]
})";

TEST_F(DynamicIndexListUpdateTest, CrudPayloadInvalidOperation)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Specify wrong operation
    ASSERT_FALSE(ds->processUpdate(WRONG_TYPE_CRUD));
    ASSERT_TRUE(CheckErrors({ "INVALID_OPERATION" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

static const char *MALFORMED_OPERATION_CRUD = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "listVersion": 1,
  "operations": [
    {
      "type": "InsertItem",
      "item": 101
    }
  ]
})";

TEST_F(DynamicIndexListUpdateTest, CrudPayloadMalformedOperation)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Specify wrong operation
    ASSERT_FALSE(ds->processUpdate(MALFORMED_OPERATION_CRUD));
    ASSERT_TRUE(CheckErrors({ "INVALID_OPERATION" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

static const char *MISSING_OPERATIONS_CRUD = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "listVersion": 1
})";

TEST_F(DynamicIndexListUpdateTest, CrudPayloadNoOperation)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    // Don't specify any operations
    ASSERT_FALSE(ds->processUpdate(MISSING_OPERATIONS_CRUD));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

static const char *MISSING_LIST_VERSION_CRUD = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "operations": [
    {
      "type": "InsertItem",
      "index": 10,
      "item": 101
    }
  ]
})";

TEST_F(DynamicIndexListUpdateTest, CrudPayloadNoListVersion)
{
    loadDocument(BASIC, RESTRICTED_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({10, 11, 12, 13, 14}));
    ASSERT_TRUE(CheckBounds(10, 15));

    ASSERT_FALSE(ds->processUpdate(MISSING_LIST_VERSION_CRUD));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_VERSION_IN_SEND_DATA" }));
}

TEST_F(DynamicIndexListUpdateTest, CrudMultiInsert) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Negative insert
    ASSERT_TRUE(ds->processUpdate(createMultiInsert(1, -3, {-31, -32})));
    root->clearPending();
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -31, -32, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 7));

    // Positive insert
    ASSERT_TRUE(ds->processUpdate(createMultiInsert(2, 3, {31, 32})));
    root->clearPending();
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -31, -32, -2, -1, 0, 31, 32, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 9));

    // Above loaded adjust insert
    ASSERT_TRUE(ds->processUpdate(createMultiInsert(3, 9, {71, 72})));
    root->clearPending();
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -31, -32, -2, -1, 0, 31, 32, 1, 2, 3, 4, 71, 72}));
    ASSERT_TRUE(CheckBounds(-5, 11));
}

TEST_F(DynamicIndexListUpdateTest, CrudMultiInsertAbove) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Attach at the end
    ASSERT_FALSE(ds->processUpdate(createMultiInsert(1, 10, {100, 101})));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListUpdateTest, CrudMultiInsertBelow) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Below loaded insert
    ASSERT_FALSE(ds->processUpdate(createMultiInsert(1, -10, {-100, -101})));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

static const char *NON_ARRAY_MULTI_INSERT = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "listVersion": 1,
  "operations": [
    {
      "type": "InsertMultipleItems",
      "index": 11,
      "items": 111
    }
  ]
})";

TEST_F(DynamicIndexListUpdateTest, CrudMultiInsertNonArray) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Below loaded insert
    ASSERT_FALSE(ds->processUpdate(NON_ARRAY_MULTI_INSERT));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListUpdateTest, CrudMultiDelete) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Remove across
    ASSERT_TRUE(ds->processUpdate(createMultiDelete(1, -1, 3)));
    root->clearPending();
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 2));

    // Delete negative
    ASSERT_TRUE(ds->processUpdate(createMultiDelete(2, -5, 2)));
    root->clearPending();
    ASSERT_TRUE(CheckChildren({-3, -2, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 0));

    // Delete at the end
    ASSERT_TRUE(ds->processUpdate(createMultiDelete(3, -2, 2)));
    root->clearPending();
    ASSERT_TRUE(CheckChildren({-3, -2, 2}));
    ASSERT_TRUE(CheckBounds(-5, -2));
}

TEST_F(DynamicIndexListUpdateTest, CrudMultiDeleteOOB) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Out of range
    ASSERT_FALSE(ds->processUpdate(createMultiDelete(1, 7, 2)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListUpdateTest, CrudMultiDeletePartialOOB) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Some out of range
    ASSERT_FALSE(ds->processUpdate(createMultiDelete(1, 15, 3)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListUpdateTest, CrudMultiDeleteAll) {
    loadDocument(BASIC, STARTING_BOUNDS_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(createMultiDelete(1, -5, 10)));
    root->clearPending();
    ASSERT_EQ(0, component->getChildCount());
}


static const char *SINGULAR_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": -5,
    "maximumExclusiveIndex": 5,
    "items": [ 0 ]
  }
})";

TEST_F(DynamicIndexListUpdateTest, CrudMultiDeleteMore) {
    loadDocument(BASIC, SINGULAR_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckChildren({0}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    // Some out of range
    ASSERT_FALSE(ds->processUpdate(createMultiDelete(1, 15, 3)));
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));

    ASSERT_EQ(1, component->getChildCount());
}

TEST_F(DynamicIndexListUpdateTest, CrudMultiDeleteLast) {
    loadDocument(BASIC, SINGULAR_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckChildren({0}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(createMultiDelete(1, 0, 1)));
    root->clearPending();
    ASSERT_EQ(0, component->getChildCount());
}

TEST_F(DynamicIndexListUpdateTest, CrudDeleteLast) {
    loadDocument(BASIC, SINGULAR_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckChildren({0}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(createDelete(1, 0)));
    root->clearPending();
    ASSERT_EQ(0, component->getChildCount());
}

TEST_F(DynamicIndexListUpdateTest, CrudInsertAdjascent) {
    loadDocument(BASIC, SINGULAR_DATA);
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(1, component->getChildCount());
    ASSERT_TRUE(CheckChildren({0}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(createInsert(1, 1, 1))); // This allowed (N+1)
    ASSERT_TRUE(ds->processUpdate(createInsert(2, 0, 11))); // This is also allowed (M)
    ASSERT_FALSE(ds->processUpdate(createInsert(3, -1, -1))); // This is not (M-1)
    ASSERT_TRUE(CheckErrors({ "LIST_INDEX_OUT_OF_RANGE" }));
    root->clearPending();

    ASSERT_TRUE(CheckChildren({11, 0, 1}));
    ASSERT_TRUE(CheckBounds(-5, 7));
    ASSERT_EQ(3, component->getChildCount());
}

static const char *LAZY_CRUD_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": -2,
    "minimumInclusiveIndex": -5,
    "maximumExclusiveIndex": 5,
    "items": [ -2, -1, 0, 1, 2 ]
  }
})";

TEST_F(DynamicIndexListUpdateTest, CrudLazyCombination)
{
    loadDocument(BASIC, LAZY_CRUD_DATA);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 3, 2));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -5, 3));

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-2, -1, 0, 1, 2}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(createLazyLoad(1, 101, 3, "3, 4" )));
    ASSERT_TRUE(ds->processUpdate(createLazyLoad(2, 102, -5, "-5, -4, -3" )));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));

    ASSERT_TRUE(ds->processUpdate(createInsert(3, -2, -103)));
    root->clearPending();
    ASSERT_EQ(11, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 6));
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -103, -2, -1, 0, 1, 2, 3, 4}));

    ASSERT_TRUE(ds->processUpdate(createInsert(4, 4, 103)));
    root->clearPending();
    ASSERT_EQ(12, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 7));
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -103, -2, -1, 0, 1, 2, 103, 3, 4}));


}

static const char *LAZY_WITHOUT_VERSION = R"({
  "token": "presentationToken",
  "listId": "vQdpOESlok",
  "correlationToken": "102",
  "startIndex": -5,
  "items": [ -5, -4, -3 ]
})";

TEST_F(DynamicIndexListUpdateTest, CrudAfterNoVersionLazy)
{
    loadDocument(BASIC, LAZY_CRUD_DATA);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 3, 2));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -5, 3));

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-2, -1, 0, 1, 2}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(LAZY_WITHOUT_VERSION));
    root->clearPending();

    ASSERT_EQ(8, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2}));

    ASSERT_FALSE(ds->processUpdate(createInsert(1, 0, 101)));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_VERSION_IN_SEND_DATA" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListUpdateTest, CrudBeforeNoVersionLazy)
{
    loadDocument(BASIC, LAZY_CRUD_DATA);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 3, 2));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -5, 3));

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-2, -1, 0, 1, 2}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_TRUE(ds->processUpdate(createInsert(1, 0, 101)));
    root->clearPending();

    ASSERT_EQ(6, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-2, -1, 101, 0, 1, 2}));

    ASSERT_FALSE(ds->processUpdate(LAZY_WITHOUT_VERSION));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_VERSION_IN_SEND_DATA" }));

    // In fail state so will not allow other operation
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 10, 100)));
    ASSERT_TRUE(CheckErrors({ "INTERNAL_ERROR" }));
}

TEST_F(DynamicIndexListUpdateTest, CrudWrongData)
{
    loadDocument(BASIC, LAZY_CRUD_DATA);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", 3, 2));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", -5, 3));

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-2, -1, 0, 1, 2}));
    ASSERT_TRUE(CheckBounds(-5, 5));


    ASSERT_TRUE(ds->processUpdate(createInsert(1, -2, -103)));
    root->clearPending();
    ASSERT_EQ(6, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 6));
    ASSERT_TRUE(CheckChildren({-103, -2, -1, 0, 1, 2}));

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", 4, 2));

    // Wrong version crud will not fly
    ASSERT_FALSE(ds->processUpdate(createInsert(3, 0, 100))); // This is cached
    ASSERT_FALSE(ds->processUpdate(createInsert(1, 0, 100))); // This is not
    ASSERT_TRUE(CheckErrors({ "DUPLICATE_LIST_VERSION" }));
}

TEST_F(DynamicIndexListUpdateTest, CrudOutOfOrder)
{
    loadDocument(BASIC, STARTING_BOUNDS_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));

    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_FALSE(ds->processUpdate(createInsert(2, 4, 103)));
    ASSERT_FALSE(ds->processUpdate(createInsert(3, 2, 100)));
    ASSERT_FALSE(ds->processUpdate(createDelete(5, 5)));

    // Duplicate version in cache
    ASSERT_FALSE(ds->processUpdate(createDelete(5, 5)));
    ASSERT_TRUE(CheckErrors({ "DUPLICATE_LIST_VERSION" }));

    ASSERT_TRUE(ds->processUpdate(createInsert(1, -3, -103)));
    ASSERT_TRUE(ds->processUpdate(createDelete(4, -5)));

    ASSERT_TRUE(ds->processUpdate(createDelete(6, 2)));
    root->clearPending();
    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckBounds(-5, 5));
    ASSERT_TRUE(CheckChildren({-4, -103, -3, -2, -1, 0, 100, 2, 103, 4}));
}

TEST_F(DynamicIndexListUpdateTest, CrudBadOutOfOrder)
{
    loadDocument(BASIC, STARTING_BOUNDS_DATA);

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(10, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}));
    ASSERT_TRUE(CheckBounds(-5, 5));

    ASSERT_FALSE(ds->processUpdate(createInsert(6, 0, 7)));
    loop->advanceToTime(500);

    // Update 6 will expire
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_VERSION" }));

    ASSERT_FALSE(ds->processUpdate(createInsert(5, 0, 6)));
    ASSERT_FALSE(ds->processUpdate(createInsert(4, 0, 5)));
    ASSERT_FALSE(ds->processUpdate(createInsert(2, 0, 3)));
    ASSERT_FALSE(ds->processUpdate(createInsert(7, 0, 8)));
    ASSERT_FALSE(ds->processUpdate(createInsert(3, 0, 4)));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_VERSION" }));
    ASSERT_FALSE(ds->processUpdate(createInsert(8, 0, 9)));
    ASSERT_TRUE(CheckErrors({ "MISSING_LIST_VERSION" }));

    ASSERT_TRUE(ds->processUpdate(createInsert(1, 0, 2)));
    loop->advanceToEnd();
    ASSERT_TRUE(CheckErrors({}));

    root->clearPending();
    ASSERT_EQ(16, component->getChildCount());
    ASSERT_TRUE(CheckChildren({-5, -4, -3, -2, -1, 7, 6, 5, 4, 3, 2, 0, 1, 2, 3, 4}));
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

static const char *SWIPE_TO_DELETE = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
  "layouts": {
    "swipeAway" : {
      "parameters": ["text1", "text2"],
      "item": {
        "type": "TouchWrapper",
        "width": 200,
        "item": {
          "type": "Frame",
          "backgroundColor": "blue",
          "height": 100,
          "items": {
            "type": "Text",
            "text": "${text1}",
            "fontSize": 60
          }
        },
        "gestures": [
          {
            "type": "SwipeAway",
            "direction": "left",
            "action":"reveal",
            "items": {
              "type": "Frame",
              "backgroundColor": "purple",
              "width": "100%",
              "items": {
                "type": "Text",
                "text": "${text2}",
                "fontSize": 60,
                "color": "white"
              }
            },
            "onSwipeDone": {
              "type": "SendEvent",
              "arguments": ["${event.source.uid}", "${index}"]
            }
          }
        ]
      }
    }
  },
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "items": [
      {
        "type": "Sequence",
        "width": "100%",
        "height": 500,
        "alignItems": "center",
        "justifyContent": "spaceAround",
        "data": "${dynamicSource}",
        "items": [
          {
            "type": "swipeAway",
            "text1": "${data}",
            "text2": "${data}"
          }
        ]
      }
    ]
  }
})";

static const char *SWIPE_TO_DELETE_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 5,
    "items": [ 1, 2, 3, 4, 5 ]
  }
})";

TEST_F(DynamicIndexListUpdateTest, SwipeToDelete)
{
    config->set({
        {RootProperty::kSwipeAwayAnimationEasing, "linear"},
        {RootProperty::kPointerSlopThreshold, 5},
        {RootProperty::kSwipeVelocityThreshold, 5},
        {RootProperty::kTapOrScrollTimeout, 10},
        {RootProperty::kPointerInactivityTimeout, 1000}
    });
    loadDocument(SWIPE_TO_DELETE, SWIPE_TO_DELETE_DATA);
    advanceTime(10);

    ASSERT_TRUE(component);
    ASSERT_EQ(5, component->getChildCount());
    ASSERT_EQ(5, component->getDisplayedChildCount());

    auto idToDelete = component->getChildAt(0)->getUniqueId();

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(200,1), false));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(190,1), true));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(140,1), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(140,1), true));

    advanceTime(800);
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    int indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(0, indexToDelete);

    ASSERT_TRUE(ds->processUpdate(createDelete(1, indexToDelete)));
    advanceTime(100);
    ASSERT_EQ(4, component->getChildCount());
    ASSERT_EQ(4, component->getDisplayedChildCount());
    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    root->clearDirty();


    // Repeat for very first
    idToDelete = component->getChildAt(0)->getUniqueId();

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(200,1), false));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(190,1), true));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(140,1), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(140,1), true));

    advanceTime(800);
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(0, indexToDelete);
    root->clearDirty();

    ASSERT_TRUE(ds->processUpdate(createDelete(2, indexToDelete)));
    root->clearPending();
    ASSERT_EQ(3, component->getChildCount());
    ASSERT_EQ(3, component->getDisplayedChildCount());
    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyBounds,kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    root->clearDirty();


    // Remove one at the end
    idToDelete = component->getChildAt(2)->getUniqueId();

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(200,201), false));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(190,201), true));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(140,201), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(140,201), true));

    advanceTime(800);
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(2, indexToDelete);
    root->clearDirty();

    ASSERT_TRUE(ds->processUpdate(createDelete(3, indexToDelete)));
    root->clearPending();
    root->clearDirty();

    ASSERT_EQ(2, component->getChildCount());
    ASSERT_EQ(2, component->getDisplayedChildCount());

    // again
    idToDelete = component->getChildAt(0)->getUniqueId();

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(200,1), false));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(190,1), true));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(140,1), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(140,1), true));

    advanceTime(800);
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(0, indexToDelete);
    root->clearDirty();

    ASSERT_TRUE(ds->processUpdate(createDelete(4, indexToDelete)));
    root->clearPending();
    ASSERT_EQ(1, component->getChildCount());
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyBounds,kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    root->clearDirty();

    // empty the list
    idToDelete = component->getChildAt(0)->getUniqueId();

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(200,1), false));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(190,1), true));
    advanceTime(100);
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerMove, Point(140,1), true));
    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerUp, Point(140,1), true));

    advanceTime(800);
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    deletedId = event.getValue(kEventPropertyArguments).getArray().at(0).asString();
    indexToDelete = event.getValue(kEventPropertyArguments).getArray().at(1).asNumber();
    ASSERT_EQ(idToDelete, deletedId);
    ASSERT_EQ(0, indexToDelete);
    root->clearDirty();

    ASSERT_TRUE(ds->processUpdate(createDelete(5, indexToDelete)));
    root->clearPending();
    ASSERT_EQ(0, component->getChildCount());
    ASSERT_EQ(0, component->getDisplayedChildCount());
    root->clearDirty();
}

static const char *SEQUENCE_RECREATE_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 1,
    "items": [
      { "label": "I am a label.", "sequence": ["red", "green", "blue", "yellow", "purple"] }
    ]
  }
})";

static const char *SEQUENCE_RECREATE = R"({
  "type": "APL",
  "version": "1.7",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Container",
      "height": 300,
      "width": 300,
      "data": "${dynamicSource}",
      "items": {
        "type": "Container",
        "height": "100%",
        "width": "100%",
        "items": [
          {
            "type": "Sequence",
            "height": "50%",
            "width": "100%",
            "data": "${data.sequence}",
            "items": {
              "type": "Frame",
              "backgroundColor": "${data}",
              "height": 10,
              "width": "100%"
            }
          }
        ]
      }
    }
  }
})";

static const char *REPLACE_SEQUENCE_CRUD = R"({
  "presentationToken": "presentationToken",
  "listId": "vQdpOESlok",
  "listVersion": 1,
  "operations": [
    {
      "type": "DeleteListItem",
      "index": 0
    },
    {
      "type": "InsertListItem",
      "index": 0,
      "item": { "sequence": ["purple", "yellow", "blue", "green", "red"] }
    }
  ]
})";

TEST_F(DynamicIndexListUpdateTest, SequenceRecreate)
{
    loadDocument(SEQUENCE_RECREATE, SEQUENCE_RECREATE_DATA);
    advanceTime(10);

    ASSERT_EQ(1, component->getChildCount());
    auto sequence = component->getCoreChildAt(0)->getCoreChildAt(0);
    ASSERT_EQ(5, sequence->getChildCount());

    ASSERT_EQ(Rect(0,  0, 300, 300), component->getCoreChildAt(0)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(0, 0, 300, 150), sequence->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(0,  0, 300, 10), sequence->getCoreChildAt(0)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(0, 10, 300, 10), sequence->getCoreChildAt(1)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(0, 20, 300, 10), sequence->getCoreChildAt(2)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(0, 30, 300, 10), sequence->getCoreChildAt(3)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(0, 40, 300, 10), sequence->getCoreChildAt(4)->getCalculated(kPropertyBounds).get<Rect>());

    ASSERT_TRUE(ds->processUpdate(REPLACE_SEQUENCE_CRUD));
    root->clearPending();

    sequence = component->getCoreChildAt(0)->getCoreChildAt(0);
    ASSERT_EQ(5, sequence->getChildCount());

    ASSERT_EQ(Rect(0,  0, 300, 300).toDebugString(), component->getCoreChildAt(0)->getCalculated(kPropertyBounds).get<Rect>().toDebugString());
    ASSERT_EQ(Rect(0,  0, 300, 150), sequence->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(0,   0, 300, 10), sequence->getCoreChildAt(0)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(0, 10, 300, 10), sequence->getCoreChildAt(1)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(0, 20, 300, 10), sequence->getCoreChildAt(2)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(0, 30, 300, 10), sequence->getCoreChildAt(3)->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_EQ(Rect(0, 40, 300, 10), sequence->getCoreChildAt(4)->getCalculated(kPropertyBounds).get<Rect>());
}

static const char *FILLED_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "vQdpOESlok",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 5,
    "items": [ 0, 1, 2, 3, 4 ]
  }
})";

TEST_F(DynamicIndexListUpdateTest, DeleteMultipleAll)
{
    loadDocument(BASIC, FILLED_DATA);
    advanceTime(10);

    ASSERT_TRUE(CheckBounds(0, 5));
    ASSERT_EQ(5, component->getChildCount());

    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(ds->processUpdate(createMultiDelete(1, 0, 100)));
    root->clearPending();

    ASSERT_EQ(0, component->getChildCount());
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

TEST_F(DynamicIndexListUpdateTest, CurrentOrTargetPageCanBeDeleted)
{
    auto swipeToNextPage = [&]() {
        root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(150, 10)));
        advanceTime(100);
        root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(50, 10)));
        root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(50, 10)));
        root->clearPending();
    };

    int listVersion = 0;
    auto createInsertItem = [&](int index, std::string text = "") {
        if (text.empty()) text = std::to_string(index);
        return "{"
                "  \"presentationToken\": \"presentationToken\","
                "  \"listId\": \"vQdpOESlok\","
                "  \"listVersion\": " + std::to_string(++listVersion) + ","
                "  \"operations\": ["
                "    {"
                "      \"type\": \"InsertItem\","
                "      \"index\": " + std::to_string(index) + ","
                "      \"item\": { \"color\": \"green\", \"text\": \"" + text + "\" }" +
                "    }"
                "  ]"
                "}";
    };

    loadDocument(BASIC_PAGER, EMPTY_PAGER_DATA);

    // Insert a few items
    for (int i = 10; i <= 15; i++) {
        ASSERT_TRUE(ds->processUpdate(createInsertItem(i)));
    }
    root->clearPending();

    // We start on the first page
    ASSERT_TRUE(CheckPager(0, {{"frame-10"}, {"frame-11"}, {"frame-12"}, {"frame-13"}, {"frame-14"}, {"frame-15"}}));

    // Swipe! But before you reach the next page, delete it
    swipeToNextPage();
    ASSERT_TRUE(ds->processUpdate(createDelete(++listVersion, 11)));
    advanceTime(1000);
    
    // We remain on the first page
    ASSERT_TRUE(CheckPager(0, {{"frame-10"}, {"frame-12"}, {"frame-13"}, {"frame-14"}, {"frame-15"}}));

    // Swipe! Now we reach the next page
    swipeToNextPage();
    advanceTime(1000);
    ASSERT_TRUE(CheckPager(1, {{"frame-10"}, {"frame-12"}, {"frame-13"}, {"frame-14"}, {"frame-15"}}));

    // Swipe! Now delete the source page, but the swipe still succeeds in moving to the next page
    swipeToNextPage();
    ASSERT_TRUE(ds->processUpdate(createDelete(++listVersion, 11)));
    advanceTime(1000);
    ASSERT_TRUE(CheckPager(1, {{"frame-10"}, {"frame-13"}, {"frame-14"}, {"frame-15"}}));

    // Swipe! Again, delete target page, but also try to jump to page 3
    swipeToNextPage();
    advanceTime(10);
    // The animation has progressed a bit
    ASSERT_TRUE(CheckPager(1, {{"frame-10"}, {"frame-13", true}, {"frame-14", true}, {"frame-15"}}));
    // Now delete the target page
    ASSERT_TRUE(ds->processUpdate(createDelete(++listVersion, 12)));
    // And also manually try to go to page 3
    PagerComponent::setPageUtil(component, 3, kPageDirectionForward, ActionRef(nullptr));
    advanceTime(1000);
    // We succeed in reaching what was formally page 3 (now page 2)
    ASSERT_TRUE(CheckPager(2, {{"frame-10"}, {"frame-13"}, {"frame-15"}}));

    // Need to insert a couple of items
    ASSERT_TRUE(ds->processUpdate(createInsertItem(13, "88")));
    ASSERT_TRUE(ds->processUpdate(createInsertItem(14, "99")));
    root->clearPending();
    ASSERT_TRUE(CheckPager(2, {{"frame-10"}, {"frame-13"}, {"frame-15"}, {"frame-88"}, {"frame-99"}}));

    // Swipe! This time, delete the source page and jump to page 4
    swipeToNextPage();
    advanceTime(10);
    // The animation has progressed a bit
    ASSERT_TRUE(CheckPager(2, {{"frame-10"}, {"frame-13"}, {"frame-15", true}, {"frame-88", true}, {"frame-99"}}));
    // Now delete the source page
    ASSERT_TRUE(ds->processUpdate(createDelete(++listVersion, 12)));
    // And also manually try to go to the last page
    PagerComponent::setPageUtil(component, 4, kPageDirectionForward, ActionRef(nullptr));
    advanceTime(1000);
    // We succeed in reaching the last page
    ASSERT_TRUE(CheckPager(3, {{"frame-10"}, {"frame-13"}, {"frame-88"}, {"frame-99"}}));

    // Some errors are expected from unfulfilled requests
    ASSERT_TRUE(ds->getPendingErrors().size() > 0);
}

