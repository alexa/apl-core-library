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

#include "apl/focus/focusmanager.h"

// Test adding and remove components dynamically

using namespace apl;

class DynamicComponentTest : public DocumentWrapper {};

static const char *TEST_BASE = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": [
        {
          "type": "Frame",
          "id": "frame1",
          "width": 100,
          "height": 100
        },
        {
          "type": "Frame",
          "id": "frame2",
          "width": 100,
          "height": 100
        },
        {
          "type": "Frame",
          "id": "frame3",
          "width": 100,
          "height": 100
        }
      ]
    }
  }
})";

class DynamicComponentTestSimple : public DynamicComponentTest {
public:
    std::vector<ComponentPtr> frame;

    void init() {
        loadDocument(TEST_BASE);
        ASSERT_EQ(3, component->getChildCount());
        ASSERT_FALSE(component->needsLayout());

        for (int i = 0 ; i < component->getChildCount() ; i++)
            frame.emplace_back(component->getChildAt(i));

        ASSERT_TRUE(IsEqual(Rect(0,0,metrics.getWidth(), metrics.getHeight()), component->getCalculated(kPropertyBounds)));
        ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame[0]->getCalculated(kPropertyBounds)));
        ASSERT_TRUE(IsEqual(Rect(0,100,100,100), frame[1]->getCalculated(kPropertyBounds)));
        ASSERT_TRUE(IsEqual(Rect(0,200,100,100), frame[2]->getCalculated(kPropertyBounds)));
    }

    void TearDown() {
        frame.clear();
        DynamicComponentTest::TearDown();
    }
};

static const char *TEST_ELEMENT = R"({
  "type": "Frame",
  "width": 200,
  "height": 200
})";

TEST_F(DynamicComponentTestSimple, AddOnly)
{
    init();

    // Insert the child at a given offset
    JsonData data(TEST_ELEMENT);
    auto child = component->getContext()->inflate(data.get());
    ASSERT_TRUE(child);
    ASSERT_TRUE(component->insertChild(child, 0));
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();  // Forces the layout
    ASSERT_TRUE(IsEqual(Rect(0,0,metrics.getWidth(), metrics.getHeight()), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,200,200), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,200,100,100), frame[0]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,300,100,100), frame[1]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,400,100,100), frame[2]->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(component,kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[0], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[1], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[2], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child,frame[0], frame[1], frame[2]));
}

TEST_F(DynamicComponentTestSimple, InsertInMiddle)
{
    init();

    // Insert the child at a given offset
    JsonData data(TEST_ELEMENT);
    auto child = component->getContext()->inflate(data.get());
    ASSERT_TRUE(child);
    ASSERT_TRUE(component->insertChild(child, 2));
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();  // Forces the layout
    ASSERT_TRUE(IsEqual(Rect(0,0,metrics.getWidth(), metrics.getHeight()), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,200,200,200), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame[0]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,100,100,100), frame[1]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,400,100,100), frame[2]->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(component,kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[0]));
    ASSERT_TRUE(CheckDirty(frame[1]));
    ASSERT_TRUE(CheckDirty(frame[2], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child, frame[2]));
}

TEST_F(DynamicComponentTestSimple, InsertAtEnd)
{
    init();

    // Insert the child at a given offset
    JsonData data(TEST_ELEMENT);
    auto child = component->getContext()->inflate(data.get());
    ASSERT_TRUE(child);
    ASSERT_TRUE(component->appendChild(child));
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();  // Forces the layout
    ASSERT_TRUE(IsEqual(Rect(0,0,metrics.getWidth(), metrics.getHeight()), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,300,200,200), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame[0]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,100,100,100), frame[1]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,200,100,100), frame[2]->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(component,kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[0]));
    ASSERT_TRUE(CheckDirty(frame[1]));
    ASSERT_TRUE(CheckDirty(frame[2]));
    ASSERT_TRUE(CheckDirty(root, component, child));
}

TEST_F(DynamicComponentTestSimple, RemoveFront)
{
    init();

    ASSERT_TRUE(frame[0]->remove());
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();  // Forces the layout
    ASSERT_TRUE(IsEqual(Rect(0,0,metrics.getWidth(), metrics.getHeight()), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame[1]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,100,100,100), frame[2]->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame[1], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[2], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, frame[1], frame[2]));
}

