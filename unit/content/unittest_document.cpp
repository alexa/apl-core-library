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

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "gtest/gtest.h"
#include "apl/content/content.h"
#include "apl/content/metrics.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/context.h"
#include "apl/engine/rootcontext.h"
#include "apl/content/importrequest.h"

using namespace apl;

const char *BASIC_DOC =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"item\": {"
    "      \"type\": \"Text\""
    "    }"
    "  }"
    "}";

TEST(DocumentTest, Load)
{
    auto content = Content::create(BASIC_DOC, makeDefaultSession());

    ASSERT_FALSE(content->isReady());
    ASSERT_FALSE(content->isWaiting());
    ASSERT_FALSE(content->isError());

    ASSERT_EQ(1, content->getParameterCount());
    ASSERT_EQ(std::string("payload"), content->getParameterAt(0));
    content->addData("payload", "\"duck\"");
    ASSERT_TRUE(content->isReady());

    auto m = Metrics().size(1024,800).theme("dark");
    auto config = RootConfig().defaultIdleTimeout(15000);
    auto doc = RootContext::create(m, content, config);

    ASSERT_TRUE(doc);
    ASSERT_EQ(15000, content->getDocumentSettings()->idleTimeout(config));
    ASSERT_EQ(15000, doc->settings().idleTimeout());
}

const char *BASIC_DOC_NO_TYPE_FIELD =
        "{"
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"Text\""
        "    }"
        "  }"
        "}";

TEST(DocumentTest, NoTypeField)
{
    auto content = Content::create(BASIC_DOC_NO_TYPE_FIELD, makeDefaultSession());
    ASSERT_FALSE(content);
}

const char *BASIC_DOC_BAD_TYPE_FIELD =
        "{"
        "  \"type\": \"APMLTemplate\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"Text\""
        "    }"
        "  }"
        "}";

