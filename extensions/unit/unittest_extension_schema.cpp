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
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>

using namespace alexaext;
using namespace rapidjson;

class ExtensionSchemaTest : public ::testing::Test {
public:

    static const std::string asString(const Value& value) {
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        value.Accept(writer);
        return std::string(buffer.GetString());
    }


    inline
    ::testing::AssertionResult IsEqual(const Value& lhs, const Value& rhs) {
        asString(lhs);
        asString(rhs);

        if (lhs != rhs) {
            return ::testing::AssertionFailure() << "Documents not equal\n"
                                                 << "lhs:\n" << asString(lhs)
                                                 << "\nrhs:\n" << asString(rhs) << "\n";
        }
        return ::testing::AssertionSuccess();
    }

    Document testDocument;
};

static const char* URI = "alexaext:test:10";

// Extension Schema top level structure.
// "Golden" example for comparison with builder results
static const char* SCHEMA = R"(
{
  "type": "Schema",
  "version": "1.0",
  "events": [],
  "types": [],
  "commands": [],
  "liveData": [],
  "uri": "alexaext:test:10"
}
)";

TEST_F(ExtensionSchemaTest, Schema) {

    Value schemaMsg = ExtensionSchema(&testDocument.GetAllocator(), "1.0").uri(URI);
    ASSERT_FALSE(schemaMsg.IsNull());

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(SCHEMA);
    ASSERT_FALSE(lhsDoc.HasParseError() && lhsDoc.Empty());

    ASSERT_TRUE(IsEqual(lhsDoc, schemaMsg));
}

// sample values
static const char* TEST_VALUES = R"(
{
    "key1": 1,
    "key2": true,
    "key3": "three"
}
)";

// Extension Schema type structure.
// "Golden" example for comparison with builder results
static const char* TYPES = R"(
{
    "types": [
        {
            "name": "MyType",
            "properties": {
                "prop1": "number",
                "prop2": "boolean",
                "prop3": {
                    "type": "string",
                    "required": false,
                    "default": "three"
                }
            }
        },
        {
            "name": "MyType2",
            "properties": {
                "prop4": {
                    "description": "prop4 description",
                    "required": true,
                    "default": {
                        "key1": 1,
                        "key2": true,
                        "key3": "three"
                    }
                }
            },
            "extends": "MyType"
        }
    ]
}
)";

TEST_F(ExtensionSchemaTest, SchemaTypes) {

    Document complexProperty;
    complexProperty.Parse(TEST_VALUES);

    ExtensionSchema schemaMsg(&testDocument.GetAllocator(), "1.0");
    // add events
    schemaMsg.uri(URI)
            .dataType("MyType", [](TypeSchema& typeSchema) {
                typeSchema.property("prop1", "number")
                        .property("prop2", "boolean")
                        .property("prop3", [](TypePropertySchema& propertySchema) {
                                    propertySchema.type("string")
                                    .required(false)
                                    .defaultValue("three");
                        });
            })
            .dataType("MyType2", [&complexProperty](TypeSchema& typeSchema) {
                typeSchema.extends("MyType")
                        .property("prop4", [&complexProperty](TypePropertySchema& propertySchema) {
                            propertySchema.description("prop4 description")
                                    .required(true)
                                    .defaultValue(complexProperty.Move());
                        });
            });

    Value rhsValue = schemaMsg;
    ASSERT_FALSE(rhsValue.IsNull());

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(TYPES);
    ASSERT_FALSE(lhsDoc.HasParseError() && lhsDoc.Empty());

    // get the Events and compare to expected
    Value* expected = ExtensionSchema::TYPES().Get(lhsDoc);
    Value* actual = ExtensionSchema::TYPES().Get(rhsValue);
    ASSERT_TRUE(IsEqual(*expected, *actual));
}


// Extension Schema event structure.
// "Golden" example for comparison with builder results
static const char* EVENTS = R"(
{
    "events": [
        {
            "name": "myEventOne"
        },
        {
            "name": "myEventTwo",
            "fastMode": false
        },
        {
            "name": "myEventThree",
            "fastMode": true
        }
    ]
}
)";

TEST_F(ExtensionSchemaTest, SchemaEvents) {

    ExtensionSchema schemaMsg(&testDocument.GetAllocator(), "1.0");
    // add events
    schemaMsg.uri(URI)
            .event("myEventOne")
            .event("myEventTwo", [](EventSchema& eventSchema) {
                eventSchema.fastMode(false);
            })
            .event("myEventThree", [](EventSchema& eventSchema) {
                eventSchema.fastMode(true);
            });

    Value rhsValue = schemaMsg;
    ASSERT_FALSE(rhsValue.IsNull());

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(EVENTS);
    ASSERT_FALSE(lhsDoc.HasParseError() && lhsDoc.Empty());

    // get the Events and compare to expected
    Value* expected = ExtensionSchema::EVENTS().Get(lhsDoc);
    Value* actual = ExtensionSchema::EVENTS().Get(rhsValue);
    ASSERT_TRUE(IsEqual(*expected, *actual));
}


