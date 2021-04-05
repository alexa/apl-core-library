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

#include "../tools/src/enumparser.h"

#include "gtest/gtest.h"

#include <sstream>

class EnumgenTest : public ::testing::Test {
public:
    void SetUp() override {
        map.clear();
    }

    void loadDocument(const char *data) {
        enums::EnumParser parser;
        parser.add(std::istringstream(data), "");
        map = parser.enumerations();
    }

public:
    enums::EnumMap map;
};

// Define some convenience operators for checking and printing test results
namespace enums {

bool operator==(const EnumItem& lhs, const EnumItem& rhs) {
    return lhs.name == rhs.name && lhs.value == rhs.value && lhs.comment == rhs.comment;
}

std::ostream& operator<<(std::ostream& out, const EnumItem& item) {
    out << item.name << "=" << item.value << " {" << item.comment << "}";
    return out;
}

}

// *********** TESTS START HERE ***************

static const char *BASIC = R"(
    enum MyTest {
      ZERO,
      ONE,
      TWO
    };
)";

static const enums::EnumMap BASIC_EXPECTED = {
    {
        "MyTest",
        {
            { "ZERO", 0, "" },
            { "ONE", 1, "" },
            { "TWO", 2, "" },
        }
    }
};

TEST_F(EnumgenTest, Basic)
{
    loadDocument(BASIC);
    ASSERT_EQ(BASIC_EXPECTED, map);
}

/***************************************
 * Assign numeric values
 ***************************************/
static const char *ASSIGNED_VALUES = R"(
    enum MyTest {
      ZERO,
      ONE = 2,
      TWO,
      THREE = ONE,
      FOUR
    };
)";

static const enums::EnumMap ASSIGNED_VALUES_EXPECTED = {
    {
        "MyTest",
        {
            { "ZERO", 0, "" },
            { "ONE", 2, "" },
            { "TWO", 3, "" },
            { "THREE", 2, "" },
            { "FOUR", 3, "" },
        }
    }
};

TEST_F(EnumgenTest, AssignedValues)
{
    loadDocument(ASSIGNED_VALUES);
    ASSERT_EQ(ASSIGNED_VALUES_EXPECTED, map);
}

/***************************************
 * Include a trailing comma at the end of an enumeration
 ***************************************/
static const char *BASIC_TRAILING = R"(
    enum MyTest {
      ZERO,
      ONE,
      TWO,
    };
)";

static const enums::EnumMap BASIC_TRAILING_EXPECTED = {
    {
        "MyTest",
        {
            { "ZERO", 0, "" },
            { "ONE", 1, "" },
            { "TWO", 2, "" },
        }
    }
};

TEST_F(EnumgenTest, BasicTrailing)
{
    loadDocument(BASIC_TRAILING);
    ASSERT_EQ(BASIC_TRAILING_EXPECTED, map);
}


/***************************************
 * Inline comments should be transferred
 ***************************************/

static const char *INLINE_COMMENTS = R"(
    enum MyTest {
      ZERO,  // Trailing comment
      /* Leading comment */
      ONE,
      // Leading comment
      TWO,   // with Trailing comment
      THREE,
      /// This comment goes with item FOUR
      FOUR
    };
)";

static const enums::EnumMap INLINE_COMMENTS_EXPECTED = {
    {
        "MyTest",
        {
            { "ZERO", 0, "// Trailing comment" },
            { "ONE", 1, "/* Leading comment */" },
            { "TWO", 2, "// with Trailing comment" },
            { "THREE", 3, ""},
            { "FOUR", 4, "/// This comment goes with item FOUR"}
        }
    }
};

TEST_F(EnumgenTest, InlineComments)
{
    loadDocument(INLINE_COMMENTS);
    ASSERT_EQ(INLINE_COMMENTS_EXPECTED, map);
}



/***************************************
 * Ignore common #ifdef statements
 ***************************************/

static const char *DEFINITIONS = R"(
    enum MyTest {
      ZERO,
#ifdef __FOOBAR__
      ONE,
#elif __OTHERBUZZ__
      TWO,
#else
      THREE,
#endif
      FOUR
    };
)";

static const enums::EnumMap DEFINITIONS_EXPECTED = {
    {
        "MyTest",
        {
            { "ZERO", 0, "" },
            { "ONE", 1, "" },
            { "TWO", 2, "" },
            { "THREE", 3, "" },
            { "FOUR", 4, "" },
        }
    }
};

TEST_F(EnumgenTest, Definitions)
{
    loadDocument(DEFINITIONS);
    ASSERT_EQ(DEFINITIONS_EXPECTED, map);
}



/***************************************
 * Support multiple enumerations
 ***************************************/

static const char *MULTIPLE_ENUMS = R"(
    enum class TestA {
      ALPHA = TestB::ONE,
      BETA,
      GAMMA = TestB::ZERO
    };

    enum TestB {
      ZERO = 100,
      ONE,
      TWO
    };
)";

static const enums::EnumMap MULTIPLE_ENUMS_EXPECTED = {
    {
        "TestA",
        {
            {"ALPHA", 101, ""},
            {"BETA", 102, ""},
            {"GAMMA", 100, ""}
        }
    },
    {
        "TestB",
        {
            {"ZERO", 100, ""},
            {"ONE", 101, ""},
            {"TWO", 102, ""}
        }
    }
};

