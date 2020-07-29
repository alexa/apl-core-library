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

#include "debugtools.h"
#include "apl/component/corecomponent.h"

namespace apl {

static std::map<std::string, std::function<std::string(const Component&)>> sPseudoProperties =
    {
        // Return the global bounding rectangle for this component.
        {"__global",  [](const Component& c) {
          return c.getGlobalBounds().toString();
        }},

        // Return the actual bounding rectangle for this component.  It has been clipped by all parents
        {"__actual",  [](const Component& c) {
          Rect r = c.getGlobalBounds();
          auto parent = c.getParent();
          while (parent) {
              r = r.intersect(
                  parent->getGlobalBounds());
              parent = parent->getParent();
          }
          return r.toString();
        }},

        // Return whether or not this component inherits state from the parent component
        {"__inherit", [](const Component& c) {
          return static_cast<const CoreComponent *>(&c)->getInheritParentState()
                 ? "true"
                 : "false";
        }},

        // Return the component scroll offset
        {"__scroll",  [](const Component& c) {
          return static_cast<const CoreComponent *>(&c)->scrollPosition().toString();
        }},

        // Return "true" if the component has non-zero opacity and should be drawn.  It may
        // not actually be on the screen.
        {"__visible", [](const Component& c) {
          if (c.getCalculated(kPropertyOpacity).getDouble() <= 0.0 ||
              c.getCalculated(kPropertyDisplay).getInteger() != kDisplayNormal)
              return "false";

          for (auto parent = c.getParent(); parent; parent = parent->getParent()) {
              if (parent->getCalculated(kPropertyOpacity).getDouble() <= 0.0 ||
                  parent->getCalculated(kPropertyDisplay).getInteger() != kDisplayNormal)
                  return "false";
          }

          return "true";
        }},

        // Return the provenance
        {"__path",    [](const Component& c) {
          return c.provenance();
        }},
    };


class HierarchyVisitor : public Visitor<CoreComponent> {
public:
    HierarchyVisitor(std::vector<std::string>&& args) : mIndent(0), mArgs(std::move(args)) {}

    void visit(const CoreComponent& component) override {
        std::string result = component.name() + component.getUniqueId();
        if (!component.getId().empty())
            result += "(" + component.getId() + ")";

        for (auto& m : mArgs) {
            if (sComponentPropertyBimap.has(m)) {
                PropertyKey key = static_cast<PropertyKey>(sComponentPropertyBimap.at(m));
                auto value = component.getCalculated(key);
                if (!value.empty())
                    result +=
                        std::string(" ") + m + ":" + component.getCalculated(key).toDebugString();
            }
            else {
                auto it = sPseudoProperties.find(m);
                if (it != sPseudoProperties.end())
                    result += std::string(" ") + m + ":" + it->second(component);
            }
        }
        LOG(LogLevel::DEBUG) << std::string(mIndent, ' ') << result;
    }

    void push() override { mIndent += 2; }

    void pop() override { mIndent -= 2; }

private:
    int mIndent;
    std::vector<std::string> mArgs;
};


void
dumpHierarchy(const ComponentPtr& component, std::initializer_list<std::string> args)
{
    HierarchyVisitor hv(args);
    std::dynamic_pointer_cast<CoreComponent>(component)->accept(hv);
}


void
dumpContext(const ContextPtr& context, int indent)
{
    for (auto it = context->begin(); it != context->end(); it++) {
        int upstream = context->countUpstream(it->first);
        int downstream = context->countDownstream(it->first);
        auto result = it->first + " := " + it->second.toDebugString();
        if (upstream)
            result += "[" + std::to_string(upstream) + " upstream]";
        if (downstream)
            result += "[" + std::to_string(downstream) + " downstream]";
        LOG(LogLevel::DEBUG) << std::string(indent, ' ') << result;
    }

    auto parent = context->parent();
    if (parent)
        dumpContext(parent, indent + 4);
}



} // namespace apl