TEST_F(DynamicComponentTestSimple, RemoveMiddle)
{
    init();

    ASSERT_TRUE(frame[1]->remove());
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();  // Forces the layout
    ASSERT_TRUE(IsEqual(Rect(0,0,metrics.getWidth(), metrics.getHeight()), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame[0]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,100,100,100), frame[2]->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame[2], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, frame[2]));
}

TEST_F(DynamicComponentTestSimple, RemoveEnd)
{
    init();

    ASSERT_TRUE(frame[2]->remove());
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();  // Forces the layout
    ASSERT_TRUE(IsEqual(Rect(0,0,metrics.getWidth(), metrics.getHeight()), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame[0]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,100,100,100), frame[1]->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component));
}


TEST_F(DynamicComponentTestSimple, AddAndRemove)
{
    init();

    // Insert the child at a given offset
    JsonData data(TEST_ELEMENT);
    auto child = component->getContext()->inflate(data.get());

    ASSERT_TRUE(child);
    ASSERT_TRUE(component->insertChild(child, 0));
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();  // Forces the layout
    ASSERT_TRUE(IsEqual(Rect(0,0,metrics.getWidth(), metrics.getHeight()), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,200,200), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,200,100,100), frame[0]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,300,100,100), frame[1]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,400,100,100), frame[2]->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(component,kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child,
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[0], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[1], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[2], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child,frame[0], frame[1], frame[2]));

    // Remove the child
    ASSERT_TRUE(child->remove());
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();  // Forces the layout
    ASSERT_TRUE(IsEqual(Rect(0,0,metrics.getWidth(), metrics.getHeight()), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame[0]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,100,100,100), frame[1]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,200,100,100), frame[2]->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child));
    ASSERT_TRUE(CheckDirty(frame[0], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[1], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[2], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, frame[0], frame[1], frame[2]));
}

TEST_F(DynamicComponentTestSimple, AddAndMove)
{
    init();

    // Insert the child at a given offset
    JsonData data(TEST_ELEMENT);
    auto child = component->getContext()->inflate(data.get());
    ASSERT_TRUE(child);
    ASSERT_TRUE(component->insertChild(child, 0));
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();  // Forces the layout
    ASSERT_TRUE(IsEqual(Rect(0,0,metrics.getWidth(), metrics.getHeight()), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,200,200), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,200,100,100), frame[0]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,300,100,100), frame[1]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,400,100,100), frame[2]->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child,
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[0], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[1], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[2], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child,frame[0], frame[1], frame[2]));

    // Move the child to a new location
    ASSERT_TRUE(child->remove());
    ASSERT_TRUE(component->insertChild(child, 2));
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();  // Forces the layout
    ASSERT_TRUE(IsEqual(Rect(0,0,metrics.getWidth(), metrics.getHeight()), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame[0]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,100,100,100), frame[1]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,200,200,200), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,400,100,100), frame[2]->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child, kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[0], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[1], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[2]));  // It didn't move
    ASSERT_TRUE(CheckDirty(root, component, child, frame[0], frame[1]));
}

const static char * CHILD_WITH_LAYOUT_PROPERTIES = R"({
  "type": "Frame",
  "width": 200,
  "height": 200,
  "grow": 1,
  "alignSelf": "center"
})";

TEST_F(DynamicComponentTestSimple, LayoutProperties)
{
    init();

    JsonData data(CHILD_WITH_LAYOUT_PROPERTIES);
    auto child = context->inflate(data.get());
    ASSERT_TRUE(child);

    // Insert it into the layout
    ASSERT_TRUE(component->insertChild(child, 1));
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();  // Forces the layout
    auto height = metrics.getHeight();
    auto width = metrics.getWidth();
    ASSERT_TRUE(IsEqual(Rect(0,0,width, height), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame[0]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect((width - 200) / 2,100,200,height - 300), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,height - 200,100,100), frame[1]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,height - 100,100,100), frame[2]->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(component,kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child,
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[1], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[2], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child, frame[1], frame[2]));  // frame[0] didn't move

    // Move the child to a new location
    ASSERT_TRUE(child->remove());
    ASSERT_TRUE(component->insertChild(child, 2));
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();  // Forces the layout
    ASSERT_TRUE(IsEqual(Rect(0,0,metrics.getWidth(), metrics.getHeight()), component->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame[0]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,100,100,100), frame[1]->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect((width-200)/2,200,200,height-300), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,height-100,100,100), frame[2]->getCalculated(kPropertyBounds)));

    // frame[1] and child swapped places
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child, kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[1], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child, frame[1]));
}

