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
#ifndef ALEXAEXT_EXTENSIONSCHEMA_H
#define ALEXAEXT_EXTENSIONSCHEMA_H

#include <functional>
#include <map>
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>

namespace alexaext {

/**
 * The extension schema defines the extension api exposed to the execution environment.  The schema
 * is typically included in the RegistrationSuccess message returned by extensions during initialization.
 * See the "Alexa Extension Developer Guide" for the extension schema specification and discussion of extension
 * message passing.
 *
 * The extension schema are json objects and may be created using rapidjson, or using the schema builders from
 * this class. The allocator used in schema creation, must match the allocator of the enclosing message.
 *
 * An example schema may look as follows:
 *
 *            ExtensionSchema schema(allocator, "1.0");
 *            schema.uri("alexaext:myextension:10")
 *                  .event("myEventOne")
 *                  .event("myEventThree", [](EventSchema& eventSchema) {
 *                          eventSchema.fastMode(true);
 *                  })
 *                  .command("myCommandTwo",
 *                            [](CommandSchema& commandSchema) {
 *                               commandSchema.dataType("MyDataType")
 *                                        .requireResponse(true)
 *                                        .description("myDescription");
 *                            })
 *                  .liveDataArray("MyArray",
 *                                  [](LiveDataSchema& dataSchema) {
 *                                      dataSchema.dataType("MyDataType")
 *                                             .eventHandler(LiveDataSchema::OPERATION_SET(), "onSet");
 *                                  });
 *
 * Schema builders may be assigned to rapidjson::Value objects. Move semantics are used
 * in the assignment, making the memory resources of the builder invalid.
 *
 *              rapidjson::Value schemaVal = schema;
 *
 * Schema values can be extracted using the rapidjson:Pointers defined in the builders.  For example:
 *
 *              auto uri = ExtensionSchema::URI().Get(rawSchema)->GetString();
 *
 *
 *  Rapidjson tutorial: https://rapidjson.org/md_doc_tutorial.html
 *  Rapidjson Pointers: https://rapidjson.org/md_doc_pointer.html
 *
 */


/**
 * Base class for extension schema builders. Provides utility methods
 * for chaining builders together via lambda.
 */
class SchemaBuilder {
public:
    virtual ~SchemaBuilder() = default;

    explicit SchemaBuilder(rapidjson::Document::AllocatorType* allocator)
            : mAllocator(allocator),
              mValue(std::make_shared<rapidjson::Value>()) {}

    explicit SchemaBuilder(std::shared_ptr<rapidjson::Document> document)
            : mAllocator(&document->GetAllocator()), mValue(document) {}

    operator rapidjson::Value&&() {
        return std::move(*mValue);
    }

    static const rapidjson::Pointer& NAME() {
        static const rapidjson::Pointer ptr("/name");
        return ptr;
    }

protected:
    /// Build a schema structure and push it onto an array
    template<class S, class... Args>
    void factoryPush(const rapidjson::Pointer& path,
                     const std::function<void(S&)>& builder,
                     Args... args) {
        // create a typed schema builder object and pass it to a builder function
        S schema(mAllocator, std::forward<Args>(args)...);
        if (builder)
            builder(schema);
        // add definition to schema
        path.Get(*mValue)->PushBack(schema, *mAllocator);
    }

    /// Build a schema structure and add it to an object
    template<class S, class... Args>
    void factoryAdd(const rapidjson::Pointer& path,
                    const std::function<void(S&)>& builder,
                    Args... args) {
        // create a typed schema builder object and pass it to a builder function
        S schema(mAllocator, std::forward<Args>(args)...);
        if (builder)
            builder(schema);
        // add definition to schema
        path.Set(*mValue, schema, *mAllocator);
    }

    /// Build a schema structure and add it to an object using a named key
    template<class S, class... Args>
    void factoryAdd(const rapidjson::Pointer& path,
                    const std::string& key,
                    const std::function<void(S&)>& builder,
                    Args... args) {
        // create a typed schema builder object and pass it to a builder function
        S schema(mAllocator, std::forward<Args>(args)...);
        if (builder)
            builder(schema);
        // add definition to schema
        path.Get(*mValue)->AddMember(rapidjson::Value(key.c_str(), *mAllocator), schema, *mAllocator);
    }

protected:
    rapidjson::Document::AllocatorType* mAllocator;
    std::shared_ptr<rapidjson::Value> mValue;
};

// forward declare ExtensionSchema builder dependencies for file readability
class TypeSchema;

class EventSchema;

class CommandSchema;

class LiveDataSchema;

/**
 * Extension Schema builder.
 * The extension schema defines the extension api that is exposed to the execution environment.
 * The schemas is specific to the extension version and uniquely identified by the extension URI.
 */
class ExtensionSchema : public SchemaBuilder {
public:

