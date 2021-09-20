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
 */

#include "gtest/gtest.h"
#include "alexaext/alexaext.h"

using namespace alexaext;
using namespace rapidjson;

class ExtensionMessageTest : public ::testing::Test {
public:

    inline
    ::testing::AssertionResult IsValid(const Document& d) {
        if (d.HasParseError()) {
            return ::testing::AssertionFailure() << "Document has parse error at offset "
                                                 << d.GetErrorOffset();
        }

        if (d.IsObject() && d.ObjectEmpty()) {
            return ::testing::AssertionFailure() << "Document is empty object";
        } else if (d.IsArray() && d.Empty()) {
            return ::testing::AssertionFailure() << "Document is empty array";
        } else if (d.IsNull()) {
            return ::testing::AssertionFailure() << "Document is null";
        }

        return ::testing::AssertionSuccess();
    }

    inline
    ::testing::AssertionResult IsEqual(const Value& lhs, const Value& rhs) {
        if (lhs != rhs) {
            return ::testing::AssertionFailure() << "Documents not equal\n"
                                                 << "lhs:\n" << AsPrettyString(lhs)
                                                 << "\nrhs:\n" << AsPrettyString(rhs) << "\n";
        }
        return ::testing::AssertionSuccess();
    }

    Document testDocument;
};

static const char* URI = "alexaext:test:10";

static const char* TEST_MESSAGE = R"(
{
    "version": "1.2.3",
    "method": "TestMethod",
    "uri": "alexaext:test:10",
    "target": "alexaext:test:10"
}
)";

// Test message class
class TestMessage : public BaseMessage<TestMessage> {
public:
    explicit TestMessage(const std::string& version)
            : BaseMessage("TestMethod", version) {}
};

TEST_F(ExtensionMessageTest, TestBaseMessage) {

    Document rhsDoc = TestMessage("1.2.3").uri(URI);

    ASSERT_TRUE(IsValid(rhsDoc));

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(TEST_MESSAGE);
    ASSERT_TRUE(IsValid(lhsDoc));

    ASSERT_TRUE(IsEqual(lhsDoc, rhsDoc));
}

static const char* REGISTER_MESSAGE = R"(
{
    "version": "1.2.3",
    "method": "Register",
    "uri": "alexaext:test:10",
    "target": "alexaext:test:10"
}
)";

TEST_F(ExtensionMessageTest, RegistrationRequest) {

    Document rhsDoc = RegistrationRequest("1.2.3").uri(URI);

    ASSERT_TRUE(IsValid(rhsDoc));

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(REGISTER_MESSAGE);
    ASSERT_TRUE(IsValid(lhsDoc));

    ASSERT_TRUE(IsEqual(lhsDoc, rhsDoc));
}

// sample values
static const char* TEST_MAP_VALUES = R"(
{
    "key1": 1,
    "key2": true,
    "key3": "three"
}
)";

// sample values
static const char* TEST_ARRAY_VALUES = R"(
[
    1,
    true,
    "three"
]
)";

static const char* REGISTER_SETTINGS_MESSAGE = R"(
{
    "version": "1.2.3",
    "method": "Register",
    "uri": "alexaext:test:10",
    "target": "alexaext:test:10",
    "settings": {
        "key1": 1,
        "key2": true,
        "key3": "three"
    }
}
)";

TEST_F(ExtensionMessageTest, RegistrationRequestWithSettings) {

    // Settings are typically extracted from a different document
    Document settings;
    settings.Parse(TEST_MAP_VALUES);

    Document rhsDoc = RegistrationRequest("1.2.3").uri(URI)
            .settings(settings);

    ASSERT_TRUE(IsValid(rhsDoc));

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(REGISTER_SETTINGS_MESSAGE);
    ASSERT_TRUE(IsValid(lhsDoc));

    ASSERT_TRUE(IsEqual(lhsDoc, rhsDoc));
}

static const char* REGISTER_FLAGS_MESSAGE = R"(
{
    "version": "1.2.3",
    "method": "Register",
    "uri": "alexaext:test:10",
    "target": "alexaext:test:10",
    "flags": {
        "key1": 1,
        "key2": true,
        "key3": "three"
    }
}
)";