TEST(DocumentTest, DontEnforceBadTypeField)
{
    auto content = Content::create(BASIC_DOC_BAD_TYPE_FIELD, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto m = Metrics().size(1024,800).theme("dark");
    auto config = RootConfig();
    auto doc = RootContext::create(m, content, config);
    ASSERT_TRUE(doc);
}

TEST(DocumentTest, EnforceBadTypeField)
{
    auto content = Content::create(BASIC_DOC_BAD_TYPE_FIELD, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto m = Metrics().size(1024,800).theme("dark");
    auto config = RootConfig().enforceTypeField(true);
    auto doc = RootContext::create(m, content, config);
    ASSERT_FALSE(doc);
}

const char *BASIC_DOC_WITH_SETTINGS =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"settings\": {"
        "    \"idleTimeout\": 10000"
        "  },"
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"Text\""
        "    }"
        "  }"
        "}";

TEST(DocumentTest, Settings)
{
    auto content = Content::create(BASIC_DOC_WITH_SETTINGS, makeDefaultSession());

    ASSERT_TRUE(content->isReady());

    auto m = Metrics().size(1024,800).theme("dark");
    auto doc = RootContext::create(m, content);

    ASSERT_TRUE(doc);
    ASSERT_EQ(10000, content->getDocumentSettings()->idleTimeout(doc->rootConfig()));
    ASSERT_EQ(10000, doc->settings().idleTimeout());
}

// NOTE: Backward compatibility for some APL 1.0 users where a runtime allowed "features" instead of "settings"
static const char *BASIC_DOC_WITH_FEATURES =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"features\": {"
        "    \"idleTimeout\": 10002"
        "  },"
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"Text\""
        "    }"
        "  }"
        "}";

TEST(DocumentTest, Features)
{
    auto content = Content::create(BASIC_DOC_WITH_FEATURES, makeDefaultSession());

    ASSERT_TRUE(content->isReady());

    auto m = Metrics().size(1024,800).theme("dark");
    auto doc = RootContext::create(m, content);

    ASSERT_TRUE(doc);
    ASSERT_EQ(10002, doc->settings().idleTimeout());
    ASSERT_EQ(10002, content->getDocumentSettings()->idleTimeout(doc->rootConfig()));
}


// NOTE: Ensure that "settings" overrides "features"
static const char *BASIC_DOC_WITH_FEATURES_AND_SETTINGS =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"features\": {"
        "    \"idleTimeout\": 10002"
        "  },"
        "  \"settings\": {"
        "    \"idleTimeout\": 80000"
        "  },"
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"Text\""
        "    }"
        "  }"
        "}";

TEST(DocumentTest, SettingsAndFeatures)
{
    auto content = Content::create(BASIC_DOC_WITH_FEATURES_AND_SETTINGS, makeDefaultSession());

    ASSERT_TRUE(content->isReady());

    auto m = Metrics().size(1024,800).theme("dark");
    auto doc = RootContext::create(m, content);

    ASSERT_TRUE(doc);
    ASSERT_EQ(80000, doc->settings().idleTimeout());
    ASSERT_EQ(80000, content->getDocumentSettings()->idleTimeout(doc->rootConfig()));
}

const char *BASIC_DOC_WITH_USER_DEFINED_SETTINGS =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"settings\": {"
        "    \"idleTimeout\": 20000,"
        "    \"userSettingString\": \"MyValue\","
        "    \"userSettingNumber\": 500,"
        "    \"userSettingBool\": true,"
        "    \"userSettingDimension\": \"100dp\","
        "    \"userSettingArray\": ["
        "      \"valueA\","
        "      \"valueB\","
        "      \"valueC\""
        "    ],"
        "    \"userSettingMap\": {"
        "      \"keyA\": \"valueA\","
        "      \"keyB\": \"valueB\""
        "    }"
        "  },"
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"Text\""
        "    }"
        "  }"
        "}";

TEST(DocumentTest, UserDefinedSettings)
{
    auto content = Content::create(BASIC_DOC_WITH_USER_DEFINED_SETTINGS, makeDefaultSession());

    ASSERT_TRUE(content->isReady());

    auto m = Metrics().size(1024,800).theme("dark");
    auto doc = RootContext::create(m, content);
    auto context = doc->contextPtr();

    auto settings = content->getDocumentSettings();

    ASSERT_TRUE(settings);
    ASSERT_EQ(Object::NULL_OBJECT(), settings->getValue("settingAbsent"));
    ASSERT_EQ(20000, settings->idleTimeout());
    ASSERT_STREQ("MyValue",settings->getValue("userSettingString").getString().c_str());
    ASSERT_EQ(500, settings->getValue("userSettingNumber").getInteger());
    ASSERT_TRUE(settings->getValue("userSettingBool").getBoolean());
    ASSERT_EQ(Object(Dimension(100)), settings->getValue("userSettingDimension").asDimension(*context));
    ASSERT_EQ(Object::kArrayType, settings->getValue("userSettingArray").getType());
    ASSERT_EQ(Object::kMapType, settings->getValue("userSettingMap").getType());

}

const char *BASIC_DOC_WITHOUT_SETTINGS =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"Text\""
        "    }"
        "  }"
        "}";

TEST(DocumentTest, WithoutSettings)
{
    auto content = Content::create(BASIC_DOC_WITHOUT_SETTINGS, makeDefaultSession());

    ASSERT_TRUE(content->isReady());

    auto m = Metrics().size(1024,800).theme("dark");
    auto doc = RootContext::create(m, content);

    ASSERT_TRUE(doc);
    ASSERT_EQ(30000, doc->settings().idleTimeout());
    ASSERT_EQ(30000, content->getDocumentSettings()->idleTimeout(doc->rootConfig()));
    ASSERT_EQ(Object::NULL_OBJECT(), doc->settings().getValue("userSetting"));
}


TEST(DocumentTest, LoadError) {
    auto content = Content::create("cannotParse", makeDefaultSession());
    ASSERT_FALSE(content);
}

const char *ONE_DEPENDENCY =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"import\": ["
    "    {"
    "      \"name\": \"basic\","
    "      \"version\": \"1.2\""
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"item\": {"
    "      \"type\": \"Text\""
    "    }"
    "  }"
    "}";

const char *BASIC_PACKAGE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\""
    "}";

const char *ONE_DEPENDENCY_VERSION =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"import\": ["
        "    {"
        "      \"name\": \"basic\","
        "      \"version\": \"1.2\""
        "    }"
        "  ],"
        "  \"mainTemplate\": {"
        "    \"parameters\": ["
        "      \"payload\""
        "    ],"
        "    \"item\": {"
        "      \"type\": \"Text\""
        "    }"
        "  }"
        "}";

TEST(DocumentTest, LoadOneDependency)
{
    auto content = Content::create(ONE_DEPENDENCY, makeDefaultSession());

    ASSERT_TRUE(content);
    ASSERT_FALSE(content->isReady());
    ASSERT_TRUE(content->isWaiting());
    ASSERT_FALSE(content->isError());

    auto requested = content->getRequestedPackages();
    ASSERT_EQ(1, requested.size());
    auto it = requested.begin();
    ASSERT_STREQ("basic", it->reference().name().c_str());
    ASSERT_STREQ("1.2", it->reference().version().c_str());

    // The requested list is cleared
    ASSERT_EQ(0, content->getRequestedPackages().size());
    ASSERT_TRUE(content->isWaiting());

    content->addPackage(*it, BASIC_PACKAGE);
    ASSERT_FALSE(content->isWaiting());

    ASSERT_EQ("1.1", content->getAPLVersion());
}

const char *INCOMPATIBLE_MAIN =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.very_custom_version\","
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"Text\""
        "    }"
        "  }"
        "}";

TEST(DocumentTest, IncompatibleMainVersion)
{
    auto content = Content::create(INCOMPATIBLE_MAIN, makeDefaultSession());

    ASSERT_TRUE(content);
    ASSERT_FALSE(content->isWaiting());

    ASSERT_EQ("1.very_custom_version", content->getAPLVersion());

    auto m = Metrics().size(1024,800).theme("dark");
    auto rootConfig = RootConfig().enforceAPLVersion(APLVersion::kAPLVersionLatest);
    auto doc = RootContext::create(m, content, rootConfig);

    ASSERT_FALSE(doc);
}

const char *BASIC_INCOMPATIBLE_PACKAGE =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.very_custom_version\""
        "}";

TEST(DocumentTest, IncompatibleImportVersion)
{
    auto content = Content::create(ONE_DEPENDENCY, makeDefaultSession());

    ASSERT_TRUE(content);
    ASSERT_FALSE(content->isReady());
    ASSERT_TRUE(content->isWaiting());
    ASSERT_FALSE(content->isError());

    auto requested = content->getRequestedPackages();
    ASSERT_EQ(1, requested.size());
    auto it = requested.begin();
    ASSERT_STREQ("basic", it->reference().name().c_str());
    ASSERT_STREQ("1.2", it->reference().version().c_str());

    // The requested list is cleared
    ASSERT_EQ(0, content->getRequestedPackages().size());
    ASSERT_TRUE(content->isWaiting());

    content->addPackage(*it, BASIC_INCOMPATIBLE_PACKAGE);
    ASSERT_FALSE(content->isWaiting());

    ASSERT_EQ("1.1", content->getAPLVersion());

    auto m = Metrics().size(1024,800).theme("dark");
    auto doc = RootContext::create(m, content);

    ASSERT_FALSE(doc);
}

TEST(DocumentTest, NotEnforceSpecVersionCheck)
{
    auto content = Content::create(INCOMPATIBLE_MAIN, makeDefaultSession());

    ASSERT_TRUE(content);
    ASSERT_FALSE(content->isWaiting());
    ASSERT_FALSE(content->isError());
    ASSERT_TRUE(content->isReady());

    ASSERT_EQ("1.very_custom_version", content->getAPLVersion());

    auto m = Metrics().size(1024,800).theme("dark");
    auto config = RootConfig();
    config.enforceAPLVersion(APLVersion::kAPLVersionIgnore);
    auto doc = RootContext::create(m, content, config);

    ASSERT_TRUE(doc);
}

TEST(DocumentTest, EnforceSpecVersionCheck)
{
    auto content = Content::create(ONE_DEPENDENCY_VERSION, makeDefaultSession());

    ASSERT_TRUE(content);

    auto requested = content->getRequestedPackages();
    ASSERT_EQ(1, requested.size());
    auto it = requested.begin();
    ASSERT_STREQ("basic", it->reference().name().c_str());
    ASSERT_STREQ("1.2", it->reference().version().c_str());

    // The requested list is cleared
    ASSERT_EQ(0, content->getRequestedPackages().size());
    ASSERT_TRUE(content->isWaiting());

    content->addPackage(*it, BASIC_PACKAGE);
    ASSERT_FALSE(content->isWaiting());

    ASSERT_EQ("1.0", content->getAPLVersion());

    content->addData("payload", "\"duck\"");
    ASSERT_TRUE(content->isReady());

    auto m = Metrics().size(1024,800).theme("dark");
    auto config = RootConfig();
    config.enforceAPLVersion(APLVersion::kAPLVersion11);
    auto doc = RootContext::create(m, content, config);

    ASSERT_FALSE(doc);
}

TEST(DocumentTest, EnforceSpecVersionCheckMultipleVersions)
{
    auto content = Content::create(ONE_DEPENDENCY_VERSION, makeDefaultSession());

    ASSERT_TRUE(content);

    auto requested = content->getRequestedPackages();
    ASSERT_EQ(1, requested.size());
    auto it = requested.begin();
    ASSERT_STREQ("basic", it->reference().name().c_str());
    ASSERT_STREQ("1.2", it->reference().version().c_str());

    // The requested list is cleared
    ASSERT_EQ(0, content->getRequestedPackages().size());
    ASSERT_TRUE(content->isWaiting());

    content->addPackage(*it, BASIC_PACKAGE);
    ASSERT_FALSE(content->isWaiting());

    ASSERT_EQ("1.0", content->getAPLVersion());

    content->addData("payload", "\"duck\"");
    ASSERT_TRUE(content->isReady());

    auto m = Metrics().size(1024,800).theme("dark");
    auto doc = RootContext::create(m, content);

    ASSERT_TRUE(doc);
}

const char *SINGLE_WITH_RESOURCE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"import\": ["
    "    {"
    "      \"name\": \"basic\","
    "      \"version\": \"1.2\""
    "    }"
    "  ],"
    "  \"resources\": ["
    "    {"
    "      \"strings\": {"
    "        \"test\": \"A\""
    "      }"
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Text\""
    "    }"
    "  }"
    "}";

const char *BASIC_SINGLE_PKG =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"resources\": ["
    "    {"
    "      \"string\": {"
    "        \"item\": \"Here\","
    "        \"test\": \"B\""
    "      }"
    "    }"
    "  ]"
    "}";

TEST(DocumentTest, DependencyCheck)
{
    auto doc = Content::create(SINGLE_WITH_RESOURCE, makeDefaultSession());
    ASSERT_TRUE(doc);
    ASSERT_TRUE(doc->isWaiting());
    auto requested = doc->getRequestedPackages();
    ASSERT_EQ(1, requested.size());
    auto it = requested.begin();
    ASSERT_STREQ("basic", it->reference().name().c_str());
    ASSERT_STREQ("1.2", it->reference().version().c_str());

    // The requested list is cleared
    ASSERT_EQ(0, doc->getRequestedPackages().size());

    doc->addPackage(*it, BASIC_SINGLE_PKG);
    ASSERT_FALSE(doc->isWaiting());

    // Now check resources
    auto root = RootContext::create(Metrics(), doc);
    ASSERT_TRUE(root);
    ASSERT_EQ(2, root->info().resources().size());
    ASSERT_EQ(Object("Here"), root->context().opt("@item"));  // item does not get overridden
    ASSERT_EQ(Object("A"), root->context().opt("@test"));     // test gets overridden
}

const char *DIAMOND =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"import\": ["
    "    {"
    "      \"name\": \"A\","
    "      \"version\": \"2.2\""
    "    },"
    "    {"
    "      \"name\": \"B\","
    "      \"version\": \"1.0\""
    "    }"
    "  ],"
    "  \"resources\": ["
    "    {"
    "      \"strings\": {"
    "        \"test\": \"Hello\""
    "      }"
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Text\""
    "    }"
    "  }"
    "}";

const char *DIAMOND_A =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"import\": ["
    "    {"
    "      \"name\": \"C\","
    "      \"version\": \"1.5\""
    "    }"
    "  ],"
    "  \"resources\": ["
    "    {"
    "      \"strings\": {"
    "        \"test\": \"My A\","
    "        \"A\": \"This is A\","
    "        \"overwrite_A\": \"Original_A\","
    "        \"overwrite_C\": \"A\""
    "      }"
    "    }"
    "  ]"
    "}";

const char *DIAMOND_B =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"import\": ["
    "    {"
    "      \"name\": \"C\","
    "      \"version\": \"1.5\""
    "    }"
    "  ],"
    "  \"resources\": ["
    "    {"
    "      \"strings\": {"
    "        \"test\": \"My B\","
    "        \"B\": \"This is B\","
    "        \"overwrite_B\": \"Original_B\","
    "        \"overwrite_C\": \"B\""
    "      }"
    "    }"
    "  ]"
    "}";

const char *DIAMOND_C =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"resources\": ["
    "    {"
    "      \"strings\": {"
    "        \"C\": \"This is C\","
    "        \"test\": \"My C\","
    "        \"overwrite_A\": \"C's version of A\","
    "        \"overwrite_B\": \"C's version of B\","
    "        \"overwrite_C\": \"C's version of C\""
    "      }"
    "    }"
    "  ]"
    "}";

TEST(DocumentTest, MultipleDependencies)
{
    auto doc = Content::create(DIAMOND, makeDefaultSession());
    ASSERT_TRUE(doc);
    ASSERT_TRUE(doc->isWaiting());
    auto requested = doc->getRequestedPackages();
    ASSERT_EQ(2, requested.size());

    // The requested list is cleared
    ASSERT_EQ(0, doc->getRequestedPackages().size());

    for (auto it = requested.begin() ; it != requested.end() ; it++) {
        auto s = it->reference().name();
        if (s == "A")
            doc->addPackage(*it, DIAMOND_A);
        else if (s == "B")
            doc->addPackage(*it, DIAMOND_B);
        else
            FAIL() << "Unrecognized package " << s;
    }

    ASSERT_TRUE(doc->isWaiting());
    requested = doc->getRequestedPackages();
    ASSERT_EQ(1, requested.size());
    auto it = requested.begin();
    ASSERT_EQ(it->reference().name(), "C");

    doc->addPackage(*it, DIAMOND_C);
    ASSERT_FALSE(doc->isWaiting());
    ASSERT_TRUE(doc->isReady());

    // Now check resources
    auto m = Metrics().size(1024,800).theme("dark");
    auto root = RootContext::create(m, doc);
    ASSERT_TRUE(root);
    auto context = root->contextPtr();
    ASSERT_TRUE(context);
    ASSERT_EQ(7, root->info().resources().size());
    ASSERT_EQ(Object("This is A"), context->opt("@A"));
    ASSERT_EQ(Object("This is B"), context->opt("@B"));
    ASSERT_EQ(Object("This is C"), context->opt("@C"));
    ASSERT_EQ(Object("Original_A"), context->opt("@overwrite_A"));
    ASSERT_EQ(Object("Original_B"), context->opt("@overwrite_B"));
    ASSERT_EQ(Object("B"), context->opt("@overwrite_C"));
}

static const char *DUPLICATE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"import\": ["
    "    {"
    "      \"name\": \"A\","
    "      \"version\": \"2.2\""
    "    },"
    "    {"
    "      \"name\": \"A\","
    "      \"version\": \"1.2\""
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Text\""
    "    }"
    "  }"
    "}";

static const char *DUPLICATE_A_2_2 =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"import\": ["
    "    {"
    "      \"name\": \"A\","
    "      \"version\": \"1.2\""
    "    }"
    "  ],"
    "  \"resources\": ["
    "    {"
    "      \"strings\": {"
    "        \"A\": \"Not A\""
    "      }"
    "    }"
    "  ]"
    "}";

static const char *DUPLICATE_A_1_2 =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"resources\": ["
    "    {"
    "      \"strings\": {"
    "        \"A\": \"A\","
    "        \"B\": \"B\""
    "      }"
    "    }"
    "  ]"
    "}";

TEST(DocumentTest, Duplicate)
{
    auto doc = Content::create(DUPLICATE, makeDefaultSession());
    ASSERT_TRUE(doc);
    ASSERT_TRUE(doc->isWaiting());
    auto requested = doc->getRequestedPackages();
    ASSERT_EQ(2, requested.size());

    // The requested list is cleared
    ASSERT_EQ(0, doc->getRequestedPackages().size());

    for (auto it = requested.begin() ; it != requested.end() ; it++) {
        auto s = it->reference().version();
        if (s == "1.2")
            doc->addPackage(*it, DUPLICATE_A_1_2);
        else if (s == "2.2")
            doc->addPackage(*it, DUPLICATE_A_2_2);
        else
            FAIL() << "Unrecognized package " << s;
    }

    ASSERT_FALSE(doc->isWaiting());
    ASSERT_TRUE(doc->isReady());

    // Now check resources
    auto m = Metrics().size(1024,800).theme("dark");
    auto root = RootContext::create(m, doc);
    ASSERT_TRUE(root);
    auto context = root->contextPtr();
    ASSERT_TRUE(context);
    ASSERT_EQ(2, root->info().resources().size());
    ASSERT_EQ(Object("Not A"), context->opt("@A"));
    ASSERT_EQ(Object("B"), context->opt("@B"));
}

const char *FAKE_MAIN_TEMPLATE =
    "{"
    " \"item\": {"
    "   \"type\": \"Text\""
    " }"
    "}";

static std::string
makeTestPackage(std::vector<const char *> dependencies, std::map<const char *, const char *> stringMap)
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    doc.AddMember("type", "APL", allocator);
    doc.AddMember("version", "1.1", allocator);

    // Add imports
    rapidjson::Value imports(rapidjson::kArrayType);
    for (const char *it : dependencies) {
        rapidjson::Value importBlock(rapidjson::kObjectType);
        importBlock.AddMember("name", rapidjson::StringRef(it), allocator);
        importBlock.AddMember("version", "1.0", allocator);
        imports.PushBack(importBlock, allocator);
    }
    doc.AddMember("import", imports, allocator);

    // Add resources
    rapidjson::Value resources(rapidjson::kArrayType);
    rapidjson::Value resourceBlock(rapidjson::kObjectType);
    rapidjson::Value resourceStrings(rapidjson::kObjectType);
    for (auto it : stringMap)
        resourceStrings.AddMember(rapidjson::StringRef(it.first), rapidjson::StringRef(it.second), allocator);
    resourceBlock.AddMember("strings", resourceStrings, allocator);
    resources.PushBack(resourceBlock, allocator);
    doc.AddMember("resources", resources, allocator);

    // Add a mainTemplate section (just in case)
    rapidjson::Document mainTemplate;
    mainTemplate.Parse(FAKE_MAIN_TEMPLATE);
    doc.AddMember("mainTemplate", mainTemplate, allocator);

    // Convert to a string
    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return std::string(buffer.GetString());
}

TEST(DocumentTest, Generated)
{
    auto m = Metrics().size(1024,800).theme("dark");

    auto json = makeTestPackage({}, {{"test", "value"}});
    auto content = Content::create(json, makeDefaultSession());
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());

    auto doc = RootContext::create(m, content);
    ASSERT_TRUE(doc);
    auto context = doc->contextPtr();
    ASSERT_TRUE(context);

    ASSERT_EQ(1, doc->info().resources().size());
    ASSERT_EQ(Object("value"), context->opt("@test"));
}

