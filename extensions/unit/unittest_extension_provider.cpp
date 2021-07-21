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

#include <alexaext/alexaext.h>
#include <rapidjson/document.h>

#include "gtest/gtest.h"

using namespace alexaext;
using namespace rapidjson;

class SimpleExtension final : public ExtensionBase {

public:

    explicit SimpleExtension(const std::string& uri) : ExtensionBase(uri) {};

    explicit SimpleExtension(const std::set<std::string>& uris) : ExtensionBase(uris) {};

    bool invokeCommand(const std::string& uri, const rapidjson::Value& command) override {
        if (getURIs().count("aplext:ugly:1")) {
            throw ExtensionException::create("ugly %s %s", "exception", "error");
        }
        auto name = Command::NAME().Get(command);
        return (name && *name != "nope");
    }

    rapidjson::Document createRegistration(const std::string& uri, const rapidjson::Value& registerRequest) override {

        if (uri == "aplext:ugly:1") {
            throw ExtensionException::create("ugly %s %s", "exception", "error");
        } else if (uri == "aplext:ugly:2") {
            return RegistrationFailure("1.0").uri("aplext:ugly:2").errorCode(13).errorMessage("total failure");
        }

        const Value* config = RegistrationRequest::SETTINGS().Get(registerRequest);

        // extract the settings
        if (config->IsObject()) {
            auto obj = config->GetObject();
            if (obj.HasMember("A"))
                settingA = obj["A"].GetInt();
            if (obj.HasMember("B"))
                settingsB = obj["B"].GetString();
        }

        Document environment;
        environment.CopyFrom(*config, environment.GetAllocator()); // sending the settings back as environment

        auto registration = RegistrationSuccess("1.0").uri(uri).token("SessionToken1")
                .environment(environment)
                .schema("1.0", [uri](ExtensionSchema schema) {
                    schema.uri(uri).event("boo");
                });

        return registration;
    }

    // test method to simulate internally generated events
    bool generateTestEvent(const std::string& uri, const std::string& eventName) {
        return invokeExtensionEventHandler(uri, Event("1.0").uri(uri).name(eventName));
    }

    // test method to simulate internally generated live data updates
    bool generateLiveDataUpdate(const std::string& uri, const std::string& objectName) {
        return invokeLiveDataUpdate(uri, LiveDataUpdate("1.0").uri(uri).objectName(objectName));
    }

    int settingA;
    std::string settingsB;
};

/**
 * Test class;
 */
class ExtensionProviderTest : public ::testing::Test {
public:
    void SetUp() override {

        // Set up the extension provider (exercising the shared pointer cast for test purposes)
        auto extRegistrar = std::make_shared<ExtensionRegistrar>();
        extPro = std::static_pointer_cast<ExtensionProvider>(extRegistrar);

        // create a proxy to access a local extension, the extension is
        // created immediately
        auto foo = std::make_shared<SimpleExtension>(
                std::set<std::string>({"aplext:foo:10", "aplext:foo:11"}));
        auto pFoo = std::make_shared<LocalExtensionProxy>(foo);

        // create a proxy to access a local extension, the extension creation
        // is deferred
        auto pBar =
                std::make_shared<LocalExtensionProxy>("aplext:bar:10", [this](const std::string& uri) {
                    auto bar = std::make_shared<SimpleExtension>(std::set<std::string>({uri}));
                    // save extension map for test use
                    testExtensions.emplace(uri, bar);
                    return bar;
                });

        // create a proxy to access a local extension that is poorly behaved
        // the extension is created immediately
        auto ugly = std::make_shared<SimpleExtension>(std::set<std::string>({"aplext:ugly:1", "aplext:ugly:2"}));
        auto pUgly = std::make_shared<LocalExtensionProxy>(ugly);

        // Example of Runtime registration
        extRegistrar->registerExtension(pFoo)
                .registerExtension(pBar)
                .registerExtension(pUgly);


        // test use only: save extension map for access within the tests
        testExtensions.emplace("aplext:foo:10", foo);
        testExtensions.emplace("aplext:foo:11", foo);
        testExtensions.emplace("aplext:ugly:1", ugly);
        testExtensions.emplace("aplext:ugly:2", ugly);
    }