static const char *HIERARCHY = R"({
  "type": "TouchWrapper",
  "checked": true,
  "width": 200,
  "height": 200,
  "items": {
    "type": "Text",
    "id": "myText",
    "text": "Hello"
  }
})";

TEST_F(DynamicComponentTestSimple, AddHierarchy)
{
    init();

    JsonData data(HIERARCHY);
    auto child = context->inflate(data.get());
    ASSERT_TRUE(child);

    ASSERT_TRUE(component->insertChild(child, 1));
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0,100,200,200), child->getCalculated(kPropertyBounds)));

    // Running layout updates the bounds of the attached children
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child,
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(child->getChildAt(0), kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child, child->getChildAt(0), frame[1], frame[2]));

    // Move the hierarchy to a new spot
    ASSERT_TRUE(child->remove());
    ASSERT_TRUE(component->appendChild(child));

    ASSERT_TRUE(component->needsLayout());
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0,300,200,200), child->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child, kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child, frame[1], frame[2]));  // frame[0] and the embedded Text don't change

    ASSERT_TRUE(child->remove());
    child->release();   // If you don't call this, it won't get cleaned up.
}

// Remove a dirty component and verify that it is removed from the dirty list.
// Re-add that component and verify that the dirty list is retained
TEST_F(DynamicComponentTestSimple, MoveDirty)
{
    init();

    JsonData data(HIERARCHY);
    auto child = context->inflate(data.get());
    auto text = child->getChildAt(0);
    ASSERT_TRUE(child);
    ASSERT_TRUE(text);

    ASSERT_TRUE(component->appendChild(child));
    root->clearPending();
    root->clearDirty();

    // Now change the text. This should mark it as dirty
    executeCommand("SetValue", {{"componentId", "myText"}, {"property", "text"}, {"value", "foobar"}}, false);
    ASSERT_TRUE(CheckDirtyDoNotClear(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirtyDoNotClear(root, text));

    // Without clearing the dirty flags, remove the component
    ASSERT_TRUE(child->remove());
    root->clearPending();

    // After the layout, the fact that text is dirty should no longer be visible
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component));

    // Now add the child back in and it should re-add itself to the dirty list
    ASSERT_TRUE(component->appendChild(child));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child));   // Nothing changed for the child; not even the property bounds
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, text));
}

// Verify that focus is released when a component is removed
TEST_F(DynamicComponentTestSimple, Focus)
{
    init();

    JsonData data(HIERARCHY);
    auto child = context->inflate(data.get());
    ASSERT_TRUE(child);

    ASSERT_TRUE(component->insertChild(child, 1));
    root->clearPending();
    clearDirty();

    // Set the focus
    child->update(kUpdateTakeFocus, 1);
    auto& fm = context->focusManager();

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_FALSE(root->hasEvent());  // We don't get a focus event because it was viewhost-instigated

    ASSERT_TRUE(CheckDirty(root));  // Nothing got redrawn

    // Now remove the hierarchy
    ASSERT_TRUE(child->remove());
    root->clearPending();

    ASSERT_FALSE(fm.getFocus());  // Focus should be cleared
    ASSERT_TRUE(root->hasEvent());  // We get an unfocus event
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_FALSE(event.getComponent());

    // The detached component should be unfocused
    ASSERT_FALSE(std::static_pointer_cast<CoreComponent>(child)->getState().get(kStateFocused));

    // The children property will be dirty
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component, frame[1], frame[2]));

    child->release();   // If you don't call this, it won't get cleaned up.
}

// Make sure a detached component can't be found with findComponentById
TEST_F(DynamicComponentTestSimple, FindById)
{
    init();

    ASSERT_FALSE(context->findComponentById("myText"));

    JsonData data(HIERARCHY);
    auto child = context->inflate(data.get());
    auto text = child->getChildAt(0);
    ASSERT_TRUE(child);
    ASSERT_TRUE(text);

    // Stuff it into the hierarchy
    ASSERT_TRUE(component->insertChild(child, 1));
    root->clearPending();
    clearDirty();

    // Verify that we find the text component
    ASSERT_EQ(text, context->findComponentById("myText"));

    // Pull it out of the hierarchy
    child->remove();
    ASSERT_FALSE(context->findComponentById("myText"));

    child->release();
}