TEST(DocumentTest, GenerateChain)
{
    auto m = Metrics().size(1024,800).theme("dark");

    auto json = makeTestPackage({"A"}, {{"test", "value"}});
    auto content = Content::create(json, makeDefaultSession());
    ASSERT_TRUE(content);
    ASSERT_FALSE(content->isReady());
    ASSERT_TRUE(content->isWaiting());

    auto requested = content->getRequestedPackages();
    ASSERT_EQ(1, requested.size());
    auto it = requested.begin();
    auto pkg_a = makeTestPackage({"B"}, {{"testA", "A"}});
    content->addPackage(*it, pkg_a.c_str());

    ASSERT_TRUE(content->isWaiting());
    requested = content->getRequestedPackages();
    ASSERT_EQ(1, requested.size());
    it = requested.begin();
    ASSERT_EQ(it->reference().name(), "B");
    auto pkg_b = makeTestPackage({}, {{"testB", "B"}});
    content->addPackage(*it, pkg_b.c_str());

    ASSERT_FALSE(content->isWaiting());
    ASSERT_TRUE(content->isReady());

    auto doc = RootContext::create(m, content);
    ASSERT_TRUE(doc);
    auto context = doc->contextPtr();
    ASSERT_TRUE(context);

    ASSERT_EQ(Object("value"), context->opt("@test"));
    ASSERT_EQ(Object("A"), context->opt("@testA"));
    ASSERT_EQ(Object("B"), context->opt("@testB"));
}