    // Used for testing message values
    class AssertMessage : public BaseMessage<AssertMessage> {
    };

    void AssertMessage(const std::string& target, const std::string& method, const rapidjson::Value& message) {
        ASSERT_TRUE(message.IsObject());
        ASSERT_EQ(target, GetWithDefault<const char*>(AssertMessage::TARGET(), message, ""));
        ASSERT_EQ(method, GetWithDefault<const char*>(AssertMessage::METHOD(), message, ""));
        ASSERT_STREQ("1.0", GetWithDefault<const char*>(AssertMessage::VERSION(), message, ""));
    }

    void TearDown() override {
        extPro = nullptr;
        testExtensions.clear();
        ::testing::Test::TearDown();
    }

    ExtensionProviderPtr extPro;                          // provider instance for tests
    std::map<std::string, ExtensionPtr> testExtensions; // direct access to extensions for test
    rapidjson::Document document;
};


static const char* SETTINGS = R"(
{
    "A": 64,
    "B": "hello"
}
)";


/**
 * Test the simple api of an ExtensionRegister.
 */
TEST_F(ExtensionProviderTest, BasicRegister) {

    // the test registry has 3 test extensions
    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    ASSERT_TRUE(extPro->hasExtension("aplext:foo:11"));
    ASSERT_TRUE(extPro->hasExtension("aplext:bar:10"));
}

/**
 * Test successful get of an extension.
 */
TEST_F(ExtensionProviderTest, GetExtensionSuccess) {

    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);
}

/**
 * Test failure get of an extension.
 */
TEST_F(ExtensionProviderTest, GetExtensionFailure) {

    ASSERT_FALSE(extPro->hasExtension("nope"));
    auto nope = extPro->getExtension("nope");
    ASSERT_FALSE(nope);
}

/**
 * Test registration success callback.
 */
TEST_F(ExtensionProviderTest, RegistrationSuccess) {
    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    document.Parse(SETTINGS);
    Document req = RegistrationRequest("1.0").uri("aplext:foo:10").settings(document);

    bool gotsuccess = false;
    auto invoke = foo->getRegistration(
            "aplext:foo:10", req,
            [this, &gotsuccess](const std::string& uri, const rapidjson::Value& registerSuccess) {
                gotsuccess = true;
                ASSERT_EQ("aplext:foo:10", uri);
                AssertMessage(uri, "RegisterSuccess", registerSuccess);
            },
            nullptr);
    ASSERT_TRUE(invoke);
    ASSERT_TRUE(gotsuccess);

    auto ext = testExtensions.at("aplext:foo:10");
    auto simple = std::static_pointer_cast<SimpleExtension>(ext);
    ASSERT_EQ(64, simple->settingA);
    ASSERT_EQ("hello", simple->settingsB);
}

/**
 * Test registration success without without callback.
 */
TEST_F(ExtensionProviderTest, RegistrationSuccessNullCallbacks) {
    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    document.Parse(SETTINGS);
    Document req = RegistrationRequest("1.0").uri("aplext:foo:10").settings(document);

    // test null callbacks are tolerated
    auto invoke = foo->getRegistration("aplext:foo:10", req, nullptr, nullptr);
    ASSERT_TRUE(invoke);
}

/**
 * Test registration failure callback.
 */