    explicit ExtensionSchema(rapidjson::Document::AllocatorType* allocator,
                             const std::string& version)
            : SchemaBuilder(allocator) {
        TYPE().Set(*mValue, "Schema", *mAllocator);
        VERSION().Set(*mValue, version.c_str(), *mAllocator);
        EVENTS().Set(*mValue, rapidjson::Value(rapidjson::kArrayType), *mAllocator);
        TYPES().Set(*mValue, rapidjson::Value(rapidjson::kArrayType), *mAllocator);
        COMMANDS().Set(*mValue, rapidjson::Value(rapidjson::kArrayType), *mAllocator);
        LIVE_DATA().Set(*mValue, rapidjson::Value(rapidjson::kArrayType), *mAllocator);
    }

    static const rapidjson::Pointer& TYPE() {
        static const rapidjson::Pointer ptr("/type");
        return ptr;
    }

    static const rapidjson::Pointer& VERSION() {
        static const rapidjson::Pointer ptr("/version");
        return ptr;
    }

    /**
     * The unique identifier for this extension.  The URI should follow RFC-3986
     * "Uniform Resource Identifier (URI): Generic Syntax", and identify the domain,
     * function, and version of the extension.  For example:
     *     alexaext:fishfeeder:10
     */
    ExtensionSchema& uri(const std::string& uri) {
        URI().Set(*mValue, uri.c_str(), *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& URI() {
        static const rapidjson::Pointer ptr("/uri");
        return ptr;
    }

    /**
     * Add an extension data type definition.
     * The types block of the extension schema defines extension data types. Extension data
     * types may be simple or complex, and extend primitive values, objects, arrays, or other types.
     */
    ExtensionSchema& dataType(const std::string& name,
                              const std::function<void(TypeSchema&)>& builder = nullptr) {
        factoryPush<TypeSchema>(TYPES(), builder, name);
        return *this;
    }

    static const rapidjson::Pointer& TYPES() {
        static const rapidjson::Pointer ptr("/types");
        return ptr;
    }

    /**
     * Add an extension event definition.
     * The events block of the extension schema defines events emitted by the
     * extension and received by the execution environment.
     */
    ExtensionSchema& event(const std::string& name,
                           const std::function<void(EventSchema&)>& builder = nullptr) {
        factoryPush<EventSchema>(EVENTS(), builder, name);
        return *this;
    }

    static const rapidjson::Pointer& EVENTS() {
        static const rapidjson::Pointer ptr("/events");
        return ptr;
    }

    /**
     * Add a extension command definition.
     * The commands block of the extension schema defines commands invoked from
     * the execution environment and executed by the extension.
     */
    ExtensionSchema& command(const std::string& name,
                             const std::function<void(CommandSchema&)>& builder = nullptr) {
        factoryPush<CommandSchema>(COMMANDS(), builder, name);
        return *this;
    }

    static const rapidjson::Pointer& COMMANDS() {
        static const rapidjson::Pointer ptr("/commands");
        return ptr;
    }

    /**
     * Add an extension live data array definition.
     * The liveData block of the extension Schema defines data streams published
     * by the extension and received by the execution environment.
     */
    ExtensionSchema& liveDataArray(const std::string& name,
                                   const std::function<void(LiveDataSchema&)>& builder = nullptr) {
        factoryPush<LiveDataSchema>(LIVE_DATA(), builder, name, true);
        return *this;
    }

    /**
     * Add an extension live data map definition.
     * The liveData block of the extension Schema defines data streams published
     * by the extension and received by the execution environment.
    */
    ExtensionSchema& liveDataMap(const std::string& name,
                                 const std::function<void(LiveDataSchema&)>& builder = nullptr) {
        factoryPush<LiveDataSchema>(LIVE_DATA(), builder, name, false);
        return *this;
    }

    static const rapidjson::Pointer& LIVE_DATA() {
        static const rapidjson::Pointer ptr("/liveData");
        return ptr;
    }
};

// forward declare TypeSchema builder dependencies for file readability
class TypePropertySchema;

/**
* Extension data type builder.
* Extension data types may be simple or complex, and extend primitive values, objects, arrays, or other types.
*/
class TypeSchema : public SchemaBuilder {
public:
    explicit TypeSchema(rapidjson::Document::AllocatorType* allocator,
                        const std::string& name)
            : SchemaBuilder(allocator) {
        NAME().Set(*mValue, name.c_str(), *mAllocator);
        PROPERTIES().Set(*mValue, rapidjson::Value(rapidjson::kObjectType), *mAllocator);
    }

    /// optional, extension of another data type
    TypeSchema& extends(const std::string& extends) {
        EXTENDS().Set(*mValue, extends.c_str(), *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& EXTENDS() {
        static const rapidjson::Pointer ptr("/extends");
        return ptr;
    }

    TypeSchema& property(const std::string& name, const std::string& type) {
        PROPERTIES().Get(*mValue)->AddMember(rapidjson::Value(name.c_str(), *mAllocator),
                                             rapidjson::Value(type.c_str(), *mAllocator), *mAllocator);
        return *this;
    }

    TypeSchema& property(const std::string& name,
                         const std::function<void(TypePropertySchema&)>& builder = nullptr) {
        factoryAdd<TypePropertySchema>(PROPERTIES(), name, builder);
        return *this;
    }

    static const rapidjson::Pointer& PROPERTIES() {
        static const rapidjson::Pointer ptr("/properties");
        return ptr;
    }
};

class TypePropertySchema : public SchemaBuilder {
public:
    explicit TypePropertySchema(rapidjson::Document::AllocatorType* allocator)
            : SchemaBuilder(allocator) {}

    /// "number" | "integer" | "bool" | "string" | "object" | another data type
    TypePropertySchema& type(const std::string& dataType) {
        TYPE().Set(*mValue, dataType.c_str(), *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& TYPE() {
        static const rapidjson::Pointer ptr("/type");
        return ptr;
    }

    TypePropertySchema& description(const std::string& description) {
        DESCRIPTION().Set(*mValue, description.c_str(), *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& DESCRIPTION() {
        static const rapidjson::Pointer ptr("/description");
        return ptr;
    }

    TypePropertySchema& required(bool required) {
        REQUIRED().Set(*mValue, required, *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& REQUIRED() {
        static const rapidjson::Pointer ptr("/required");
        return ptr;
    }

    /**
    * Set a property default value of type T. Supports primitive types:
    * tparam T Either bool, int, unsigned, int64_t, uint64_t, double, float, const char*
    */
    template<typename T>
    TypePropertySchema& defaultValue(const T value) {
        DEFAULT().Set(*mValue, value, *mAllocator);
        return *this;
    }

    // Set a complex property with move semantics.
    TypePropertySchema& defaultValue(rapidjson::Value& value) {
        DEFAULT().Set(*mValue, value, *mAllocator);
        return *this;
    }

    // Set a complex property with copy semantics.
    TypePropertySchema& defaultValue(const rapidjson::Value& value) {
        DEFAULT().Set(*mValue, value, *mAllocator);// pointer manages CopyFrom
        return *this;
    }

    static const rapidjson::Pointer& DEFAULT() {
        static const rapidjson::Pointer ptr("/default");
        return ptr;
    }
};

/**
 * Extension event builder.
 * An extension event is emitted by the extension and received by the execution environment.
 */
class EventSchema : public SchemaBuilder {
public:

    explicit EventSchema(rapidjson::Document::AllocatorType* allocator,
                         const std::string& name)
            : SchemaBuilder(allocator) {
        NAME().Set(*mValue, name.c_str(), *mAllocator);
    }

    EventSchema fastMode(bool fastMode) {
        FAST_MODE().Set(*mValue, fastMode, *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& FAST_MODE() {
        static const rapidjson::Pointer ptr("/fastMode");
        return ptr;
    }
};


/**
 * Extension command builder.
 * An extension command is invoked from the execution
 * environment and executed by the extension.
 */
class CommandSchema : public SchemaBuilder {
public:

    explicit CommandSchema(rapidjson::Document::AllocatorType* allocator,
                           const std::string& name) : SchemaBuilder(allocator) {
        NAME().Set(*mValue, name.c_str(), *mAllocator);
    }

    /// Data type of the command payload.
    CommandSchema& dataType(const std::string& dataType) {
        PAYLOAD().Set(*mValue, dataType.c_str(), *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& PAYLOAD() {
        static const rapidjson::Pointer ptr("/payload");
        return ptr;
    }

    CommandSchema& requireResponse(bool requireResponse) {
        REQUIRE_RESPONSE().Set(*mValue, requireResponse, *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& REQUIRE_RESPONSE() {
        static const rapidjson::Pointer ptr("/requireResponse");
        return ptr;
    }

    CommandSchema& description(const std::string& description) {
        DESCRIPTION().Set(*mValue, description.c_str(), *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& DESCRIPTION() {
        static const rapidjson::Pointer ptr("/description");
        return ptr;
    }

    CommandSchema& allowFastMode(bool allowFastMode) {
        ALLOW_FAST_MODE().Set(*mValue, allowFastMode, *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& ALLOW_FAST_MODE() {
        static const rapidjson::Pointer ptr("/allowFastMode");
        return ptr;
    }

};

//forward declare LiveDataSchema builder dependencies for file readability
class EventHandlerPropertySchema;

class EventHandlerSchema;

/**
 * Extension live data builder
 * Live data is a data stream published by the extension
 * and received by the execution environment.
 */
class LiveDataSchema : public SchemaBuilder {
public:

    explicit LiveDataSchema(rapidjson::Document::AllocatorType* allocator,
                            const std::string& name, bool isDataArray)
            : SchemaBuilder(allocator),
              mIsDataArray(isDataArray) {
        NAME().Set(*mValue, name.c_str(), *mAllocator);
        EVENTS().Set(*mValue, rapidjson::Value(rapidjson::kArrayType), *mAllocator);
    };

    LiveDataSchema& dataType(const std::string& dataType) {
        // naming convention for live data array appends [] to denote the array
        auto typeName = mIsDataArray ? dataType + "[]" : dataType;
        DATA_TYPE().Set(*mValue, typeName.c_str(), *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& DATA_TYPE() {
        static const rapidjson::Pointer ptr("/type");
        return ptr;
    }

    LiveDataSchema& eventHandler(const rapidjson::Pointer& operation, const std::string& eventHandler,
                                 const std::function<void(EventHandlerSchema&)>& builder = nullptr) {
        factoryAdd<EventHandlerSchema>(operation, builder, eventHandler);
        return *this;
    }

    static const rapidjson::Pointer& OPERATION_ADD() {
        static const rapidjson::Pointer ptr("/events/add");
        return ptr;
    }

    static const rapidjson::Pointer& OPERATION_REMOVE() {
        static const rapidjson::Pointer ptr("/events/remove");
        return ptr;
    }

    static const rapidjson::Pointer& OPERATION_UPDATE() {
        static const rapidjson::Pointer ptr("/events/update");
        return ptr;
    }

    static const rapidjson::Pointer& OPERATION_SET() {
        static const rapidjson::Pointer ptr("/events/set");
        return ptr;
    }

    static const rapidjson::Pointer& EVENTS() {
        static const rapidjson::Pointer ptr("/events");
        return ptr;
    }


private:
    bool mIsDataArray;
};

class EventHandlerSchema : public SchemaBuilder {
public:
    explicit EventHandlerSchema(rapidjson::Document::AllocatorType* allocator,
                                const std::string& eventHandler)
            : SchemaBuilder(allocator) {
        EVENT_HANDLER().Set(*mValue, eventHandler.c_str(), *mAllocator);
        PROPERTIES().Set(*mValue, rapidjson::Value(rapidjson::kArrayType), *mAllocator);
    }

    EventHandlerSchema& property(const std::string& name,
                                 const std::function<void(EventHandlerPropertySchema&)>& builder = nullptr) {
        factoryPush<EventHandlerPropertySchema>(PROPERTIES(), builder, name);
        return *this;
    }

    static const rapidjson::Pointer& EVENT_HANDLER() {
        static const rapidjson::Pointer ptr("/eventHandler");
        return ptr;
    }

    static const rapidjson::Pointer& PROPERTIES() {
        static const rapidjson::Pointer ptr("/properties");
        return ptr;
    }
};

class EventHandlerPropertySchema : public SchemaBuilder {
public:
    explicit EventHandlerPropertySchema(rapidjson::Document::AllocatorType* allocator,
                                        const std::string& name)
            : SchemaBuilder(allocator) {
        NAME().Set(*mValue, name.c_str(), *mAllocator);
    }

    EventHandlerPropertySchema& updateOnChange(bool updateOnChange) {
        UPDATE().Set(*mValue, updateOnChange, *mAllocator);
        return *this;
    }

    EventHandlerPropertySchema& collapse(bool collapse) {
        COLLAPSE().Set(*mValue, collapse, *mAllocator);
        return *this;
    }

    static const rapidjson::Pointer& UPDATE() {
        static const rapidjson::Pointer ptr("/update");
        return ptr;
    }

    static const rapidjson::Pointer& COLLAPSE() {
        static const rapidjson::Pointer ptr("/collapse");
        return ptr;
    }
};

} // namespace alexaext

#endif // ALEXAEXT_EXTENSIONSCHEMA_H


