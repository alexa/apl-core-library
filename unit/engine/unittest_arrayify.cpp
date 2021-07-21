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

class Arrayify : public MemoryWrapper {
public:
    Arrayify() : MemoryWrapper() {
        context = Context::createTestContext(Metrics(), session);
    }

    ContextPtr context;

    void TearDown() override
    {
        context = nullptr;
        MemoryWrapper::TearDown();
    }
};

static const char *TEST_ARRAY = "[\"a\", \"b\", \"c\"]";

TEST_F(Arrayify, BasicArrayify)
{
    // Construct a JSON array
    rapidjson::Document doc;
    rapidjson::ParseResult doc_okay = doc.Parse(TEST_ARRAY);
    ASSERT_TRUE(doc_okay);

    // This version results in ConstJSONArray
    auto result = arrayify(doc);
    ASSERT_EQ(3, result.size());
    ASSERT_TRUE(result.begin()->IsString());
    ASSERT_STREQ("a", result.begin()->GetString());
    ASSERT_STREQ("c", result[2].GetString());
}

static const std::vector<std::string> SHORT_TESTS = {
    "{ \"extend\": \"toucan\"}",
    "{ \"extends\": \"toucan\"}",
    "{ \"extend\": [\"toucan\"]}",
    "{ \"extends\": [\"toucan\"]}"
};

TEST_F(Arrayify, Short)
{
    for (const auto& s : SHORT_TESTS) {
        rapidjson::Document d;
        d.Parse(s.c_str());

        EXPECT_TRUE(d.IsObject());
        int count = 0;
        for (const auto& m : arrayifyProperty(d, "extend", "extends")) {
            EXPECT_TRUE(m.IsString());
            ASSERT_STREQ("toucan", m.GetString());
            count++;
        }
        EXPECT_EQ(1, count);
    }
}

static const std::vector<std::string> MISSING_TEST = {
    "{}",
    "{ \"nope\": \"toucan\"}",
    "{ \"extend\": []}",
    "{ \"extends\": []}"
};

TEST_F(Arrayify, Missing)
{
    for (const auto& s : MISSING_TEST) {
        rapidjson::Document d;
        d.Parse(s.c_str());

        EXPECT_TRUE(d.IsObject());
        auto res = arrayifyProperty(d, "extend", "extends");
        EXPECT_EQ(0, res.size());
    }
}

static const std::vector<std::string> LONG_TESTS = {
    "{ \"extend\": [\"toucan\", \"parrot\"]}",
    "{ \"extends\": [\"toucan\", \"parrot\"]}"
};

TEST_F(Arrayify, Long)
{
    for (const auto& s : LONG_TESTS) {
        rapidjson::Document d;
        d.Parse(s.c_str());

        EXPECT_TRUE(d.IsObject());
        int count = 0;
        for (const auto& m : arrayifyProperty(d, "extend", "extends")) {
            EXPECT_TRUE(m.IsString());
            count++;
        }
        EXPECT_EQ(2, count);
    }
}

static const char *CONTEXT_ARRAY = "[\"a\", \"b\", \"c\"]";

TEST_F(Arrayify, EvaluateContext)
{
    // Construct a JSON array
    rapidjson::Document doc;
    rapidjson::ParseResult doc_okay = doc.Parse(CONTEXT_ARRAY);
    ASSERT_TRUE(doc_okay);

    context->putConstant("payload", doc);

    // This version results in ["a", "b", "c"]
    auto result = arrayify(*context, "${payload}");
    ASSERT_EQ(3, result.size());
    ASSERT_TRUE(result.begin()->isString());
    ASSERT_STREQ("a", result.begin()->getString().c_str());
    ASSERT_STREQ("c", result[2].getString().c_str());

    // This version results in ["fuzzy"]
    result = arrayify(*context, "${'fuzzy'}");
    ASSERT_EQ(1, result.size());
    ASSERT_TRUE(result.begin()->isString());
    ASSERT_STREQ("fuzzy", result.begin()->getString().c_str());

    // This version should give ["x", "a", "b", "c", "z"]
    Object v(std::vector<Object>({"x", "${payload}", "z"}));
    result = arrayify(*context, v);
    ASSERT_EQ(5, result.size());
    ASSERT_TRUE(result.begin()->isString());
    ASSERT_STREQ("x", result[0].getString().c_str());
    ASSERT_STREQ("b", result[2].getString().c_str());
}

static const char *CONTEXT_ARRAY_2 = "[\"a\", \"b\", \"${payload}\", \"c\"]";

std::vector<std::pair<std::string, int> > CONTEXT_LONG_TESTS = {
    { "{ \"extend\": [\"${payload}\", \"parrot\"]}", 5 },
    { "{ \"extends\": [\"toucan\", \"${payload}\"]}", 5 },
    { "{ \"extend\": \"${payload}\" }", 4 },
    { "{ \"nope\": [ \"a\" ] }", 0}
};