TEST_F(DynamicComponentTestSimple, FindByUID)
{
    init();

    JsonData data(HIERARCHY);
    auto child = context->inflate(data.get());
    auto text = child->getChildAt(0);
    ASSERT_TRUE(child);
    ASSERT_TRUE(text);

    // Stuff it into the hierarchy
    ASSERT_TRUE(component->insertChild(child, 1));
    root->clearPending();
    clearDirty();

    // Verify that we find the text component
    ASSERT_EQ(text, context->findComponentById(text->getUniqueId()));

    // Pull it out of the hierarchy
    child->remove();
    ASSERT_FALSE(context->findComponentById(text->getUniqueId()));

    child->release();
}

static const char *HIERARCHY_INHERIT = R"({
  "type": "TouchWrapper",
  "checked": true,
  "inheritParentState": true,
  "width": 200,
  "height": 200,
  "items": {
    "type": "Text",
    "text": "Hello",
    "inheritParentState": true
  }
})";

// Test what happens when you add and remove a hierarchy that has inherit parent state set
TEST_F(DynamicComponentTestSimple, AddHierarchyInherit)
{
    init();

    JsonData data(HIERARCHY_INHERIT);
    auto child = context->inflate(data.get());
    auto text = child->getChildAt(0);
    ASSERT_TRUE(child);
    ASSERT_TRUE(text);

    // Note that both components start with the checked state (one is inherited)
    ASSERT_TRUE(IsEqual(true, child->getCalculated(kPropertyChecked)));
    ASSERT_TRUE(IsEqual(true, text->getCalculated(kPropertyChecked)));

    ASSERT_TRUE(component->insertChild(child, 1));
    ASSERT_TRUE(component->needsLayout());

    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0,100,200,200), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(false, child->getCalculated(kPropertyChecked)));
    ASSERT_TRUE(IsEqual(false, text->getCalculated(kPropertyChecked)));

    // Running layout updates the bounds of the attached children
    // This also propagates checked.
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child,
                           kPropertyBounds, kPropertyInnerBounds, kPropertyChecked, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(text, kPropertyBounds, kPropertyInnerBounds, kPropertyChecked, kPropertyLaidOut,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child, text, frame[1], frame[2]));

    // Disconnect the hierarchy and attach elsewhere
    ASSERT_TRUE(child->remove());
    component->setProperty(kPropertyChecked, true);
    clearDirty();

    component->appendChild(child);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0,300,200,200), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(true, child->getCalculated(kPropertyChecked)));
    ASSERT_TRUE(IsEqual(true, text->getCalculated(kPropertyChecked)));

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child, kPropertyBounds, kPropertyChecked, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(text, kPropertyChecked));
    ASSERT_TRUE(CheckDirty(root, component, child, text));
}

TEST_F(DynamicComponentTestSimple, AddAlreadyAttached)
{
    init();

    ASSERT_FALSE(component->insertChild(frame[1], 2));
}

TEST_F(DynamicComponentTestSimple, RemoveUnattached)
{
    init();

    // Insert the child at a given offset
    JsonData data(TEST_ELEMENT);
    auto child = component->getContext()->inflate(data.get());
    ASSERT_TRUE(child);

    ASSERT_FALSE(child->remove());

    child->release();
}

static const char *SEQUENCE = R"({
  "type": "APL",
  "version": "1.1",
  "layouts": {
    "Box":{
      "parameters": [ "label" ],
      "items": {
        "type": "Frame",
        "id": "frame${label}",
        "width": 100,
        "height": 300
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "width": "100%",
      "height": "100%",
      "items": [
          { "type": "Box", "label": 1 },
          { "type": "Box", "label": 2 },
          { "type": "Box", "label": 3 },
          { "type": "Box", "label": 4 },
          { "type": "Box", "label": 5 },
          { "type": "Box", "label": 6 },
          { "type": "Box", "label": 7 },
          { "type": "Box", "label": 8 },
          { "type": "Box", "label": 9 }
      ]
    }
  }
})";