TEST_F(ExtensionMessageTest, RegistrationRequestWithFlags) {

    // Flags are opaque data passed from runtime
    Document flags;
    flags.Parse(TEST_MAP_VALUES);

    Document rhsDoc = RegistrationRequest("1.2.3").uri(URI)
        .flags(flags);

    ASSERT_TRUE(IsValid(rhsDoc));

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(REGISTER_FLAGS_MESSAGE);
    ASSERT_TRUE(IsValid(lhsDoc));

    ASSERT_TRUE(IsEqual(lhsDoc, rhsDoc));
}

// minimal schema for inclusion in message
static const char* SCHEMA = R"(
{
  "type": "Schema",
  "uri": "alexaext:test:10"
}
)";

// "Golden" example for comparison with builder results
static const char* REGISTER_SUCCESS_MESSAGE = R"(
{
    "version": "1.2.3",
    "method": "RegisterSuccess",
    "uri": "alexaext:test:10",
    "target": "alexaext:test:10",
    "token": "token4",
    "environment": {
        "key1": 1,
        "key2": true,
        "key3": "three"
    },
    "schema": {
        "type": "Schema",
        "uri": "alexaext:test:10"
    }
}
)";

TEST_F(ExtensionMessageTest, RegistrationSuccess) {

    // static environment provided by alternate doc
    Document environment;
    environment.Parse(TEST_MAP_VALUES);

    // static schema provided by alternate doc
    Document schema;
    schema.Parse(SCHEMA);

    Document rhsDoc = RegistrationSuccess("1.2.3").uri(URI)
            .token("token4")
            .environment(environment)
            .schema(schema);

    ASSERT_TRUE(IsValid(rhsDoc));

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(REGISTER_SUCCESS_MESSAGE);
    ASSERT_TRUE(IsValid(lhsDoc));

    ASSERT_TRUE(IsEqual(lhsDoc, rhsDoc));
}

// "Golden" example for comparison with builder results
static const char* REGISTER_FAILURE_MESSAGE = R"(
{
    "version": "1.2.3",
    "method": "RegisterFailure",
    "uri": "alexaext:test:10",
    "target": "alexaext:test:10",
    "code": 400,
    "message": "Bad Request"
}
)";

TEST_F(ExtensionMessageTest, RegistrationFailure) {

    Document rhsDoc = RegistrationFailure("1.2.3").uri(URI)
            .errorCode(400).errorMessage("Bad Request");

    ASSERT_TRUE(IsValid(rhsDoc));

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(REGISTER_FAILURE_MESSAGE);
    ASSERT_TRUE(IsValid(rhsDoc));

    ASSERT_TRUE(IsEqual(lhsDoc, rhsDoc));
}

TEST_F(ExtensionMessageTest, RegistrationFailureForUnknownURI) {
    rapidjson::Document expected;
    expected.Parse(R"(
        {
            "version": "1.0",
            "method": "RegisterFailure",
            "uri": "alexaext:test:10",
            "target": "alexaext:test:10",
            "code": 100,
            "message": "Unknown extension - uri: alexaext:test:10"
        }
    )");
    ASSERT_TRUE(IsValid(expected));

    rapidjson::Document actual = RegistrationFailure::forUnknownURI(URI);
    ASSERT_TRUE(IsValid(actual));

    ASSERT_TRUE(IsEqual(expected, actual));
}

TEST_F(ExtensionMessageTest, RegistrationFailureForInvalidMessage) {
    rapidjson::Document expected;
    expected.Parse(R"(
        {
            "version": "1.0",
            "method": "RegisterFailure",
            "uri": "alexaext:test:10",
            "target": "alexaext:test:10",
            "code": 200,
            "message": "Invalid or malformed message."
        }
    )");
    ASSERT_TRUE(IsValid(expected));

    rapidjson::Document actual = RegistrationFailure::forInvalidMessage(URI);
    ASSERT_TRUE(IsValid(actual));

    ASSERT_TRUE(IsEqual(expected, actual));
}

