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

#ifdef ALEXAEXTENSIONS

#include <memory>

#include "../testeventloop.h"
#include "../embed/testdocumentmanager.h"

using namespace apl;
using namespace alexaext;

class SimpleTestExtension final : public alexaext::ExtensionBase {
public:
    explicit SimpleTestExtension(const std::set<std::string>& uris) : ExtensionBase(uris) {};

    rapidjson::Document createRegistration(const std::string& uri, const rapidjson::Value& registerRequest) override {
        std::string schema = R"({
            "type": "Schema",
            "version": "1.0",
            "commands": [
              {
                "name": "Test"
              }
            ]
          })";
        rapidjson::Document doc;
        doc.Parse(schema.c_str());
        doc.AddMember("uri", rapidjson::Value(uri.c_str(), doc.GetAllocator()), doc.GetAllocator());
        return alexaext::RegistrationSuccess("1.0").uri(uri).token("I_AM_A_TOKEN").schema(doc);
    }

    bool invokeCommand(const std::string& uri, const rapidjson::Value& command) override {
        commandTriggered = true;
        return true;
    }

    bool commandTriggered = false;
};

class EmbeddedExtensionsTest : public DocumentWrapper {
public:
    EmbeddedExtensionsTest()
        : documentManager(std::make_shared<TestDocumentManager>())
    {
        config->documentManager(std::static_pointer_cast<DocumentManager>(documentManager));
    }

    void createProvider() {
        extensionProvider = std::make_shared<alexaext::ExtensionRegistrar>();
        mediator = ExtensionMediator::create(extensionProvider,
                                             alexaext::Executor::getSynchronousExecutor());
    }

    void loadExtensions(const char* document) {
        createContent(document, nullptr);

        if (!extensionProvider) {
            createProvider();
        }

        // Experimental feature required
        config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider)
            .extensionMediator(mediator);

        ensureRequestedExtensions(content->getExtensionRequests());

        // load them into config via the mediator
        mediator->loadExtensions(config->getExtensionFlags(), content);
    }

    void ensureRequestedExtensions(std::set<std::string> requestedExtensions) {
        // Create a test extension for every request unless it's been requested before
        for (auto& req: requestedExtensions) {
            if (testExtensions.count(req) > 0) continue;
            auto ext = std::make_shared<SimpleTestExtension>(std::set<std::string>({req}));
            auto proxy = std::make_shared<alexaext::LocalExtensionProxy>(ext);
            extensionProvider->registerExtension(proxy);
            // save direct access to extension for test use
            testExtensions.emplace(req, ext);
        }
    }

    void TearDown() override {
        extensionProvider = nullptr;
        mediator = nullptr;
        testExtensions.clear();

        DocumentWrapper::TearDown();
    }

    alexaext::ExtensionRegistrarPtr extensionProvider;                 // provider instance for tests
    ExtensionMediatorPtr mediator;
    std::map<std::string, std::weak_ptr<SimpleTestExtension>> testExtensions; // direct access to extensions for test

protected:
    std::shared_ptr<TestDocumentManager> documentManager;
};

static const char* HOST_DOC = R"({
  "type": "APL",
  "version": "2023.1",
  "extension": [
    {
      "uri": "aplext:hello:10",
      "name": "Hello"
    }
  ],
  "onMount": {
    "type": "Hello:Test"
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "top",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "SEND_EVENTER",
            "arguments": ["LOADED"]
          }
        ],
        "onFail": [
          {
            "type": "InsertItem",
            "sequencer": "SEND_EVENTER",
            "arguments": ["FAILED"]
          }
        ]
      }
    }
  }
})";

static const char* EMBEDDED_DOC_TRIES_EXTENSION = R"({
  "type": "APL",
  "version": "2023.2",
  "onMount": {
    "type": "Hello:Test"
  },
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText",
      "value": "Hello, World!"
    }
  }
})";

TEST_F(EmbeddedExtensionsTest, NoHostExtensionsAccess)
{
    loadExtensions(HOST_DOC);

    // verify the extension was registered
    ASSERT_TRUE(extensionProvider->hasExtension("aplext:hello:10"));
    auto ext = extensionProvider->getExtension("aplext:hello:10");
    ASSERT_TRUE(ext);
    // direct access to extension for test inspection
    auto hello = testExtensions["aplext:hello:10"].lock();

    // We have all we need. Inflate.
    inflate();

    // Check onMount triggered extension command.
    ASSERT_TRUE(hello->commandTriggered);
    hello->commandTriggered = false;

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    auto content = Content::create(EMBEDDED_DOC_TRIES_EXTENSION, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    // Check onMount not triggered extension command.
    ASSERT_FALSE(hello->commandTriggered);
    // Complained about command not been there
    ASSERT_TRUE(session->checkAndClear());
}

static const char* EMBEDDED_DOC_REQUESTS_HOST_EXTENSION = R"({
  "type": "APL",
  "version": "2023.2",
  "extension": [
    {
      "uri": "aplext:hello:10",
      "name": "Hello"
    }
  ],
  "onMount": {
    "type": "Hello:Test"
  },
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText",
      "value": "Hello, World!"
    }
  }
})";

