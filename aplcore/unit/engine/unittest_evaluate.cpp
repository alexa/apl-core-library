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

class EvaluateTest : public DocumentWrapper {};

// The boolean test cases are testing for "Truthy" (see section 4.2 of the APL specification)
template<class T>
struct TEST_CASE {
    std::string object;
    T defValue;
    T expected;
};

const static std::vector<TEST_CASE<bool>> BOOLEAN_TESTS = {
    { R"({})", true, true },   // No key -> use the default
    { R"({})", false, false },
    { R"({"key": null})", true, false }, // Null is falsy
    { R"({"key": true})", false, true },
    { R"({"key": false})", true, false },
    { R"({"key": 0})", true, false },  // Zero number is false
    { R"({"key": -21})", false, true },  // Non-zero number is true
    { R"({"key": "abc"})", false, true }, // Non-empty string is true
    { R"({"key": ""})", true, false },  // Empty string is false
    { R"({"key": []})", false, true }, // Any array is truthy
    { R"({"key": {}})", false, true }, // Any object is truthy
};

TEST_F(EvaluateTest, Boolean)
{
    for (const auto &m : BOOLEAN_TESTS) {
        auto data = JsonData(m.object);
        auto obj = Object(data.get());
        ASSERT_EQ(propertyAsBoolean(*context, obj, "key", m.defValue), m.expected)
            << "object: " << m.object
            << " defValue: " << m.defValue
            << " expected: " << m.expected;
    }
}

const static std::vector<TEST_CASE<double>> DOUBLE_TESTS = {
    { R"({})", 23.25, 23.25 },   // No key -> use the default
    { R"({})", 0, 0 },
    { R"({"key": null})", 23.25, 23.25 },
    { R"({"key": true})", 23.25, 1 },  // True and false values are stored internally as numbers
    { R"({"key": false})", 23.25, 0 },
    { R"({"key": 0})", 23.25, 0 },
    { R"({"key": 1})", 23.25, 1 },
    { R"({"key": "abc"})", 23.25, 23.25 },
    { R"({"key": ""})", 23.25, 23.25 },
    { R"({"key": []})", 23.25, 23.25 },
    { R"({"key": {}})", 23.25, 23.25 },
};

TEST_F(EvaluateTest, Double)
{
    for (const auto &m : DOUBLE_TESTS) {
        auto data = JsonData(m.object);
        auto obj = Object(data.get());
        ASSERT_EQ(propertyAsDouble(*context, obj, "key", m.defValue), m.expected)
            << "object: " << m.object
            << " defValue: " << m.defValue
            << " expected: " << m.expected;
    }
}

const static std::vector<TEST_CASE<int>> INT_TESTS = {
    { R"({})", 23, 23 },   // No key -> use the default
    { R"({})", 0, 0 },
    { R"({"key": null})", 23, 23 },
    { R"({"key": true})", 23, 1 },  // True and false values are stored internally as numbers
    { R"({"key": false})", 23, 0 },
    { R"({"key": 0})", 23, 0 },
    { R"({"key": 1})", 23, 1 },
    { R"({"key": "abc"})", 23, 23 },
    { R"({"key": ""})", 23, 23 },
    { R"({"key": []})", 23, 23 },
    { R"({"key": {}})", 23, 23 },
};

TEST_F(EvaluateTest, Int)
{
    for (const auto &m : INT_TESTS) {
        auto data = JsonData(m.object);
        auto obj = Object(data.get());
        ASSERT_EQ(propertyAsInt(*context, obj, "key", m.defValue), m.expected)
            << "object: " << m.object
            << " defValue: " << m.defValue
            << " expected: " << m.expected;
    }
}