static const char *SEQUENCE_COMPONENT = R"({
  "type": "Frame",
  "width": 200,
  "height": 200,
  "spacing": 40
})";

TEST_F(DynamicComponentTest, Sequence)
{
    metrics.size(1000,1000);
    loadDocument(SEQUENCE);
    ASSERT_TRUE(component);

    std::vector<ComponentPtr> frame;
    for (auto i = 0 ; i < component->getChildCount() ; i++)
        frame.emplace_back(component->getChildAt(i));

    // Make sure that the first four are attached (these are the visible ones)
    root->clearDirty();

    JsonData data(SEQUENCE_COMPONENT);
    auto child = context->inflate(data.get());
    ASSERT_TRUE(child);

    component->insertChild(child, 1);
    root->clearPending();

    ASSERT_TRUE(IsEqual(Rect(0,340,200,200), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[1], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[2], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirtyAtLeast(root, component, child, frame[1], frame[2], frame[3]));  // frame[0] was skipped over

    child->remove();
    root->clearPending();
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child));
    ASSERT_TRUE(CheckDirty(frame[1], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(frame[2], kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirtyAtLeast(root, component, frame[1], frame[2], frame[3]));  // frame[0] was skipped over
}

// Add a child far out in the sequence and verify that it doesn't get attached
TEST_F(DynamicComponentTest, SequenceFarOut)
{
    metrics.size(1000,1000);
    loadDocument(SEQUENCE);
    ASSERT_TRUE(component);

    std::vector<ComponentPtr> frame;
    for (auto i = 0 ; i < component->getChildCount() ; i++)
        frame.emplace_back(component->getChildAt(i));

    // Make sure that the first four are attached (these are the visible ones)
    root->clearDirty();

    JsonData data(SEQUENCE_COMPONENT);
    auto child = context->inflate(data.get());
    ASSERT_TRUE(child);

    component->insertChild(child, 8);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component));  // Nothing changed on the screen

    child->remove();
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component));  // Nothing changed on the screen
}

static const char *TWO_CONTAINERS = R"({
  "type": "APL",
  "version": "1.1",
  "layouts": {
    "Box":{
      "parameters": [ "label" ],
      "items": {
        "type": "Frame",
        "id": "frame${label}",
        "width": 100,
        "height": 100
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "height": "100%",
      "width": "100%",
      "items": [
        {
          "type": "Container",
          "id": "myContainer",
          "height": "50%",
          "width": "100%",
          "direction": "row",
          "items": [
            { "type": "Box", "label": 1 },
            { "type": "Box", "label": 2 },
            { "type": "Box", "label": 3 }
          ]
        },
        {
          "type": "Sequence",
          "id": "mySequence",
          "scrollDirection": "horizontal",
          "height": "50%",
          "width": "100%",
          "items": [
            { "type": "Box", "label": 4 },
            { "type": "Box", "label": 5 },
            { "type": "Box", "label": 6 }
          ]
        }
      ]
    }
  }
})";

static const char *MIXED_COMPONENT = R"({
  "type": "Frame",
  "width": 200,
  "height": 200,
  "spacing": 40,
  "alignSelf": "end"
})";

// Move a component between two containers.
TEST_F(DynamicComponentTest, MoveBetween)
{
    loadDocument(TWO_CONTAINERS);
    ASSERT_TRUE(component);
    auto height = metrics.getHeight();

    auto container = component->findComponentById("myContainer");
    auto sequence = component->findComponentById("mySequence");
    ASSERT_TRUE(container);
    ASSERT_TRUE(sequence);

    JsonData data(MIXED_COMPONENT);
    auto child = context->inflate(data.get());
    ASSERT_TRUE(child);

    ASSERT_TRUE(container->insertChild(child, 1));
    root->clearPending();

    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), container->getChildAt(0)->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(140,height/2-200,200,200), child->getCalculated(kPropertyBounds)));

    ASSERT_TRUE(CheckDirty(container, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(child, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(container->getChildAt(2), kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(container->getChildAt(3), kPropertyBounds, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, container, child, container->getChildAt(2), container->getChildAt(3)));

    // Now move it to the sequence
    child->remove();
    sequence->insertChild(child, 1);
    root->clearPending();

    ASSERT_TRUE(IsEqual(Rect(140,0,200,200), child->getCalculated(kPropertyBounds)));
}

static const char *PAGER = R"({
  "type": "APL",
  "version": "1.1",
  "layouts": {
    "Box":{
      "parameters": [ "label" ],
      "items": {
        "type": "Frame",
        "id": "frame${label}",
        "width": 100,
        "height": 100
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "id": "myPager",
      "height": "100%",
      "width": "100%",
      "items": [
        { "type": "Box", "label": 1 },
        { "type": "Box", "label": 2 },
        { "type": "Box", "label": 3 }
      ]
    }
  }
})";

static const char *TEST_PAGER_ELEMENT = R"({
  "type": "Frame",
  "width": 200,
  "height": 200
})";