TEST_F(ExtensionProviderTest, RegistrationFailure) {
    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    document.Parse(SETTINGS);
    Document req = RegistrationRequest("1.0").uri("aplext:foo:10").settings(document);

    bool gotfailure = false;
    auto invoke = foo->getRegistration(
            "DNE", req,
            nullptr, [this, &gotfailure](const std::string& uri, const rapidjson::Value& registerFailure) {
                gotfailure = true;
                ASSERT_EQ("DNE", uri);
                AssertMessage(uri, "RegisterFailure", registerFailure);
                ASSERT_EQ(kErrorUnknownURI, GetWithDefault<int>(RegistrationFailure::CODE(), registerFailure, -1));
                ASSERT_EQ((sErrorMessage[kErrorUnknownURI] + uri),
                          GetWithDefault<const char*>(RegistrationFailure::MESSAGE(), registerFailure, ""));
            });
    ASSERT_FALSE(invoke);
    ASSERT_TRUE(gotfailure);
}

/**
 * Test registration exception
 */
TEST_F(ExtensionProviderTest, RegistrationException) {
    ASSERT_TRUE(extPro->hasExtension("aplext:ugly:1"));
    auto ugly = extPro->getExtension("aplext:ugly:1");
    ASSERT_TRUE(ugly);

    document.Parse(SETTINGS);
    Document req = RegistrationRequest("1.0").uri("aplext:ugly:1").settings(document);

    bool gotfailure = false;
    auto invoke = ugly->getRegistration(
            "aplext:ugly:1", req,
            nullptr, [this, &gotfailure](const std::string& uri, const rapidjson::Value& registerFailure) {
                gotfailure = true;
                ASSERT_EQ("aplext:ugly:1", uri);
                AssertMessage(uri, "RegisterFailure", registerFailure);
                ASSERT_EQ(kErrorExtensionException,
                          GetWithDefault<int>(RegistrationFailure::CODE(), registerFailure, -1));
                ASSERT_STREQ("ugly exception error",
                             GetWithDefault<const char*>(RegistrationFailure::MESSAGE(), registerFailure, ""));
            });
    ASSERT_FALSE(invoke);
    ASSERT_TRUE(gotfailure);
}

/**
 * Test registration failure from extension.
 */
TEST_F(ExtensionProviderTest, RegistrationFailureFromExtension) {
    ASSERT_TRUE(extPro->hasExtension("aplext:ugly:2"));
    auto ugly = extPro->getExtension("aplext:ugly:2");
    ASSERT_TRUE(ugly);

    document.Parse(SETTINGS);
    Document req = RegistrationRequest("1.0").uri("aplext:ugly:2").settings(document);

    bool gotfailure = false;
    auto invoke = ugly->getRegistration(
            "aplext:ugly:2", req,
            nullptr, [this, &gotfailure](const std::string& uri, const rapidjson::Value& registerFailure) {
                gotfailure = true;
                ASSERT_EQ("aplext:ugly:2", uri);
                AssertMessage(uri, "RegisterFailure", registerFailure);
                ASSERT_EQ(13, GetWithDefault<int>(RegistrationFailure::CODE(), registerFailure, -1));
                ASSERT_STREQ("total failure",
                             GetWithDefault<const char*>(RegistrationFailure::MESSAGE(), registerFailure, ""));
            });
    ASSERT_TRUE(invoke);
    ASSERT_TRUE(gotfailure);
}

/**
 * Test get registration failure without callback.
 */
TEST_F(ExtensionProviderTest, GetRegistrationFailureNullCallback) {
    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    document.Parse(SETTINGS);
    Document req = RegistrationRequest("aplext:foo:10").settings(document);

    // test null callbacks are tolerated
    auto invoke = foo->getRegistration("aplext:foo:10", req, nullptr, nullptr);
    ASSERT_TRUE(invoke);
}


/**
 * Test registration success callback.
 */