TEST(DocumentTest, Loop)
{
    auto m = Metrics().size(1024,800).theme("dark");

    auto json = makeTestPackage({"A", "B"}, {{"test", "value"}});
    auto pkg_a = makeTestPackage({"B"}, {{"testA", "A"}});
    auto pkg_b = makeTestPackage({"A"}, {{"testB", "B"}});

    auto content = Content::create(json, makeDefaultSession());
    ASSERT_TRUE(content);
    ASSERT_FALSE(content->isReady());
    ASSERT_TRUE(content->isWaiting());
    for (auto it : content->getRequestedPackages()) {
        if (it.reference().name() == "A") content->addPackage(it, pkg_a);
        else if (it.reference().name() == "B") content->addPackage(it, pkg_b);
        else FAIL() << "Unknown package " << it.reference().name();
    }

    ASSERT_TRUE(content->isError());

}

TEST(DocumentTest, NonReversal)
{
    auto m = Metrics().size(1024,800).theme("dark");

    auto json = makeTestPackage({"A", "B"}, {{"test", "value"}});
    auto pkg_a = makeTestPackage({}, {{"testA", "A"}, {"testB", "A"}});
    auto pkg_b = makeTestPackage({"A"}, {{"testB", "B"}});

    auto content = Content::create(json, makeDefaultSession());
    ASSERT_TRUE(content);

    ASSERT_TRUE(content->isWaiting());
    for (auto it : content->getRequestedPackages()) {
        if (it.reference().name() == "A") content->addPackage(it, pkg_a);
        else if (it.reference().name() == "B") content->addPackage(it, pkg_b);
        else FAIL() << "Unknown package " << it.reference().name();
    }
    ASSERT_TRUE(content->isReady());

    auto doc = RootContext::create(m, content);
    ASSERT_TRUE(doc);
    auto context = doc->contextPtr();
    ASSERT_TRUE(context);

    ASSERT_EQ(3, doc->info().resources().size());
    ASSERT_EQ(Object("value"), context->opt("@test"));
    ASSERT_EQ(Object("A"), context->opt("@testA"));
    ASSERT_EQ(Object("B"), context->opt("@testB"));  // B depends on A, so B overrides A
}

