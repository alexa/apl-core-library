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

#include "testeventloop.h"

using namespace apl;


/**
 * This file contains some test cases to make sure our fake "SimpleTextMeasurement" class behaves correctly.
 * The simple text measurement class assumes that each and every character in a text string occupies exactly
 * one 10x10 block.
 *
 * Because TextMeasurement routines expect a Component pointer, we construct a fake component class that
 * only serves to return an Object containing the text.
 */


class FakeComponent : public Component {
public:
    FakeComponent(const ContextPtr& context, const std::string& id, const std::string& text)
        : Component(context, id)
    {
        mCalculated.set(kPropertyText, text);
    }

    void release() override {}
    size_t getChildCount() const override {return 0;}
    ComponentPtr getChildAt(size_t index) const override { return nullptr; }
    bool appendChild(const ComponentPtr& child) override { return false; }
    bool insertChild(const ComponentPtr& child, size_t index) override { return false; }
    bool remove() override { return false; }
    bool canInsertChild() const override { return false; }
    bool canRemoveChild() const override { return false; }
    ComponentType getType() const override { return kComponentTypeText; }
    ComponentPtr getParent() const override { return nullptr; }
    void update(UpdateType type, float value) override {}
    void update(UpdateType type, const std::string& value) override {}
    void ensureLayout(bool useDirtyFlag) override {}
    size_t getDisplayedChildCount() const override{return 0;}
    ComponentPtr getDisplayedChildAt(size_t drawIndex) const override { return nullptr; }
    std::string getHierarchySignature() const override { return std::string(); }
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override { return rapidjson::Value(); }
    rapidjson::Value serializeAll(rapidjson::Document::AllocatorType& allocator) const override { return rapidjson::Value(); }
    rapidjson::Value serializeDirty(rapidjson::Document::AllocatorType& allocator) override { return rapidjson::Value(); }
    std::string provenance() const override { return std::string(); }
    rapidjson::Value serializeVisualContext(rapidjson::Document::AllocatorType& allocator) override { return rapidjson::Value(); }
    ComponentPtr findComponentById(const std::string& id) const override { return nullptr; }
    ComponentPtr findComponentAtPosition(const Point& position) const override { return nullptr; }
};


class SimpleText : public ::testing::Test {
public:
    SimpleText()
        : Test()
    {
        measure = std::make_shared<SimpleTextMeasurement>();
        metrics = Metrics().size(100,100);
        config = RootConfig().measure(measure);
        context = Context::createTestContext(metrics, config);
    }

    ::testing::AssertionResult CheckSize(const ComponentPtr& component,
                                         float width, MeasureMode widthMode,
                                         float height, MeasureMode heightMode,
                                         float targetWidth, float targetHeight)
    {
        auto ls = measure->measure(component.get(), width, widthMode, height, heightMode);
        if (ls.width != targetWidth)
            return ::testing::AssertionFailure() << "width mismatch.  Expected " << targetWidth << " but got " << ls.width;
        if (ls.height != targetHeight)
            return ::testing::AssertionFailure() << "height mismatch.  Expected " << targetHeight << " but got " << ls.height;

        return ::testing::AssertionSuccess();
    }

public:
    std::shared_ptr<TextMeasurement> measure;
    ContextPtr context;
    Metrics metrics;
    RootConfig config;
};