TEST_F(ExtensionMessageTest, RegistrationFailureForUnknownException) {
    rapidjson::Document expected;
    expected.Parse(R"(
        {
            "version": "1.0",
            "method": "RegisterFailure",
            "uri": "alexaext:test:10",
            "target": "alexaext:test:10",
            "code": 300,
            "message": "Unknown Exception."
        }
    )");
    ASSERT_TRUE(IsValid(expected));

    rapidjson::Document actual = RegistrationFailure::forUnknownException(URI);
    ASSERT_TRUE(IsValid(actual));

    ASSERT_TRUE(IsEqual(expected, actual));
}

TEST_F(ExtensionMessageTest, RegistrationFailureForException) {
    rapidjson::Document expected;
    expected.Parse(R"(
        {
            "version": "1.0",
            "method": "RegisterFailure",
            "uri": "alexaext:test:10",
            "target": "alexaext:test:10",
            "code": 400,
            "message": "Extension Exception - uri:alexaext:test:10 msg:Something failed"
        }
    )");
    ASSERT_TRUE(IsValid(expected));

    rapidjson::Document actual = RegistrationFailure::forException(URI, "Something failed");
    ASSERT_TRUE(IsValid(actual));

    ASSERT_TRUE(IsEqual(expected, actual));
}

TEST_F(ExtensionMessageTest, RegistrationFailureForFailedCommand) {
    rapidjson::Document expected;
    expected.Parse(R"(
        {
            "version": "1.0",
            "method": "RegisterFailure",
            "uri": "alexaext:test:10",
            "target": "alexaext:test:10",
            "code": 500,
            "message": "Failed Command - id: MyCommand"
        }
    )");
    ASSERT_TRUE(IsValid(expected));

    rapidjson::Document actual = RegistrationFailure::forFailedCommand(URI, "MyCommand");
    ASSERT_TRUE(IsValid(actual));

    ASSERT_TRUE(IsEqual(expected, actual));
}

TEST_F(ExtensionMessageTest, RegistrationFailureForInvalidExtensionSchema) {
    rapidjson::Document expected;
    expected.Parse(R"(
        {
            "version": "1.0",
            "method": "RegisterFailure",
            "uri": "alexaext:test:10",
            "target": "alexaext:test:10",
            "code": 600,
            "message": "Invalid or malformed extension schema. uri: alexaext:test:10"
        }
    )");
    ASSERT_TRUE(IsValid(expected));

    rapidjson::Document actual = RegistrationFailure::forInvalidExtensionSchema(URI);
    ASSERT_TRUE(IsValid(actual));

    ASSERT_TRUE(IsEqual(expected, actual));
}

// "Golden" example for comparison with builder results
static const char* COMMAND_MESSAGE = R"(
{
    "version": "1.2.3",
    "method": "Command",
    "payload": {
        "key1": 1,
        "key2": true,
        "key3": "three",
        "key4": {
            "key1": 1,
            "key2": true,
            "key3": "three"
        }
    },
    "uri": "alexaext:test:10",
    "target": "alexaext:test:10",
    "id": 13,
    "name": "myCommand"
}
)";

TEST_F(ExtensionMessageTest, Command) {

    // payload using move
    Document complexProperty;
    complexProperty.Parse(TEST_MAP_VALUES);

    Document rhsDoc = Command("1.2.3").uri(URI)
           .id(13).name("myCommand")
           .property("key1", 1)
           .property("key2", true)
           .property("key3", "three")
           .property("key4", complexProperty.Move());

    ASSERT_TRUE(IsValid(rhsDoc));

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(COMMAND_MESSAGE);
    ASSERT_TRUE(IsValid(lhsDoc));

    ASSERT_TRUE(IsEqual(lhsDoc, rhsDoc));
}

// "Golden" example for comparison with builder results
static const char* COMMAND_SUCCESS_MESSAGE = R"(
{
    "version": "1.2.3",
    "method": "CommandSuccess",
    "uri": "alexaext:test:10",
    "target": "alexaext:test:10",
    "id": 13
}
)";