TEST(DocumentTest, Reversal)
{
    auto m = Metrics().size(1024,800).theme("dark");

    auto json = makeTestPackage({"A", "B"}, {{"test", "value"}});
    auto pkg_a = makeTestPackage({"B"}, {{"testA", "A"}, {"testB", "A"}});
    auto pkg_b = makeTestPackage({}, {{"testB", "B"}});

    auto content = Content::create(json, makeDefaultSession());
    ASSERT_TRUE(content);

    ASSERT_TRUE(content->isWaiting());
    for (auto it : content->getRequestedPackages()) {
        if (it.reference().name() == "A") content->addPackage(it, pkg_a);
        else if (it.reference().name() == "B") content->addPackage(it, pkg_b);
        else FAIL() << "Unknown package " << it.reference().name();
    }
    ASSERT_TRUE(content->isReady());

    auto doc = RootContext::create(m, content);
    ASSERT_TRUE(doc);
    auto context = doc->contextPtr();
    ASSERT_TRUE(context);

    ASSERT_EQ(3, doc->info().resources().size());
    ASSERT_EQ(Object("value"), context->opt("@test"));
    ASSERT_EQ(Object("A"), context->opt("@testA"));
    ASSERT_EQ(Object("A"), context->opt("@testB"));  // A depends on B, so A overrides B
}