TEST_F(Arrayify, ContextArrayifyProperty)
{
    // Construct a JSON array
    rapidjson::Document doc;
    rapidjson::ParseResult doc_okay = doc.Parse(CONTEXT_ARRAY_2);
    ASSERT_TRUE(doc_okay);

    context->putConstant("payload", doc);

    for (const auto& p : CONTEXT_LONG_TESTS) {
        rapidjson::Document d;
        rapidjson::ParseResult okay = d.Parse(p.first.c_str());
        ASSERT_TRUE(okay);

        auto result = arrayifyProperty(*context, d, "extend", "extends");
        ASSERT_EQ(p.second, result.size());
    }
}

static const char *COMMAND_ARRAY =
    "{"
    "  \"commands\": ["
    "    {"
    "      \"type\": \"SendEvent\","
    "      \"arguments\": \"Start\""
    "    },"
    "    \"${payload}\","
    "    {"
    "      \"type\": \"SendEvent\","
    "      \"arguments\": \"End\""
    "    }"
    "  ]"
    "}";

static const char *COMMAND_ARGS =
    "{"
    "  \"type\": \"SendEvent\","
    "  \"arguments\": \"Middle\""
    "}";

TEST_F(Arrayify, ExtendCommands)
{
    rapidjson::Document doc;
    rapidjson::ParseResult doc_okay = doc.Parse(COMMAND_ARGS);
    ASSERT_TRUE(doc_okay);
    context->putConstant("payload", doc);

    rapidjson::Document array;
    ASSERT_TRUE(static_cast<rapidjson::ParseResult>(array.Parse(COMMAND_ARRAY)));

    auto result = arrayifyProperty(*context, array, "command", "commands");

    ASSERT_EQ(3, result.size());

    auto argValues = std::vector<Object>{"Start", "Middle", "End"};
    auto iter = argValues.begin();

    for (auto v : result) {
        ASSERT_TRUE(v.isMap());
        ASSERT_EQ(Object("SendEvent"), v.get("type"));
        ASSERT_EQ(*iter, v.get("arguments"));
        iter++;
    }
}

static const std::vector<std::pair<std::string, Object>> BINDINGS = {
    { "a", "fuzzy duck" },
    { "b", std::vector<Object>{"a", "b"} },
    { "c", "This is a ${a}" },
};

using map_so = std::map<std::string, Object>;
using il_so = std::initializer_list<map_so::value_type>;

static const std::vector<std::pair<Object, std::vector<Object>>> SHALLOW_TEST_CASES = {
    { 23, { 23 } },
    { "random string", { "random string" } },
    { "${a}", { "fuzzy duck" }},
    { "${b}", { "a", "b" }},
    { "${c}", { "This is a ${a}" }},
    { std::vector<Object>{1, 2, "${a}"}, { 1, 2, "fuzzy duck" }},
    { std::vector<Object>{"${b}", "${b}"}, { "a", "b", "a", "b" }},
    { std::make_shared<map_so>(il_so{{"name", "${a}"}}), { std::make_shared<map_so>(il_so{{"name", "${a}"}})}},
};

TEST_F(Arrayify, ShallowArrayify)
{
    // auto c = Context::create(Metrics());

    for (const auto& m : BINDINGS)
        context->putConstant(m.first, m.second);

    for (auto m : SHALLOW_TEST_CASES)
        ASSERT_TRUE(IsEqual(std::move(m.second), arrayify(*context, m.first))) << m.first;
}

/**
 * In the deep test cases we first array-ify the object and then we recursively expand all data-binding
 * expressions.
 */
static const std::vector<std::pair<Object, std::vector<Object>>> DEEP_TEST_CASES = {
    { 23, { 23 } },
    { "random string", { "random string" } },
    { "${a}", { "fuzzy duck" }},
    { "${b}", { "a", "b" }},
    { "${c}", { "This is a fuzzy duck" }},
    // [ 1, 2, "${a}" ]    -> [ 1, 2, "fuzzy duck" ]
    { std::vector<Object>{1, 2, "${a}"}, { 1, 2, "fuzzy duck" }},
    // [ "${b}", "${b}" ]  -> [ "a", "b", "a", "b" ]
    { std::vector<Object>{"${b}", "${b}"}, { "a", "b", "a", "b" }},
    // { name: "${a}" }   -> { name: "fuzzy duck" }
    { std::make_shared<map_so>(il_so{{"name", "${a}"}}), { std::make_shared<map_so>(il_so{{"name", "fuzzy duck"}})}},
    // [1, [2, "${b}"]]   -> [ 1, [2, "a", "b"] ]
    { std::vector<Object>{1, std::vector<Object>{2, "${b}"}}, { 1, std::vector<Object>{2, "a", "b"} }},
    // [1, [2, ["${b}"]]]   -> [ 1, [2, ["a", "b"]] ]
    { std::vector<Object>{1, std::vector<Object>{2, std::vector<Object>{"${b}"}}}, { 1, std::vector<Object>{2, std::vector<Object>{"a", "b"} }}},
};

TEST_F(Arrayify, DeepArrayify)
{
    // auto c = Context::create(Metrics());

    for (const auto& m : BINDINGS)
        context->putConstant(m.first, m.second);

    for (auto m : DEEP_TEST_CASES)
        ASSERT_TRUE(IsEqual(std::move(m.second), asDeepArray(*context, m.first))) << m.first;
}