TEST_F(EnumgenTest, MultipleEnums)
{
    loadDocument(MULTIPLE_ENUMS);
    ASSERT_EQ(MULTIPLE_ENUMS_EXPECTED, map);
}



/***************************************
 * Even more complicated dependencies
 ***************************************/

static const char *MULTIPLE_ENUMS_STAR = R"(
    enum class TestA {
      ZERO = TestB::ONE,   // 97
      ONE = ZERO,          // 97
      TWO = TestC::ZERO,   // 0
      THREE = TestC::TWO,  // 6
      FOUR = TestB::ZERO,  // 5
      FIVE = TestB::TWO,   // 6
      SIX,                 // 7
      SEVEN = ZERO         // 97
    };

    enum TestB {
      ZERO = TestD::ZERO,  // 5
      ONE = 97,            // 97
      TWO = TestC::TWO,    // 6
    };

    enum TestC {
      ZERO = 0,    // 0
      ONE = TestD::TWO,  // 7
      TWO = TestD::ONE,  // 6
    };

    enum TestD {
      ZERO = 5,  // 5
      ONE,       // 6
      TWO,       // 7
    };
)";

static const enums::EnumMap MULTIPLE_ENUMS_STAR_EXPECTED = {
    {
        "TestA",
        {
            {"ZERO", 97, "// 97"},
            {"ONE", 97, "// 97"},
            {"TWO", 0, "// 0"},
            {"THREE", 6, "// 6"},
            {"FOUR", 5, "// 5"},
            {"FIVE", 6, "// 6"},
            {"SIX", 7, "// 7"},
            {"SEVEN", 97, "// 97"}
        }
    },
    {
        "TestB",
        {
            {"ZERO", 5, "// 5"},
            {"ONE", 97, "// 97"},
            {"TWO", 6, "// 6"}
        }
    },
    {
        "TestC",
        {
            {"ZERO", 0, "// 0"},
            {"ONE", 7, "// 7"},
            {"TWO", 6, "// 6"}
        }
    },
    {
        "TestD",
        {
            {"ZERO", 5, "// 5"},
            {"ONE", 6, "// 6"},
            {"TWO", 7, "// 7"}
        }
    }
};

TEST_F(EnumgenTest, MultipleEnumsStar)
{
    loadDocument(MULTIPLE_ENUMS_STAR);
    ASSERT_EQ(MULTIPLE_ENUMS_STAR_EXPECTED, map);
}

/***************************************
 * Dump a Java file
 ***************************************/

static const char *JAVA = R"(
    enum MyTest {
      ZERO,  // Zero comment
      // One comment
      ONE,
      TWO    /* Two comment */
    };
)";

static const char *JAVA_EXPECTED = R"(/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 */

/*
 * AUTOGENERATED FILE. DO NOT MODIFY!
 * This file is autogenerated by enumgen.
 */

package MyPackage;

import android.util.SparseArray;

public enum MyTest implements APLEnum {

    // Zero comment
    ZERO(0),
    // One comment
    ONE(1),
    /* Two comment */
    TWO(2);

    private static SparseArray<MyTest> values = null;

    public static MyTest valueOf(int idx) {
        if(MyTest.values == null) {
            MyTest.values = new SparseArray<>();
            MyTest[] values = MyTest.values();
            for(MyTest value : values) {
                MyTest.values.put(value.getIndex(), value);
            }
        }
        return MyTest.values.get(idx);
    }

    private final int index;

    MyTest (int index) {
        this.index = index;
    }

    @Override
    public int getIndex() { return this.index; }
}
)";

TEST_F(EnumgenTest, JavaTest)
{
    loadDocument(JAVA);
    std::stringstream ss;
    enums::writeJava(ss, "MyPackage", "MyTest", map.at("MyTest"));

    ASSERT_STREQ(ss.str().c_str(), JAVA_EXPECTED);
    // ASSERT_TRUE(CompareStrings(JAVA_EXPECTED, ss.str()));
}


/***************************************
 * Dump a TypeScript file
 ***************************************/

static const char *TYPESCRIPT = R"(
    enum MyTest {
      ZERO,  // Zero comment
      // One comment
      ONE,
      TWO    /* Two comment */
    };
)";

static const char *TYPESCRIPT_EXPECTED = R"(/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 */

/*
 * AUTOGENERATED FILE. DO NOT MODIFY!
 * This file is autogenerated by enumgen.
 */

export enum MyTest {
    // Zero comment
    ZERO = 0,
    // One comment
    ONE = 1,
    /* Two comment */
    TWO = 2,
}
)";

TEST_F(EnumgenTest, TypescriptTest)
{
    loadDocument(TYPESCRIPT);
    std::stringstream ss;
    enums::writeTypeScript(ss, "MyTest", map.at("MyTest"));

    ASSERT_STREQ(ss.str().c_str(), TYPESCRIPT_EXPECTED);
    // ASSERT_TRUE(CompareStrings(TYPESCRIPT_EXPECTED, ss.str()));
}