// Extension Schema event structure.
// "Golden" example for comparison with builder results
static const char* COMMANDS = R"(
{
  "commands": [
    {
      "name": "myCommandOne",
      "payload": "MyDataType",
      "allowFastMode": true
    },
    {
      "name": "myCommandTwo",
      "payload": "MyDataType",
      "allowFastMode": false,
      "requireResponse": true,
      "description": "myDescription"
    }
  ]
}
)";

TEST_F(ExtensionSchemaTest, SchemaCommand) {

    ExtensionSchema schemaMsg(&testDocument.GetAllocator(), "1.0");

    // add events
    schemaMsg.uri(URI)
            .command("myCommandOne",
                     [](CommandSchema& commandSchema) {
                         commandSchema.dataType("MyDataType")
                         .allowFastMode(true);
                     })
            .command("myCommandTwo",
                     [](CommandSchema& commandSchema) {
                         commandSchema.dataType("MyDataType")
                                .allowFastMode(false)
                                .requireResponse(true)
                                .description("myDescription");
                     });

    Value rhsValue = schemaMsg;
    ASSERT_FALSE(rhsValue.IsNull());

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(COMMANDS);
    ASSERT_FALSE(lhsDoc.HasParseError() && lhsDoc.Empty());

    // get the Command and compare to expected
    Value* expected = ExtensionSchema::COMMANDS().Get(lhsDoc);
    Value* actual = ExtensionSchema::COMMANDS().Get(rhsValue);
    ASSERT_TRUE(IsEqual(*expected, *actual));
}

// Extension Schema liveData structure.
// "Golden" example for comparison with builder results
static const char* LIVE_DATA = R"(
{
  "liveData": [
    {
      "name": "MyMap",
      "type": "MyDataType",
      "events": {
        "set": {
          "eventHandler": "onSet",
          "properties": []
        },
        "update": {
          "eventHandler": "onUpdate",
          "properties": [
            {
              "name": "one",
              "update": true
            },
            {
              "name": "two",
              "update": false,
              "collapse": false
            }
          ]
        },
        "add": {
          "eventHandler": "onAdd",
          "properties": [
            {
              "name": "three",
              "collapse": false
            }
          ]
        },
        "remove": {
          "eventHandler": "onRemove",
          "properties": [
            {
              "name": "four",
              "update": true,
              "collapse": true
            }
          ]
        }
      }
    },
    {
      "name": "MyArray",
      "type": "MyDataType[]",
      "events": {
        "set": {
          "eventHandler": "onSet",
          "properties": []
        }
      }
    }
  ]
}
)";

TEST_F(ExtensionSchemaTest, SchemaLiveData) {

    ExtensionSchema schemaMsg(&testDocument.GetAllocator(), "1.0");

    // add live data
    schemaMsg.uri(URI)
            .liveDataMap("MyMap",
                         [](LiveDataSchema& dataSchema) {
                             dataSchema.dataType("MyDataType")
                                     .eventHandler(LiveDataSchema::OPERATION_SET(), "onSet")
                                     .eventHandler(LiveDataSchema::OPERATION_UPDATE(), "onUpdate",
                                                   [](EventHandlerSchema& schema) {
                                                       schema.property("one",
                                                                       [](EventHandlerPropertySchema& propertySchema) {
                                                                           propertySchema.updateOnChange(true);
                                                                       })
                                                               .property("two",
                                                                         [](EventHandlerPropertySchema& propertySchema) {
                                                                             propertySchema.collapse(false)
                                                                                     .updateOnChange(false);
                                                                         });
                                                   })
                                     .eventHandler(LiveDataSchema::OPERATION_ADD(), "onAdd",
                                                   [](EventHandlerSchema& schema) {
                                                       schema.property("three",
                                                                       [](EventHandlerPropertySchema& propertySchema) {
                                                                           propertySchema.collapse(false);
                                                                       });
                                                   })
                                     .eventHandler(LiveDataSchema::OPERATION_REMOVE(), "onRemove",
                                                   [](EventHandlerSchema& schema) {
                                                       schema.property("four",
                                                                       [](EventHandlerPropertySchema& propertySchema) {
                                                                           propertySchema.collapse(true)
                                                                                   .updateOnChange(true);
                                                                       });
                                                   });
                         })
            .liveDataArray("MyArray",
                           [](LiveDataSchema& dataSchema) {
                               dataSchema.dataType("MyDataType")
                                       .eventHandler(LiveDataSchema::OPERATION_SET(), "onSet");
                           });


    Value rhsValue = schemaMsg;
    ASSERT_FALSE(rhsValue.IsNull());

    // creat an "expected" document for comparison
    Document lhsDoc;
    lhsDoc.Parse(LIVE_DATA);
    ASSERT_FALSE(lhsDoc.HasParseError() && lhsDoc.Empty());

    // get the LiveData and compare to expected
    Value* expected = ExtensionSchema::LIVE_DATA().Get(lhsDoc);
    Value* actual = ExtensionSchema::LIVE_DATA().Get(rhsValue);
    ASSERT_TRUE(IsEqual(*expected, *actual));
}