TEST(DocumentTest, DeepReversal)
{
    auto m = Metrics().size(1024,800).theme("dark");

    std::map<std::string, std::string> packageMap = {
        { "A", makeTestPackage({"C"}, {{"foo", "A"}}) },
        { "B", makeTestPackage({}, {{"foo", "B"}}) },
        { "C", makeTestPackage({"B"}, {{"foo", "C"}}) }
    };

    auto json = makeTestPackage({"A", "B"}, {{"test", "value"}});
    auto content = Content::create(json, makeDefaultSession());
    ASSERT_TRUE(content);

    while (content->isWaiting()) {
        for (auto it : content->getRequestedPackages()) {
            auto pkg = packageMap.find(it.reference().name());
            if (pkg == packageMap.end())
                FAIL() << "Unknown package " << it.reference().name();
            content->addPackage(it, pkg->second.c_str());
        }
    }

    ASSERT_TRUE(content->isReady());

    auto doc = RootContext::create(m, content);
    ASSERT_TRUE(doc);
    auto context = doc->contextPtr();
    ASSERT_TRUE(context);

    ASSERT_EQ(Object("A"), context->opt("@foo"));  // Package A -> C -> B
}

TEST(DocumentTest, DeepLoop)
{
    auto m = Metrics().size(1024,800).theme("dark");

    std::map<std::string, std::string> packageMap = {
        { "A", makeTestPackage({"B", "C"}, {}) },
        { "B", makeTestPackage({"C", "D"}, {}) },
        { "C", makeTestPackage({"D"}, {}) },
        { "D", makeTestPackage({"A"}, {}) }
    };

    auto json = makeTestPackage({"A"}, {{"test", "value"}});
    auto content = Content::create(json, makeDefaultSession());
    ASSERT_TRUE(content);

    while (content->isWaiting()) {
        for (auto it : content->getRequestedPackages()) {
            auto pkg = packageMap.find(it.reference().name());
            if (pkg == packageMap.end())
                FAIL() << "Unknown package " << it.reference().name();
            content->addPackage(it, pkg->second.c_str());
        }
    }

    ASSERT_TRUE(content->isError());
}