TEST_F(ExtensionProviderTest, RegistrationSuccessSettings) {
    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    document.Parse(SETTINGS);
    Document req = RegistrationRequest("aplext:foo:10").settings(document);

    bool gotsuccess = false;
    auto invoke = foo->getRegistration(
            "aplext:foo:10", req,
            [this, &gotsuccess](const std::string& uri, const rapidjson::Value& registerSuccess) {
                gotsuccess = true;
                ASSERT_EQ("aplext:foo:10", uri);
                AssertMessage(uri, "RegisterSuccess", registerSuccess);
            },
            nullptr);
    ASSERT_TRUE(invoke);
    ASSERT_TRUE(gotsuccess);

    // Extension settings were set into the extension
    auto ext = testExtensions.at("aplext:foo:10");
    auto simple = std::static_pointer_cast<SimpleExtension>(ext);
    ASSERT_EQ(64, simple->settingA);
    ASSERT_EQ("hello", simple->settingsB);
}

/**
 * Test registration success callback.
 */
TEST_F(ExtensionProviderTest, RegistrationSettingsEnvironment) {
    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    document.Parse(SETTINGS);
    Document req = RegistrationRequest("aplext:foo:10").settings(document);

    bool gotsuccess = false;
    auto invoke = foo->getRegistration(
            "aplext:foo:10", req,
            [this, &gotsuccess](const std::string& uri, const rapidjson::Value& registerSuccess) {
                gotsuccess = true;
                ASSERT_EQ("aplext:foo:10", uri);
                AssertMessage(uri, "RegisterSuccess", registerSuccess);
                ASSERT_TRUE(registerSuccess.HasMember("environment"));
                const auto& environment = RegistrationSuccess::ENVIRONMENT().Get(registerSuccess);
                // extract the settings
                ASSERT_TRUE(environment);
                ASSERT_TRUE(environment->IsObject());
                auto obj = environment->GetObject();
                ASSERT_TRUE(obj.HasMember("A"));
                ASSERT_EQ(64, obj["A"]);
                ASSERT_TRUE(obj.HasMember("B"));
                ASSERT_EQ("hello", obj["B"]);
            },
            nullptr);
    ASSERT_TRUE(invoke);
    ASSERT_TRUE(gotsuccess);

    auto ext = testExtensions.at("aplext:foo:10");
    auto simple = std::static_pointer_cast<SimpleExtension>(ext);
    ASSERT_EQ(64, simple->settingA);
    ASSERT_EQ("hello", simple->settingsB);
}

/**
 * Test invoke command on an extension. Message from doc to extnsion.
 */
TEST_F(ExtensionProviderTest, InvokeCommandSuccess) {


    int id = 31;
    Document command = Command("1.0").uri("aplext:foo:10").name("command1").id(id)
            .property("prop1", Value(1).Move());


    // the extension was registered
    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    // test success callback
    bool gotsuccess = false;
    bool invoke = foo->invokeCommand(
            "aplext:foo:10", command,
            [this, &id, &gotsuccess](const std::string& uri, const rapidjson::Value& commandSuccess) {
                gotsuccess = true;
                ASSERT_EQ("aplext:foo:10", uri);
                AssertMessage(uri, "CommandSuccess", commandSuccess);
                ASSERT_TRUE(commandSuccess.HasMember("id"));
                ASSERT_EQ(id, commandSuccess["id"]);
            },
            nullptr);
    ASSERT_TRUE(invoke);
    ASSERT_TRUE(gotsuccess);
}

/**
 * Test get definition success without without callback.
 */
TEST_F(ExtensionProviderTest, InvokeCommandSuccessNullCallbacks) {
    Document command = Command("1.0").uri("aplext:foo:10").name("command1").id(31).property("prop1", Value(1).Move());

    // the extension was registered
    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);
    // test null callback is tolerated
    auto invoke = foo->invokeCommand("aplext:foo:10", command, nullptr, nullptr);
    ASSERT_TRUE(invoke);

}

/**
 * Test invoke event handler.  This is a message from extension to doc.
 */