TEST_F(ExtensionMessageTest, CommandSuccess) {

    Document rhsDoc = CommandSuccess("1.2.3").uri(URI)
            .id(13);

    //TODO Result

    ASSERT_TRUE(IsValid(rhsDoc));

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(COMMAND_SUCCESS_MESSAGE);
    ASSERT_TRUE(IsValid(lhsDoc));

    ASSERT_TRUE(IsEqual(lhsDoc, rhsDoc));
}

// "Golden" example for comparison with builder results
static const char* COMMAND_FAILURE_MESSAGE = R"(
{
    "version": "1.2.3",
    "method": "CommandFailure",
    "uri": "alexaext:test:10",
    "target": "alexaext:test:10",
    "code": 400,
    "message": "Bad Request"
}
)";

TEST_F(ExtensionMessageTest, CommandFailure) {

    Document rhsDoc = CommandFailure("1.2.3").uri(URI)
            .errorCode(400).errorMessage("Bad Request");

    ASSERT_TRUE(IsValid(rhsDoc));

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(COMMAND_FAILURE_MESSAGE);
    ASSERT_TRUE(IsValid(lhsDoc));

    ASSERT_TRUE(IsEqual(lhsDoc, rhsDoc));
}

// "Golden" example for comparison with builder results
static const char* EVENT_MESSAGE = R"(
{
    "version": "1.2.3",
    "method": "Event",
    "uri": "alexaext:test:10",
    "target": "alexaext:test:10",
    "name": "myEvent",
    "payload": {
        "key1": 1,
        "key2": true,
        "key3": "three",
        "key4": {
            "key1": 1,
            "key2": true,
            "key3": "three"
        }
    }
}
)";

TEST_F(ExtensionMessageTest, Event) {

    // payload uses move semantics
    Document doc;
    doc.Parse(TEST_MAP_VALUES);
    Value complexItem (doc, doc.GetAllocator());

    Document rhsDoc = Event("1.2.3").uri(URI)
            .name("myEvent")
            .property("key1", 1)
            .property("key2", true)
            .property("key3", "three")
            .property("key4", complexItem);

    ASSERT_TRUE(IsValid(rhsDoc));

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(EVENT_MESSAGE);
    ASSERT_TRUE(IsValid(lhsDoc));

    ASSERT_TRUE(IsEqual(lhsDoc, rhsDoc));
}

// "Golden" example for comparison with builder results
static const char* LIVE_DATA_MAP_UPDATE = R"(
{
    "version": "1.2.3",
    "method": "LiveDataUpdate",
    "operations": [
        {
            "type": "Set",
            "key": "key1",
            "item": 1
        },
        {
            "type": "Set",
            "key": "key2",
            "item": true
        },
        {
            "type": "Set",
            "key": "key3",
            "item": "three"
        },
        {
            "type": "Set",
            "key": "key4",
            "item": {
                "key1": 1,
                "key2": true,
                "key3": "three"
            }
        }
    ],
    "uri": "alexaext:test:10",
    "target": "alexaext:test:10",
    "name": "MyLiveDataMap"
}
)";

TEST_F(ExtensionMessageTest, LiveDataMapUpdate) {

    // payload uses move item
    Document doc;
    doc.Parse(TEST_MAP_VALUES);
    Value complexItem (doc, doc.GetAllocator());

    Document rhsDoc = LiveDataUpdate("1.2.3").uri(URI)
            .objectName("MyLiveDataMap")
            .liveDataMapUpdate([](LiveDataMapOperation operation) {
                operation.type("Set")
                        .key("key1")
                        .item(1);
            })
             .liveDataMapUpdate([](LiveDataMapOperation operation) {
                 operation.type("Set")
                          .key("key2")
                          .item(true);
             })
             .liveDataMapUpdate([](LiveDataMapOperation operation) {
                 operation.type("Set")
                          .key("key3")
                          .item("three");
             })
             .liveDataMapUpdate([&complexItem](LiveDataMapOperation operation) {
                 operation.type("Set")
                          .key("key4")
                          .item(complexItem);
            });

    ASSERT_TRUE(IsValid(rhsDoc));

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(LIVE_DATA_MAP_UPDATE);
    ASSERT_TRUE(IsValid(lhsDoc));

    ASSERT_TRUE(IsEqual(lhsDoc, rhsDoc));
}