static const char * PAYLOAD_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"onMount\": {"
    "    \"type\": \"SetValue\","
    "    \"componentId\": \"TestId\","
    "    \"property\": \"text\","
    "    \"value\": \"${payload.value}\""
    "  },"
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"Not set\","
    "      \"id\": \"TestId\""
    "    }"
    "  }"
    "}";

/**
 * Verify that the onMount command has access to the document payload
 */
TEST(DocumentTest, PayloadTest)
{
    auto content = Content::create(PAYLOAD_TEST, makeDefaultSession());

    ASSERT_TRUE(content);
    ASSERT_FALSE(content->isReady());
    ASSERT_FALSE(content->isWaiting());
    ASSERT_FALSE(content->isError());

    ASSERT_EQ(1, content->getParameterCount());
    ASSERT_EQ(std::string("payload"), content->getParameterAt(0));
    content->addData("payload", R"({"value": "Is Set"})");
    ASSERT_TRUE(content->isReady());

    auto doc = RootContext::create(Metrics(), content, RootConfig());

    ASSERT_TRUE(doc);
    ASSERT_STREQ("Is Set", doc->topComponent()->getCalculated(kPropertyText).asString().c_str());
}


static const char * EXTERNAL_COMMAND_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"id\": \"TextId\","
    "      \"text\": \"${payload.start}\""
    "    }"
    "  }"
    "}";

