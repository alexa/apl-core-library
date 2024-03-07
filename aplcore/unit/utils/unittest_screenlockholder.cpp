/*
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
 *
 */

#include "../testeventloop.h"
#include "apl/utils/screenlockholder.h"

using namespace apl;

class ScreenLockHolderTest : public DocumentWrapper {};

static const char *SIMPLE_DOCUMENT = R"apl(
{
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "id": "FRAME"
    }
  }
}
)apl";

TEST_F(ScreenLockHolderTest, Basic)
{
    loadDocument(SIMPLE_DOCUMENT);
    ASSERT_FALSE(root->screenLock());

    // Create a static block.  When this block terminates the lock is released.
    {
        ScreenLockHolder lock(component->getContext());
        ASSERT_FALSE(root->screenLock());

        lock.take();
        ASSERT_TRUE(root->screenLock());
    }

    ASSERT_FALSE(root->screenLock());
}

TEST_F(ScreenLockHolderTest, MultipleTakes)
{
    loadDocument(SIMPLE_DOCUMENT);
    ASSERT_FALSE(root->screenLock());

    // Create a static block.  When this block terminates the lock is released.
    {
        ScreenLockHolder lock(component->getContext());
        ASSERT_FALSE(root->screenLock());

        lock.take();
        ASSERT_TRUE(root->screenLock());

        lock.take();  // Take a second time
        ASSERT_TRUE(root->screenLock());
    }

    ASSERT_FALSE(root->screenLock());
}

TEST_F(ScreenLockHolderTest, MultipleReleases)
{
    loadDocument(SIMPLE_DOCUMENT);
    ASSERT_FALSE(root->screenLock());

    // Create a static block.  When this block terminates the lock is released.
    {
        ScreenLockHolder lock(component->getContext());
        ASSERT_FALSE(root->screenLock());

        lock.take();
        ASSERT_TRUE(root->screenLock());

        lock.take();     // Take it a second time
        ASSERT_TRUE(root->screenLock());

        lock.release();  // Release it and it should still be gone
        ASSERT_FALSE(root->screenLock());

        lock.release();  // Release it again; it should still be gone
        ASSERT_FALSE(root->screenLock());

        lock.take();     // Take it back; it should be there (the double release didn't do anything odd)
        ASSERT_TRUE(root->screenLock());
    }

    ASSERT_FALSE(root->screenLock());
}


TEST_F(ScreenLockHolderTest, Ensure)
{
    loadDocument(SIMPLE_DOCUMENT);
    ASSERT_FALSE(root->screenLock());

    // Create a static block.  When this block terminates the lock is released.
    {
        ScreenLockHolder lock(component->getContext());
        ASSERT_FALSE(root->screenLock());

        lock.ensure(true);
        ASSERT_TRUE(root->screenLock());

        lock.ensure(false);
        ASSERT_FALSE(root->screenLock());

        lock.ensure(false);
        ASSERT_FALSE(root->screenLock());

        lock.ensure(true);
        ASSERT_TRUE(root->screenLock());

        lock.ensure(true);
        ASSERT_TRUE(root->screenLock());
    }

    ASSERT_FALSE(root->screenLock());
}