TEST_F(ExtensionProviderTest, InvokeCommandFailure) {

    int id = 31;
    Document command = Command("1.0").uri("aplext:foo:10").name("nope").id(id).property("prop1", Value(1).Move());

    // the extension was registered
    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    // test failure callback
    bool gotfailure = false;
    bool invoke = foo->invokeCommand(
            "aplext:foo:10", command,
            nullptr,
            [this, &gotfailure, &id](const std::string& uri, const rapidjson::Value& commandFailure) {
                gotfailure = true;
                ASSERT_EQ("aplext:foo:10", uri);
                this->AssertMessage(uri, "CommandFailure", commandFailure);
                ASSERT_TRUE(commandFailure.HasMember("code"));
                ASSERT_EQ(kErrorFailedCommand, GetWithDefault<int>(CommandFailure::CODE(), commandFailure, -1));
                ASSERT_EQ(sErrorMessage[kErrorFailedCommand] + std::to_string(id),
                          GetWithDefault<const char*>(CommandFailure::MESSAGE(), commandFailure, ""));
            });
    ASSERT_FALSE(invoke);
    ASSERT_TRUE(gotfailure);
}

/**
 * Test invoke event handler.  This is a message from extension to doc.
 */
TEST_F(ExtensionProviderTest, InvokeCommandException) {

    int id = 31;
    Document command = Command("1.0").uri("aplext:ugly:1").name("ugly").id(id).property("prop1", Value(1).Move());

    // the extension was registered
    ASSERT_TRUE(extPro->hasExtension("aplext:ugly:1"));
    auto foo = extPro->getExtension("aplext:ugly:1");
    ASSERT_TRUE(foo);

    // test failure callback
    bool gotfailure = false;
    bool invoke = foo->invokeCommand(
            "aplext:ugly:1", command,
            nullptr,
            [this, &gotfailure, &id](const std::string& uri, const rapidjson::Value& commandFailure) {
                gotfailure = true;
                ASSERT_EQ("aplext:ugly:1", uri);
                AssertMessage(uri, "CommandFailure", commandFailure);
                ASSERT_EQ(id, GetWithDefault<int>(Command::ID(), commandFailure, -1));
                ASSERT_EQ(kErrorExtensionException, GetWithDefault<int>(CommandFailure::CODE(), commandFailure, -1));
                ASSERT_STREQ("ugly exception error",
                             GetWithDefault<const char*>(CommandFailure::MESSAGE(), commandFailure, ""));
            });
    ASSERT_FALSE(invoke);
    ASSERT_TRUE(gotfailure);
}

/**
 * Test successful receipt of event generated by extension.
 */
TEST_F(ExtensionProviderTest, InvokeEventHandlerSuccess) {

    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    bool gotsuccess = false;
    foo->registerEventCallback(
            [this, &gotsuccess](const std::string& uri, const rapidjson::Value& event) {
                gotsuccess = true;
                ASSERT_EQ("aplext:foo:10", uri);
                AssertMessage(uri, "Event", event);
                ASSERT_STREQ("hello", GetWithDefault<const char*>(Event::NAME(), event, ""));
            });

    // simulate an internal generated event
    auto ext = testExtensions.at("aplext:foo:10");
    ASSERT_TRUE(foo);
    auto simple = std::static_pointer_cast<SimpleExtension>(ext);
    ASSERT_TRUE(simple);
    bool handled = simple->generateTestEvent("aplext:foo:10", "hello");

    // the event is handled and received by the callback.
    ASSERT_TRUE(handled);
    ASSERT_TRUE(gotsuccess);
}

/**
 * Test event generation without callback.
 */
TEST_F(ExtensionProviderTest, InvokeEventHandlerNullCallback) {

    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    // no callback is register, handled gracefully

    // simulate an internal generated event
    auto ext = testExtensions.at("aplext:foo:10");
    ASSERT_TRUE(foo);
    auto simple = std::static_pointer_cast<SimpleExtension>(ext);
    ASSERT_TRUE(simple);
    bool handled = simple->generateTestEvent("aplext:foo:10", "FooEvent");

    // The local proxy always installs a handler, so the event is handled
    ASSERT_TRUE(handled);

}

/**
 * Test that multiple event handlers can be registered.
 */