TEST_F(EmbeddedExtensionsTest, NoHostRequestedExtensionsAccess)
{
    loadExtensions(HOST_DOC);

    // verify the extension was registered
    ASSERT_TRUE(extensionProvider->hasExtension("aplext:hello:10"));
    auto ext = extensionProvider->getExtension("aplext:hello:10");
    ASSERT_TRUE(ext);
    // direct access to extension for test inspection
    auto hello = testExtensions["aplext:hello:10"].lock();

    // We have all we need. Inflate.
    inflate();

    // Check onMount triggered extension command.
    ASSERT_TRUE(hello->commandTriggered);
    hello->commandTriggered = false;

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    auto content = Content::create(EMBEDDED_DOC_REQUESTS_HOST_EXTENSION, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    // Check onMount not triggered extension command.
    ASSERT_FALSE(hello->commandTriggered);
    // Complained about command not been there
    ASSERT_TRUE(session->checkAndClear());

    // Verify no extension handling set up
    auto coreDoc = std::static_pointer_cast<CoreDocumentContext>(embeddedDocumentContext);
    ASSERT_TRUE(coreDoc->rootConfig().getSupportedExtensions().empty());
    ASSERT_FALSE(coreDoc->rootConfig().getExtensionMediator());
    ASSERT_FALSE(coreDoc->rootConfig().getExtensionProvider());
}

static const char* EMBEDDED_DOC_WITH_ALLOWED_EXTENSION = R"({
  "type": "APL",
  "version": "2023.2",
  "extension": [
    {
      "uri": "aplext:goodbye:10",
      "name": "Bye"
    }
  ],
  "onMount": {
    "type": "Bye:Test"
  },
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText",
      "value": "Hello, World!"
    }
  }
})";

TEST_F(EmbeddedExtensionsTest, AccessGrantedToEmbeddedExtension)
{
    loadExtensions(HOST_DOC);

    // The provider knows about the extension that was requested by the host document
    ASSERT_TRUE(extensionProvider->hasExtension("aplext:hello:10"));
    // but not the one that will be requested by the embedded document
    ASSERT_FALSE(extensionProvider->hasExtension("aplext:goodbye:10"));

    // Load the embedded content a little early, so that we know what extensions it wants
    auto embeddedContent = Content::create(EMBEDDED_DOC_WITH_ALLOWED_EXTENSION, session);
    ASSERT_TRUE(embeddedContent->isReady());

    ensureRequestedExtensions(embeddedContent->getExtensionRequests());

    // Now the other extension is available
    ASSERT_TRUE(extensionProvider->hasExtension("aplext:goodbye:10"));

    // Direct access to extensions for test inspection
    auto hello = testExtensions["aplext:hello:10"].lock();
    auto goodbye = testExtensions["aplext:goodbye:10"].lock();

    // Inflate the primary document
    inflate();

    // Reset the Hello command triggered flag (which triggered in primary document)
    ASSERT_TRUE(hello->commandTriggered);
    hello->commandTriggered = false;

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    auto request = requestWeak.lock();
    ASSERT_TRUE(request);
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // Prepare a fresh mediator for the embedded document
    auto embeddedMediator = ExtensionMediator::create(extensionProvider,
                                                      alexaext::Executor::getSynchronousExecutor());
    embeddedMediator->loadExtensions(ObjectMap(), embeddedContent);

    // Prepare document config
    auto documentConfig = DocumentConfig::create();
    documentConfig->extensionMediator(embeddedMediator);

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", embeddedContent, true, documentConfig);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    // Check onMount not triggered Hello extension command.
    ASSERT_FALSE(hello->commandTriggered);

    // Check onMount triggered Goodbye extension command.
    ASSERT_TRUE(goodbye->commandTriggered);
    ASSERT_FALSE(session->checkAndClear());

    // Verify that mediator is set up
    auto coreDoc = std::static_pointer_cast<CoreDocumentContext>(embeddedDocumentContext);
    ASSERT_TRUE(coreDoc->rootConfig().getExtensionMediator());
}

#endif
