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

class PropDefTest : public DocumentWrapper {};

enum TestProperty {
    kPropType,
    kPropOne,
    kPropTwo,
};

enum TestType {
    kTestTypeOne,
    kTestTypeTwo,
    kTestTypeThree
};

Bimap<int, std::string> sTestPropertyBimap = {
    {kPropType, "Type"},
    {kPropOne, "One"},
    {kPropOne, "Uno"},
    {kPropOne, "Один"},  // Раз
    {kPropOne, "Ein"},
    {kPropTwo, "Two"},
    {kPropTwo, "Dos"},
    {kPropTwo, "Два"},
    {kPropTwo, "Zwei"},
};

Bimap<int, std::string> sTestTypeBimap = {
    {kTestTypeOne, "TypeOne"},
    {kTestTypeTwo, "TypeTwo"},
    {kTestTypeThree, "TypeThree"}
};

using TestPropDef = PropDef<TestProperty, sTestPropertyBimap>;

static const std::vector<TestPropDef> sTestPropDefs = {
    {kPropType, kTestTypeThree, sTestTypeBimap},
    {kPropOne, 100, asNonNegativeInteger},
    {kPropTwo, Object::FALSE_OBJECT(), asBoolean},
};

/**
 * Verify the structure of the Bimap.
 */
TEST_F(PropDefTest, Bimap)
{
    ASSERT_EQ("Type", sTestPropertyBimap.at(kPropType));
    ASSERT_EQ(kPropType, sTestPropertyBimap.at("Type"));

    ASSERT_EQ("One", sTestPropertyBimap.at(kPropOne));
    ASSERT_EQ(kPropOne, sTestPropertyBimap.at("One"));
    ASSERT_EQ(kPropOne, sTestPropertyBimap.at("Uno"));
    ASSERT_EQ(kPropOne, sTestPropertyBimap.at("Один"));
    ASSERT_EQ(kPropOne, sTestPropertyBimap.at("Ein"));

    ASSERT_EQ("Two", sTestPropertyBimap.at(kPropTwo));
    ASSERT_EQ(kPropTwo, sTestPropertyBimap.at("Two"));
    ASSERT_EQ(kPropTwo, sTestPropertyBimap.at("Dos"));
    ASSERT_EQ(kPropTwo, sTestPropertyBimap.at("Два"));
    ASSERT_EQ(kPropTwo, sTestPropertyBimap.at("Zwei"));

    ASSERT_EQ(4, sTestPropertyBimap.all(kPropOne).size()); // Four STRINGS map to the number 1
    ASSERT_EQ(std::vector<std::string>({"One", "Uno", "Один", "Ein"}), sTestPropertyBimap.all(kPropOne));

    ASSERT_EQ(1, sTestPropertyBimap.all("One").size());    // Exactly one NUMBER maps to the string "One"
    ASSERT_EQ(1, sTestPropertyBimap.all("Ein").size());    // Exactly one NUMBER maps to the string "Ein"
}

static const char *BASIC_SOURCE = R"(
    {
      "Type": "TypeTwo",
      "Uno": 200,
      "Два": true
    }
)";

static const std::map<TestProperty, Object> BASIC_RESULT = {
    {kPropType, Object(kTestTypeTwo)},
    {kPropOne,  200},
    {kPropTwo,  true}
};

/**
 * Take a JSON object that uses alternate names for properties and verify
 * that we (a) find those names appropriately in the BiMap and (b) calculate
 * the correct values using the defined conversion functions and maps.
 */
TEST_F(PropDefTest, Basic)
{
    ASSERT_EQ("One", sTestPropertyBimap.at(kPropOne));

    auto data = JsonData(BASIC_SOURCE);
    ASSERT_TRUE(data);

    auto properties = Properties(Object(data.get()));
    for (const auto& def : sTestPropDefs) {
        auto canonicalName = sTestPropertyBimap.at(def.key);

        auto p = properties.find(def.names);
        ASSERT_TRUE(p != properties.end()) << canonicalName;

        auto value = def.map ? def.map->get(p->second.asString(), -1) : def.func(*context, p->second);
        ASSERT_TRUE(IsEqual(BASIC_RESULT.at(def.key), value)) << def.names[0];
    }
}