TEST_F(SimpleText, Basic)
{
    // Empty text string should return size 0,0 whenever possible
    auto a = std::make_shared<FakeComponent>(context, "ID", "");
    ASSERT_TRUE(CheckSize(a, 100, MeasureMode::Exactly, 100, MeasureMode::Exactly, 100, 100));
    ASSERT_TRUE(CheckSize(a, -1, MeasureMode::Undefined, -1, MeasureMode::Undefined, 0, 0));
    ASSERT_TRUE(CheckSize(a, 100, MeasureMode::AtMost, 100, MeasureMode::AtMost, 0, 0));

    // Assign a larger block of text.
    a = std::make_shared<FakeComponent>(context, "ID", "123456789A");

    // When the width is fixed, the other modes depend on how much wrapper occurs
    ASSERT_TRUE(CheckSize(a, 37, MeasureMode::Exactly, 23, MeasureMode::Exactly, 37, 23));
    ASSERT_TRUE(CheckSize(a, 37, MeasureMode::Exactly, 100, MeasureMode::AtMost, 37, 40));
    ASSERT_TRUE(CheckSize(a, 37, MeasureMode::Exactly, 37, MeasureMode::AtMost, 37, 37));  // Clip some
    ASSERT_TRUE(CheckSize(a, 40, MeasureMode::Exactly, -1, MeasureMode::Undefined, 40, 30));
    ASSERT_TRUE(CheckSize(a, 20, MeasureMode::Exactly, -1, MeasureMode::Undefined, 20, 50));
    ASSERT_TRUE(CheckSize(a, 4, MeasureMode::Exactly, -1, MeasureMode::Undefined, 4, 0));   // Too narrow - no text

    // When the width is "at most", we need to check the various wrap conditions
    ASSERT_TRUE(CheckSize(a, 37, MeasureMode::AtMost, 100, MeasureMode::Exactly, 30, 100));
    ASSERT_TRUE(CheckSize(a, 137, MeasureMode::AtMost, 100, MeasureMode::Exactly, 100, 100));
    ASSERT_TRUE(CheckSize(a, 37, MeasureMode::AtMost, 37, MeasureMode::AtMost, 30, 37));
    ASSERT_TRUE(CheckSize(a, 40, MeasureMode::AtMost, 37, MeasureMode::AtMost, 40, 30));
    ASSERT_TRUE(CheckSize(a, 52, MeasureMode::AtMost, 37, MeasureMode::AtMost, 50, 20));
    ASSERT_TRUE(CheckSize(a, 137, MeasureMode::AtMost, 37, MeasureMode::AtMost, 100, 10));
    ASSERT_TRUE(CheckSize(a, 100, MeasureMode::AtMost, -1, MeasureMode::Undefined, 100, 10));
    ASSERT_TRUE(CheckSize(a, 2341, MeasureMode::AtMost, -1, MeasureMode::Undefined, 100, 10));
    ASSERT_TRUE(CheckSize(a, 23, MeasureMode::AtMost, -1, MeasureMode::Undefined, 20, 50));
    ASSERT_TRUE(CheckSize(a, 13, MeasureMode::AtMost, -1, MeasureMode::Undefined, 10, 100));
    ASSERT_TRUE(CheckSize(a, 3, MeasureMode::AtMost, -1, MeasureMode::Undefined, 0, 0));  // Too narrow - no text

    // When the width is undefined, the height will default to 10 (because the text will be laid out in a single line)
    ASSERT_TRUE(CheckSize(a, -1, MeasureMode::Undefined, 12, MeasureMode::Exactly, 100, 12));
    ASSERT_TRUE(CheckSize(a, -1, MeasureMode::Undefined, 3, MeasureMode::Exactly, 100, 3));
    ASSERT_TRUE(CheckSize(a, -1, MeasureMode::Undefined, 100, MeasureMode::AtMost, 100, 10));
    ASSERT_TRUE(CheckSize(a, -1, MeasureMode::Undefined, 5, MeasureMode::AtMost, 100, 5));
    ASSERT_TRUE(CheckSize(a, -1, MeasureMode::Undefined, -1, MeasureMode::Undefined, 100, 10));

    // Try to break things
    ASSERT_TRUE(CheckSize(a, 0, MeasureMode::AtMost, -1, MeasureMode::Undefined, 0, 0));
    ASSERT_TRUE(CheckSize(a, 0, MeasureMode::AtMost, 0, MeasureMode::AtMost, 0, 0));
}
