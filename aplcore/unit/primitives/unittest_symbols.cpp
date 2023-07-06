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

#include <algorithm>
#include <apl/utils/dump_object.h>

#include "../testeventloop.h"

#include "apl/primitives/boundsymbolset.h"
#include "apl/datagrammar/bytecode.h"

using namespace apl;

class SymbolTest : public MemoryWrapper {
protected:
    void SetUp() override {
        auto c1 = Context::createTestContext(Metrics(), RootConfig());
        c1->putUserWriteable("a", 23);
        c1->putUserWriteable("b", 1);
        c1->putUserWriteable("c", Object(std::vector<Object>{1,2,3}));

        auto c2 = Context::createFromParent(c1);
        c2->putUserWriteable("b", 2);

        auto map = std::make_shared<std::map<std::string, Object>>();
        map->emplace("name", "Sam");
        map->emplace("age", 102);
        map->emplace("friends", Object(std::vector<Object>{"Trixie", "Phool", "Sun"}));
        c2->putUserWriteable("d", map);

        contexts.emplace("CONTEXT_1", c1);
        contexts.emplace("CONTEXT_2", c2);

        base = c2;
    }

    void TearDown() override
    {
        contexts.clear();
        base = nullptr;
        MemoryWrapper::TearDown();
    }

    std::map<std::string, ContextPtr> contexts;
    ContextPtr base;
};



std::map<std::string, std::vector<std::pair<std::string, std::string>>> BASIC_TESTS = {
    {"${a}",                                                  {{"CONTEXT_1", "a"}}},
    {"${    a  }",                                            {{"CONTEXT_1", "a"}}},
    {"${a+b}",                                                {{"CONTEXT_1", "a"},   {"CONTEXT_2", "b"}}},
    {"${27+b}",                                               {{"CONTEXT_2", "b"}}},
    {"${a ? b : -1}",                                         {{"CONTEXT_1", "a"},   {"CONTEXT_2", "b"}}},
    {"${a ? -1 : b}",                                         {{"CONTEXT_1", "a"}}},
    {"${0 ? a : b}",                                          {{"CONTEXT_2", "b"}}},
    {"${1 ? a : b}",                                          {{"CONTEXT_1", "a"}}},
    {"${c[0] ? a : b}",                                       {{"CONTEXT_1", "a"},   {"CONTEXT_1", "c"}}},
    {"${0||b}",                                               {{"CONTEXT_2", "b"}}},
    {"${a||b}",                                               {{"CONTEXT_1", "a"}}},
    {"${1&&b}",                                               {{"CONTEXT_2", "b"}}},
    {"${a&&b}",                                               {{"CONTEXT_1", "a"},   {"CONTEXT_2", "b"}}},
    {"${Math.min(a,b)}",                                      {{"CONTEXT_1", "a"},   {"CONTEXT_2", "b"}}},
    {"${c}",                                                  {{"CONTEXT_1", "c"}}},
    {"${c[0]}",                                               {{"CONTEXT_1", "c"}}},
    {"${c[a]}",                                               {{"CONTEXT_1", "c"},   {"CONTEXT_1", "a"}}},
    {"${c[b]}",                                               {{"CONTEXT_1", "c"},   {"CONTEXT_2", "b"}}},
    {"${c[23 + c[b]]}",                                       {{"CONTEXT_2", "b"},   {"CONTEXT_1", "c"}}},
    {"${b} ${Math.min(24,c[a+1])}",                           {{"CONTEXT_1", "c"},   {"CONTEXT_1", "a"}, {"CONTEXT_2", "b"}}},
    {"${c[-1]}",                                              {{"CONTEXT_1", "c"}}},
    {"${d}",                                                  {{"CONTEXT_2", "d"}}},
    {"${d.name}",                                             {{"CONTEXT_2", "d"}}},
    {"${d['age']}",                                           {{"CONTEXT_2", "d"}}},
    {"${d.friends[-1]}",                                      {{"CONTEXT_2", "d"}}},
    {"${Math.random() + 1}",                                  {}},
    {"${d.friends[2+3]}",                                     {{"CONTEXT_2", "d"}}},
    {"${Math.random() * Math.random()}",                      {}},
    {"${d.friends[c[2]]}",                                    {{"CONTEXT_1", "c"},   {"CONTEXT_2", "d"}}},
    {"${c[d.friends.length - 2]}",                            {{"CONTEXT_1", "c"},   {"CONTEXT_2", "d"}}},
    {"${c[Math.round(2.3)]}",                                 {{"CONTEXT_1", "c"}}},
    {"${c[Math.random()]}",                                   {{"CONTEXT_1", "c"}}},
    {"${Math.max(Math.random(), Math.random())}",             {}},
    {"${c[Math.min(Math.random(), Math.random())]}",          {{"CONTEXT_1", "c"}}},
    {"${d.friends[Math.random()]}",                           {{"CONTEXT_2", "d"}}},
    {"${d.friends[Math.random()*d.friends.length]}",          {{"CONTEXT_2", "d"}}},
    {"${String.toUpperCase(d.friends[d.friends.length-1])}",  {{"CONTEXT_2", "d"}}},
    {"${c[2] + c.length + c[Math.random()]}",                 {{"CONTEXT_1", "c"}}},
    {"${c[Math.random()] + c.length + c[2]}",                 {{"CONTEXT_1", "c"}}},
    {"${Math.max(Math.min(1,a), Math.min(d.friends[2], b))}", {{"CONTEXT_1", "a"},   {"CONTEXT_2", "d"}, {"CONTEXT_2", "b"}}},
};

TEST_F(SymbolTest, Basic)
{
    for (const auto& m : BASIC_TESTS) {
        auto result = parseAndEvaluate(*base, m.first);
        ASSERT_EQ(m.second.size(), result.symbols.size()) << m.first;
        for (const auto& p : m.second) {
            auto it = std::find(result.symbols.begin(),
                                result.symbols.end(),
                                BoundSymbol{contexts.at(p.first), p.second});
            ASSERT_TRUE(it != result.symbols.end()) << m.first;
        }
    }
}

TEST_F(SymbolTest, BoundSymbol)
{
    auto bs = BoundSymbol{base, "a"};
    ASSERT_TRUE(bs.truthy());
    ASSERT_FALSE(bs.empty());
    ASSERT_TRUE(IsEqual(bs.toDebugString(), "BoundSymbol<a>"));

    bs = BoundSymbol{base, "missing"};
    ASSERT_FALSE(bs.truthy());
    ASSERT_TRUE(bs.empty());
    ASSERT_TRUE(IsEqual(bs.toDebugString(), "BoundSymbol<missing>"));
}