static const char * EXTERNAL_COMMAND_TEST_COMMAND =
    "["
    "  {"
    "    \"type\": \"SetValue\","
    "    \"componentId\": \"TextId\","
    "    \"property\": \"text\","
    "    \"value\": \"${payload.end}\""
    "  }"
    "]";

/**
 * Verify that an external command has access to the document payload
 */
TEST(DocumentTest, ExternalCommandTest)
{
    auto content = Content::create(EXTERNAL_COMMAND_TEST, makeDefaultSession());

    ASSERT_TRUE(content);
    ASSERT_FALSE(content->isReady());
    ASSERT_FALSE(content->isWaiting());
    ASSERT_FALSE(content->isError());

    ASSERT_EQ(1, content->getParameterCount());
    ASSERT_EQ(std::string("payload"), content->getParameterAt(0));
    content->addData("payload", R"({"start": "Is Not Set", "end": "Is Set"})");
    ASSERT_TRUE(content->isReady());

    auto doc = RootContext::create(Metrics(), content, RootConfig());

    ASSERT_TRUE(doc);
    ASSERT_STREQ("Is Not Set", doc->topComponent()->getCalculated(kPropertyText).asString().c_str());

    auto cmd = JsonData(EXTERNAL_COMMAND_TEST_COMMAND);
    ASSERT_TRUE(cmd);

    doc->executeCommands(cmd.get(), false);
    ASSERT_STREQ("Is Set", doc->topComponent()->getCalculated(kPropertyText).asString().c_str());
}

