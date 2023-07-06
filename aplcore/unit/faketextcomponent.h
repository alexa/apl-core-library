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

#include "apl/component/component.h"

using namespace apl;

class FakeTextComponent : public Component {
public:
    FakeTextComponent(const ContextPtr& context, const std::string& id, const std::string& text)
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
    Point localToGlobal(Point) const override { return {0,0}; }
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