TEST_F(DynamicComponentTest, Pager)
{
    metrics.size(600, 500);
    loadDocument(PAGER);
    ASSERT_TRUE(component);
    advanceTime(10);
    root->clearDirty();

    JsonData data(TEST_PAGER_ELEMENT);
    auto child = context->inflate(data.get());
    ASSERT_TRUE(child);

    ASSERT_TRUE(component->insertChild(child, 1));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component));   // We haven't moved to the page, so it is not dirty
    ASSERT_TRUE(IsEqual(Rect(0,0,0,0), child->getCalculated(kPropertyBounds)));

    // Move forward one page - the child should be laid out and visible now
    component->update(kUpdatePagerByEvent, 1);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0,0,600,500), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(CheckDirty(child, kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut,
                           kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component, child));

    // Now move it to the first item
    child->remove();
    component->insertChild(child,0);
    root->clearPending();

    ASSERT_EQ(child, component->getChildAt(0));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyCurrentPage));
    ASSERT_EQ(1, component->pagePosition());
    ASSERT_TRUE(CheckDirty(child));  // Child doesn't change size
    ASSERT_TRUE(CheckDirty(root, component));
}


static const char *FRAME = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "height": "100%",
      "width": "100%",
      "items": {
        "type": "Text",
        "id": "myText",
        "width": 100,
        "height": 100
      }
    }
  }
})";

static const char *TEST_FRAME_ELEMENT = R"({
  "type": "Text",
  "width": 200,
  "height": 200
})";

TEST_F(DynamicComponentTest, Frame)
{
    loadDocument(FRAME);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(0);
    ASSERT_TRUE(text);

    JsonData data(TEST_FRAME_ELEMENT);
    auto child = context->inflate(data.get());
    ASSERT_TRUE(child);

    // Can't add the child into the frame because it is occupied
    ASSERT_FALSE(component->insertChild(child,1));
    ASSERT_FALSE(component->insertChild(child,0));

    // Can't add the child to the text - it doesn't support children
    ASSERT_FALSE(text->appendChild(child));

    // Once we remove the text, we can add the new child
    text->remove();
    ASSERT_EQ(0, component->getChildCount());

    ASSERT_TRUE(component->appendChild(child));
    root->clearPending();
    ASSERT_EQ(child, component->getChildAt(0));

    // Now we can't re-add the old text
    ASSERT_FALSE(component->appendChild(text));
    text->release();
}

static const char *REBUILDER = R"({
  "type": "APL",
  "version": "1.2",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "data": "${TestArray}",
      "items": {
        "type": "Text",
        "text": "${data}"
      }
    }
  }
})";

// A component using a LayoutRebuilder or data based inflation blocks normal add/remove commands
TEST_F(DynamicComponentTest, AddRemoveBlocking)
{
    auto myArray = LiveArray::create(ObjectArray{"A", "B", "C"});
    config->liveData("TestArray", myArray);

    loadDocument(REBUILDER);
    ASSERT_TRUE(component);
    ASSERT_EQ(3, component->getChildCount());
    ASSERT_TRUE(IsEqual("A", component->getChildAt(0)->getCalculated(kPropertyText).asString()));

    JsonData data(TEST_FRAME_ELEMENT);
    auto child = context->inflate(data.get());

    ASSERT_FALSE(component->canInsertChild());
    ASSERT_FALSE(component->insertChild(child, 0));
    ASSERT_FALSE(component->appendChild(child));

    ASSERT_FALSE(component->canRemoveChild());
    ASSERT_FALSE(component->getChildAt(0)->remove());

    ASSERT_EQ(3, component->getChildCount());
}