// "Golden" example for comparison with builder results
static const char* LIVE_DATA_ARRAY_UPDATE = R"(
{
    "version": "1.2.3",
    "method": "LiveDataUpdate",
    "operations": [
        {
            "type": "Insert",
            "index": 1,
            "item": 1
        },
        {
            "type": "Insert",
            "index": 2,
            "item": true
        },
        {
            "type": "Insert",
            "index": 3,
            "item": "three"
        },
        {
            "type": "Insert",
            "index": 4,
            "item": [
                1,
                true,
                "three"
            ]
        },
        {
            "type": "Remove",
            "count": 3
        }
    ],
    "uri": "alexaext:test:10",
    "target": "alexaext:test:10",
    "name": "MyLiveDataArray"
}
)";

TEST_F(ExtensionMessageTest, LiveDataArrayUpdate) {

    // payload uses move item
    Document doc;
    doc.Parse(TEST_ARRAY_VALUES);
    Value complexItem (doc, doc.GetAllocator());

    Document rhsDoc = LiveDataUpdate("1.2.3")
            .uri(URI)
            .objectName("MyLiveDataArray")
            .liveDataArrayUpdate([](LiveDataArrayOperation operation) {
                operation.type("Insert")
                         .index(1)
                         .item(1);
            })
            .liveDataArrayUpdate([](LiveDataArrayOperation operation) {
                operation.type("Insert")
                         .index(2)
                         .item(true);
            })
            .liveDataArrayUpdate([](LiveDataArrayOperation operation) {
                operation.type("Insert")
                         .index(3)
                         .item("three");
            })
            .liveDataArrayUpdate([&complexItem](LiveDataArrayOperation operation) {
                operation.type("Insert")
                         .index(4)
                         .item(complexItem);
            })
            .liveDataArrayUpdate([](LiveDataArrayOperation operation) {
                operation.type("Remove").count(3);
            });

    ASSERT_TRUE(IsValid(rhsDoc));

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(LIVE_DATA_ARRAY_UPDATE);
    ASSERT_TRUE(IsValid(lhsDoc));

    ASSERT_TRUE(IsEqual(lhsDoc, rhsDoc));
}

static const char* SAMPLE_ENVIRONMENT = R"(
{
    "integral": 42,
    "float": 42.0,
    "fractional": 42.5,
    "bool": true,
    "string": "Hello, my name is Inigo Montoya",
    "null": null
}
)";