TEST_F(ExtensionProviderTest, MultipleEventHandlers) {

    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    bool firstSuccess = false;
    bool secondSuccess = false;
    foo->registerEventCallback(
        [&](const std::string& uri, const rapidjson::Value& event) {
          firstSuccess = true;
        });
    foo->registerEventCallback(
        [&](const std::string& uri, const rapidjson::Value& event) {
          secondSuccess = true;
        });

    // simulate an internal generated event
    auto ext = testExtensions.at("aplext:foo:10");
    ASSERT_TRUE(foo);
    auto simple = std::static_pointer_cast<SimpleExtension>(ext);
    ASSERT_TRUE(simple);
    bool handled = simple->generateTestEvent("aplext:foo:10", "hello");

    // the event is handled and received by the callback.
    ASSERT_TRUE(handled);
    ASSERT_TRUE(firstSuccess);
    ASSERT_TRUE(secondSuccess);
}

/**
 * Test successful receipt of live data update generated by extension.
 */
TEST_F(ExtensionProviderTest, InvokeLiveDataUpdate) {

    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    bool gotsuccess = false;
    foo->registerLiveDataUpdateCallback(
            [this, &gotsuccess](const std::string& uri, const rapidjson::Value& liveDataUpdate) {
                gotsuccess = true;
                ASSERT_EQ("aplext:foo:10", uri);
                AssertMessage(uri, "LiveDataUpdate", liveDataUpdate);
                ASSERT_STREQ("HelloObject", GetWithDefault<const char*>(LiveDataUpdate::OBJECT_NAME(), liveDataUpdate, ""));
            });

    // simulate an internal generated event
    auto ext = testExtensions.at("aplext:foo:10");
    ASSERT_TRUE(foo);
    auto simple = std::static_pointer_cast<SimpleExtension>(ext);
    ASSERT_TRUE(simple);
    bool handled = simple->generateLiveDataUpdate("aplext:foo:10", "HelloObject");

    // the event is handled and received by the callback.
    ASSERT_TRUE(handled);
    ASSERT_TRUE(gotsuccess);
}

/**
 * Test live data update without callback.
 */
TEST_F(ExtensionProviderTest, InvokeLiveDataUpdateNullCallback) {

    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    // no callback is register, handled gracefully

    // simulate an internal generated event
    auto ext = testExtensions.at("aplext:foo:10");
    ASSERT_TRUE(foo);
    auto simple = std::static_pointer_cast<SimpleExtension>(ext);
    ASSERT_TRUE(simple);
    bool handled = simple->generateLiveDataUpdate("aplext:foo:10", "FooObject");

    // The local proxy always installs a handler, so the update is handled
    ASSERT_TRUE(handled);
}


/**
 * Test that multiple live data update callbacks can be registered.
 */
TEST_F(ExtensionProviderTest, MultipleLiveDataUpdateCallbacks) {

    ASSERT_TRUE(extPro->hasExtension("aplext:foo:10"));
    auto foo = extPro->getExtension("aplext:foo:10");
    ASSERT_TRUE(foo);

    bool firstSuccess = false;
    bool secondSuccess = false;
    foo->registerLiveDataUpdateCallback(
        [&firstSuccess](const std::string& uri, const rapidjson::Value& liveDataUpdate) {
          firstSuccess = true;
        });
    foo->registerLiveDataUpdateCallback(
        [&secondSuccess](const std::string& uri, const rapidjson::Value& liveDataUpdate) {
          secondSuccess = true;
        });

    // simulate an internal generated event
    auto ext = testExtensions.at("aplext:foo:10");
    ASSERT_TRUE(foo);
    auto simple = std::static_pointer_cast<SimpleExtension>(ext);
    ASSERT_TRUE(simple);
    bool handled = simple->generateLiveDataUpdate("aplext:foo:10", "HelloObject");

    // the event is handled and received by the callback.
    ASSERT_TRUE(handled);
    ASSERT_TRUE(firstSuccess);
    ASSERT_TRUE(secondSuccess);
}