TEST_F(ExtensionMessageTest, GetWithDefault) {
    Document env;
    env.Parse(SAMPLE_ENVIRONMENT);
    ASSERT_TRUE(IsValid(env));

    ASSERT_EQ(42, GetWithDefault<int>("integral", env, 0));
    ASSERT_EQ(42, GetWithDefault<unsigned int>("integral", env, 0));
    ASSERT_EQ(42.0, GetWithDefault<double>("integral", env, 0));
    ASSERT_EQ(42.0f, GetWithDefault<float>("integral", env, 0));

    ASSERT_EQ(42, GetWithDefault<int>("float", env, 0));
    ASSERT_EQ(42, GetWithDefault<unsigned int>("float", env, 0));
    ASSERT_EQ(42.0, GetWithDefault<double>("float", env, 0));
    ASSERT_EQ(42.0f, GetWithDefault<float>("float", env, 0));

    ASSERT_EQ(42, GetWithDefault<int>("fractional", env, 0));
    ASSERT_EQ(42, GetWithDefault<unsigned int>("fractional", env, 0));
    ASSERT_EQ(42.5, GetWithDefault<double>("fractional", env, 0));
    ASSERT_EQ(42.5f, GetWithDefault<float>("fractional", env, 0));

    ASSERT_TRUE(GetWithDefault<bool>("bool", env, false));
    ASSERT_TRUE(GetWithDefault<bool>("integral", env, false));
    ASSERT_TRUE(GetWithDefault<bool>("float", env, false));
    ASSERT_TRUE(GetWithDefault<bool>("fractional", env, false));

    ASSERT_STREQ("Hello, my name is Inigo Montoya", GetWithDefault("string", env, ""));
    ASSERT_EQ(std::string("Hello, my name is Inigo Montoya"), GetWithDefault<std::string>("string", env, std::string()));

    ASSERT_EQ(1, GetWithDefault<int>("null", env, 1));
    ASSERT_EQ(1, GetWithDefault<unsigned int>("null", env, 1));
    ASSERT_EQ(1.0, GetWithDefault<double>("null", env, 1));
    ASSERT_EQ(1.0f, GetWithDefault<float>("null", env, 1));
    ASSERT_STREQ("default", GetWithDefault("null", env, "default"));
    ASSERT_EQ("default", GetWithDefault<std::string>("null", env, std::string("default")));

    ASSERT_EQ(1, GetWithDefault<int>("missing", env, 1));
    ASSERT_EQ(1, GetWithDefault<unsigned int>("missing", env, 1));
    ASSERT_EQ(1.0, GetWithDefault<double>("missing", env, 1));
    ASSERT_EQ(1.0f, GetWithDefault<float>("missing", env, 1));
    ASSERT_STREQ("default", GetWithDefault("missing", env, "default"));
    ASSERT_EQ("default", GetWithDefault<std::string>("missing", env, std::string("default")));

    rapidjson::Value null;
    null.SetNull();
    ASSERT_EQ(1, GetWithDefault<int>("missing", null, 1));
    ASSERT_EQ(1, GetWithDefault<unsigned int>("missing", null, 1));
    ASSERT_EQ(1.0, GetWithDefault<double>("missing", null, 1));
    ASSERT_EQ(1.0f, GetWithDefault<float>("missing", null, 1));
    ASSERT_STREQ("default", GetWithDefault("missing", null, "default"));
    ASSERT_EQ("default", GetWithDefault<std::string>("missing", null, std::string("default")));

    rapidjson::Value array;
    array.SetArray();
    ASSERT_EQ(1, GetWithDefault<int>("missing", array, 1));
    ASSERT_EQ(1, GetWithDefault<unsigned int>("missing", array, 1));
    ASSERT_EQ(1.0, GetWithDefault<double>("missing", array, 1));
    ASSERT_EQ(1.0f, GetWithDefault<float>("missing", array, 1));
    ASSERT_STREQ("default", GetWithDefault("missing", array, "default"));
    ASSERT_EQ("default", GetWithDefault<std::string>("missing", array, std::string("default")));

    rapidjson::Value number;
    number.SetDouble(1.0);
    ASSERT_EQ(1, GetWithDefault<int>("missing", number, 1));
    ASSERT_EQ(1, GetWithDefault<unsigned int>("missing", number, 1));
    ASSERT_EQ(1.0, GetWithDefault<double>("missing", number, 1));
    ASSERT_EQ(1.0f, GetWithDefault<float>("missing", number, 1));
    ASSERT_STREQ("default", GetWithDefault("missing", number, "default"));
    ASSERT_EQ("default", GetWithDefault<std::string>("missing", number, std::string("default")));

    // Null root variant
    ASSERT_EQ(1, GetWithDefault<int>("missing", nullptr, 1));
    ASSERT_EQ(1, GetWithDefault<unsigned int>("missing", nullptr, 1));
    ASSERT_EQ(1.0, GetWithDefault<double>("missing", nullptr, 1));
    ASSERT_EQ(1.0f, GetWithDefault<float>("missing", nullptr, 1));
    ASSERT_STREQ("default", GetWithDefault("missing", nullptr, "default"));
    ASSERT_EQ("default", GetWithDefault<std::string>("missing", nullptr, std::string("default")));
}