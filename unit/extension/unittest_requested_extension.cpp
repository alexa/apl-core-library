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

class RequestedExtensionTest : public DocumentWrapper {
protected:
    void postCreateContent() override {
        for (auto& m : content->getExtensionRequests()) {
            // Skip requests that start with an underscore
            if (m[0] != '_')
                config->registerExtension(m);
        }
    }
};

static const char* BASIC = R"({
  "type": "APL",
  "version": "1.2",
  "extension": {
    "uri": "URI1",
    "name": "foo"
  },
  "mainTemplate": {
    "item": {
      "type": "Text"
    }
  }
})";

/**
 * Request a single feature by raw string. The raw string should be arrayified and then
 * treated as a simple URI.
 */
TEST_F(RequestedExtensionTest, Basic) {
    loadDocument(BASIC);

    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.foo}")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension.XXX}")));
}

static const char* FANCY = R"({
  "type": "APL",
  "version": "1.2",
  "extensions": [
    "URI1",
    {
      "uri": "URI2"
    },
    {
      "uri": "URI3",
      "name": "foo"
    },
    {
      "uri": "_URI4",
      "name": "foo2"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Text"
    }
  }
})";

/**
 * Request a set of extensions, but providing a variety of correct and incorrect representations.
 */
TEST_F(RequestedExtensionTest, Fancy) {
    loadDocument(FANCY);

    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension.URI1}")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension.URI2}")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension.URI3}")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension._URI4}")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension.XXX}")));
    // verify the extension environment by name
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.foo}")));
    ASSERT_TRUE(IsEqual(Object::FALSE_OBJECT(), evaluate(*context, "${environment.extension.foo2}")));

    ASSERT_TRUE(ConsoleMessage());
}

static const char* DOC_WITH_IMPORT = R"({
  "type": "APL",
  "version": "1.2",
  "extension": {
    "uri": "URI1",
    "name": "foo"
  },
  "import": [
    {
      "name": "ThingLibrary",
      "version": "1.2"
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Text"
    }
  }
})";

static const char* SIMPLE_PACKAGE = R"({
  "type": "APL",
  "version": "1.2",
  "extension": {
    "uri": "URI2",
    "name": "foo2"
  }
})";

/**
 * An imported package requests a feature.  That feature should show up in the environment list.
 */
TEST_F(RequestedExtensionTest, Import) {
    loadDocumentWithPackage(DOC_WITH_IMPORT, SIMPLE_PACKAGE);

    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.foo}")));
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.foo2}")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension.URI3}")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension.foo3}")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension.XXX}")));
}

static const char* DUPLICATE_NAME = R"({
  "type": "APL",
  "version": "1.3",
  "extensions": [
    {
      "uri": "URI1",
      "name": "myname"
    },
    {
      "uri": "URI2",
      "name": "myname"
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Text"
    }
  }
})";

/**
 * Requesting different extensions with the same name should trigger invalid content
 */
TEST_F(RequestedExtensionTest, DuplicateAlias) {
    loadDocumentBadContent(DUPLICATE_NAME);
    ASSERT_TRUE(ConsoleMessage());
}

static const char* REPEATED_NAME = R"({
  "type": "APL",
  "version": "1.3",
  "extensions": [
    {
      "uri": "_URI1",
      "name": "myname"
    },
    {
      "uri": "_URI1",
      "name": "myname2"
    },
    {
      "uri": "_URI1",
      "name": "myname2"
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Text"
    }
  }
})";

/**
 * The same URI can have multiple names.  The same name can be re-used as long as it
 * points to the same URI.
 */
TEST_F(RequestedExtensionTest, RepeatedAlias) {
    config->registerExtension("_URI1");
    loadDocument(REPEATED_NAME);

    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.myname}")));
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.myname2}")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension.XXX}")));
}

static const char* MISSING_URI = R"({
  "type": "APL",
  "version": "1.3",
  "extensions": [
    {
      "name": "myname"
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Text"
    }
  }
})";

/**
 * The URI must be present or a session warning will be logged.
 */
TEST_F(RequestedExtensionTest, MissingURI) {
    loadDocument(MISSING_URI);

    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension.URI1}")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension.myname}")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${environment.extension.XXX}")));

    ASSERT_TRUE(ConsoleMessage());
}


/**
 * Register and extension without configurable settings.
 */
TEST_F(RequestedExtensionTest, ExtensionWithDefaultConfig) {

    loadDocument(BASIC);

    auto c = config->getExtensionEnvironment("URI1");
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), c));

    // verify the environment evaluates to true for the extension name
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.foo}")));
}

/**
 * Configuration is defaulted to Object::TRUE_OBJECT() when adding command, handler, config, to unregistered extension.
 */
TEST_F(RequestedExtensionTest, IndirectDefaultConfig) {

    // unregistered extension registers command
    auto cmd = ExtensionCommandDefinition("ext:Cmd", "cmd");
    config->registerExtensionCommand(cmd);
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), config->getExtensionEnvironment("ext:Cmd")));

    // unregistered extension registers handler
    auto handler = ExtensionEventHandler("ext:Hdlr", "hdlr");
    config->registerExtensionEventHandler(handler);
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), config->getExtensionEnvironment("ext:Hdlr")));

    config->registerExtensionEnvironment("ext:Cfg", Object(64));
    ASSERT_TRUE(IsEqual(64, config->getExtensionEnvironment("ext:Cfg")));
}

/**
 * Configuration value is the same no matter call order of api's that trigger extention regisistration
 */
TEST_F(RequestedExtensionTest, ConfigAPIOrder) {

    auto six4 = Object(64);

    // order: register, handler, command
    auto cmd1 = ExtensionCommandDefinition("ext:1", "cmd");
    auto handler1 = ExtensionEventHandler("ext:1", "hdlr");
    config->registerExtension("ext:1", six4).registerExtensionEventHandler(handler1).registerExtensionCommand(cmd1);
    ASSERT_TRUE(IsEqual(64, config->getExtensionEnvironment("ext:1")));

    // order: command, register, handler
    auto cmd2 = ExtensionCommandDefinition("ext:2", "cmd");
    auto handler2 = ExtensionEventHandler("ext:2", "hdlr");
    config->registerExtensionCommand(cmd2).registerExtension("ext:2", six4).registerExtensionEventHandler(handler2);
    ASSERT_TRUE(IsEqual(64, config->getExtensionEnvironment("ext:2")));

    // order: command, handler, register
    auto cmd3 = ExtensionCommandDefinition("ext:3", "cmd");
    auto handler3 = ExtensionEventHandler("ext:3", "hdlr");
    config->registerExtensionCommand(cmd3).registerExtensionEventHandler(handler3).registerExtension("ext:3", six4);
    ASSERT_TRUE(IsEqual(64, config->getExtensionEnvironment("ext:3")));

    // order: command, handler, config
    auto cmd4 = ExtensionCommandDefinition("ext:4", "cmd");
    auto handler4 = ExtensionEventHandler("ext:4", "hdlr");
    config->registerExtensionCommand(cmd4).registerExtensionEventHandler(handler4).registerExtensionEnvironment("ext:4",
                                                                                                               six4);
    ASSERT_TRUE(IsEqual(64, config->getExtensionEnvironment("ext:4")));

    // order: handler, config, command
    auto cmd5 = ExtensionCommandDefinition("ext:5", "cmd");
    auto handler5 = ExtensionEventHandler("ext:5", "hdlr");
    config->registerExtensionEventHandler(handler5).registerExtensionEnvironment("ext:5", six4).registerExtensionCommand(cmd5);
    ASSERT_TRUE(IsEqual(64, config->getExtensionEnvironment("ext:5")));

    // order: config, handler, command
    auto cmd6 = ExtensionCommandDefinition("ext:6", "cmd");
    auto handler6 = ExtensionEventHandler("ext:6", "hdlr");
    config->registerExtensionEnvironment("ext:6", six4).registerExtensionEventHandler(handler6).registerExtensionCommand(cmd6);
    ASSERT_TRUE(IsEqual(64, config->getExtensionEnvironment("ext:6")));
}

/**
 * Configuration overwrite. The registerExtensionConfiguration(..), and registerExtension(..) both take config,
 * last one in wins.
 */
TEST_F(RequestedExtensionTest, ConfigOverwrite) {

    config->registerExtension("ext:1", Object(64)).registerExtensionEnvironment("ext:1", Object(53));
    ASSERT_TRUE(IsEqual(53, config->getExtensionEnvironment("ext:1")));

    config->registerExtensionEnvironment("ext:2", Object(53)).registerExtension("ext:2", Object(64));
    ASSERT_TRUE(IsEqual(64, config->getExtensionEnvironment("ext:2")));

    config->registerExtensionEnvironment("ext:3", Object(53)).registerExtension("ext:3");
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), config->getExtensionEnvironment("ext:3")));

}

static const char* WITH_CONFIG = R"({
  "type": "APL",
  "version": "1.2",
  "extensions": [
    {
      "uri": "_URIXbool",
      "name": "Xbool"
    },
    {
      "uri": "_URIXstring",
      "name": "Xstring"
    },
    {
      "uri": "_URIXnumber",
      "name": "Xnumber"
    },
    {
      "uri": "_URIXcolor",
      "name": "Xcolor"
    },
    {
      "uri": "_URIXmap",
      "name": "Xmap"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Text"
    }
  }
})";

/**
 * Register and extension with simple Object configuration.
 */
TEST_F(RequestedExtensionTest, ExtensionWithSimpleConfig) {
    config->registerExtension("_URIXbool", Object::TRUE_OBJECT());
    config->registerExtension("_URIXstring", Object("dog"));
    config->registerExtension("_URIXnumber", Object(64));
    config->registerExtension("_URIXcolor", Object(Color(Color::BLUE)));

    loadDocument(WITH_CONFIG);

    // verify config and environment for boolean
    auto b = config->getExtensionEnvironment("_URIXbool");
    ASSERT_TRUE(b.isBoolean());
    ASSERT_TRUE(b.getBoolean());
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.Xbool}")));

    // verify config and environment for string
    auto d = config->getExtensionEnvironment("_URIXstring");
    ASSERT_TRUE(d.isString());
    ASSERT_EQ("dog", d.getString());
    ASSERT_TRUE(IsEqual("dog", evaluate(*context, "${environment.extension.Xstring}")));

    // verify config and environment for number
    auto n = config->getExtensionEnvironment("_URIXnumber");
    ASSERT_TRUE(n.isNumber());
    ASSERT_EQ(64, n.getInteger());
    ASSERT_TRUE(IsEqual(64, evaluate(*context, "${environment.extension.Xnumber}")));

    // verify config and environment for boolean
    auto c = config->getExtensionEnvironment("_URIXcolor");
    ASSERT_TRUE(c.isColor());
    ASSERT_EQ(Color::BLUE, c.getColor());
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), evaluate(*context, "${environment.extension.Xcolor}")));
}

/**
 * Register an extension with configuration values.
 */
TEST_F(RequestedExtensionTest, ExtensionWithConfigMap) {
    auto cfgMap = std::make_shared<ObjectMap>();
    cfgMap->emplace("cfg1", "dog");
    cfgMap->emplace("cfg2", 64);
    cfgMap->emplace("cfg3", true);
    config->registerExtension("_URIXmap", cfgMap);

    loadDocument(WITH_CONFIG);

    // Register the extension with configuration
    auto c = config->getExtensionEnvironment("_URIXmap");
    ASSERT_TRUE(c.isMap());
    auto map = c.getMap();
    ASSERT_EQ(3, map.size());
    ASSERT_TRUE(IsEqual("dog", map.at("cfg1")));
    ASSERT_TRUE(IsEqual(64, map.at("cfg2")));
    ASSERT_TRUE(IsEqual(true, map.at("cfg3")));

    // verify the environment has configuration values for the extension name
    ASSERT_TRUE(IsEqual("dog", evaluate(*context, "${environment.extension.Xmap.cfg1}")));
    ASSERT_TRUE(IsEqual(64, evaluate(*context, "${environment.extension.Xmap.cfg2}")));
    ASSERT_TRUE(IsEqual(true, evaluate(*context, "${environment.extension.Xmap.cfg3}")));
}


/**
 * Register an extension with multiple names and configuration values.
 */
TEST_F(RequestedExtensionTest, ExtensionWithSimpleConfigMultiName) {

    auto dog = Object("dog");
    config->registerExtension("_URI1", dog); //overwrites test framework simple config

    loadDocument(REPEATED_NAME);

    // Register the extension with configuration
    auto c = config->getExtensionEnvironment("_URI1");
    ASSERT_TRUE(c.isString());
    auto str = c.getString();
    ASSERT_EQ("dog", str);

    ASSERT_TRUE(IsEqual("dog", evaluate(*context, "${environment.extension.myname}")));
    ASSERT_TRUE(IsEqual("dog", evaluate(*context, "${environment.extension.myname2}")));
}

/**
 * Register an extension with multiple names and configuration values.
 */
TEST_F(RequestedExtensionTest, ExtensionWithConfigMapMultiName) {
    auto cfgMap = std::make_shared<ObjectMap>();
    cfgMap->emplace("cfg1", "dog");
    cfgMap->emplace("cfg2", 64);
    cfgMap->emplace("cfg3", true);
    config->registerExtension("_URI1", cfgMap); //overwrites test framework empty config

    loadDocument(REPEATED_NAME);

    // Register the extension with configuration
    auto c = config->getExtensionEnvironment("_URI1");
    ASSERT_TRUE(c.isMap());
    auto map = c.getMap();
    ASSERT_EQ(3, map.size());
    ASSERT_TRUE(IsEqual("dog", map.at("cfg1")));
    ASSERT_TRUE(IsEqual(64, map.at("cfg2")));
    ASSERT_TRUE(IsEqual(true, map.at("cfg3")));

    // verify the environment has configuration values for the extension name
    ASSERT_TRUE(evaluate(*context, "${environment.extension.myname}").isMap());
    ASSERT_TRUE(IsEqual("dog", evaluate(*context, "${environment.extension.myname.cfg1}")));
    ASSERT_TRUE(IsEqual(64, evaluate(*context, "${environment.extension.myname.cfg2}")));
    ASSERT_TRUE(IsEqual(true, evaluate(*context, "${environment.extension.myname.cfg3}")));

    // verify the environment has configuration values for the extension second name
    ASSERT_TRUE(evaluate(*context, "${environment.extension.myname2}").isMap());
    ASSERT_TRUE(IsEqual("dog", evaluate(*context, "${environment.extension.myname2.cfg1}")));
    ASSERT_TRUE(IsEqual(64, evaluate(*context, "${environment.extension.myname2.cfg2}")));
    ASSERT_TRUE(IsEqual(true, evaluate(*context, "${environment.extension.myname2.cfg3}")));
}


static const char* SETTINGS = R"({
  "type": "APL",
  "version": "1.2",
  "extension": {
    "uri": "URI1",
    "name": "foo"
  },
  "settings": {
    "foo": {
      "keyA": "valueA",
      "keyB": "valueB"
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Text"
    }
  }
})";


/**
 * Document does not provide extension settings
 */
TEST_F(RequestedExtensionTest, DocWithoutSettings) {
    loadDocument(BASIC);

    // verify extensions available
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.foo}")));

    // verify no settings on the extensions
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), content->getExtensionSettings("URI1")));
}

/**
 * Document provides extension settings
 */
TEST_F(RequestedExtensionTest, DocWithSettings) {

    loadDocument(SETTINGS);

    // verify extensions available
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.foo}")));

    // verify settings on the named extension
    auto es = content->getExtensionSettings("URI1");
    ASSERT_FALSE(IsEqual(Object::NULL_OBJECT(), es));

    ASSERT_TRUE(IsEqual("valueA", es.get("keyA")));
    ASSERT_TRUE(IsEqual("valueB", es.get("keyB")));
}

static const char* SETTINGS_REPEAT_URI = R"({
  "type": "APL",
  "version": "1.2",
  "extensions": [
    {
      "uri": "URI1",
      "name": "foo"
    },
    {
      "uri": "URI1",
      "name": "bar"
    }
  ],
  "settings": {
    "foo": {
      "keyA": "valueA",
      "keyB": "valueB"
    },
    "bar": {
      "keyC": "valueC",
      "keyD": "valueD"
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Text"
    }
  }
})";

/**
 * Document provides extension settings for same extension registered under multiple names, with
 * different settings keys.  The settings should be merged.
 */
TEST_F(RequestedExtensionTest, DocWithMultiSettings) {

    loadDocument(SETTINGS_REPEAT_URI);

    // verify extensions available
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.foo}")));
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.bar}")));

    // verify settings on the first named extension
    auto es = content->getExtensionSettings("URI1");
    ASSERT_FALSE(IsEqual(Object::NULL_OBJECT(), es));
    ASSERT_EQ(4, es.size());

    // settings from name "foo"
    ASSERT_TRUE(IsEqual("valueA", es.get("keyA")));
    ASSERT_TRUE(IsEqual("valueB", es.get("keyB")));

    // settings from name "bar"
    ASSERT_TRUE(IsEqual("valueC", es.get("keyC")));
    ASSERT_TRUE(IsEqual("valueD", es.get("keyD")));
}


static const char* SETTINGS_REPEAT_URI_OVERRIDE = R"({
  "type": "APL",
  "version": "1.2",
  "extensions": [
    {
      "uri": "URI1",
      "name": "foo"
    },
    {
      "uri": "URI1",
      "name": "bar"
    }
  ],
  "settings": {
    "foo": {
      "keyA": "valueA",
      "keyB": "valueB"
    },
    "bar": {
      "keyA": "valueC",
      "keyB": "valueD"
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Text"
    }
  }
})";

/**
 * Document provides extension settings for same extension registered under multiple names with
 * same settings keys, the settings should be overwritten.
 */
TEST_F(RequestedExtensionTest, DocWithSettingsOverride) {
    loadDocument(SETTINGS_REPEAT_URI_OVERRIDE);

    // verify extensions available
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.foo}")));
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.bar}")));

    // verify settings on the first named extension
    auto es = content->getExtensionSettings("URI1");
    ASSERT_FALSE(IsEqual(Object::NULL_OBJECT(), es));
    ASSERT_EQ(2, es.size());

    // settings from name "bar" overwrite "foo" because it is second in list
    ASSERT_TRUE(IsEqual("valueC", es.get("keyA")));
    ASSERT_TRUE(IsEqual("valueD", es.get("keyB")));
}

static const char* SETTINGS_WITH_PACKAGE = R"({
  "type": "APL",
  "version": "1.2",
  "import": [
    {
      "name": "pkg1",
      "version": "1.2"
    }
  ],
  "extension": {
    "uri": "URI1",
    "name": "foo"
  },
  "settings": {
    "foo": {
      "keyA": "main-A",
      "keyB": "main-B"
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Text"
    }
  }
})";

const char* SETTINGS_PKG1 = R"({
  "type": "APL",
  "version": "1.1",
  "import": [
    {
      "name": "pkg2",
      "version": "1.2"
    }
  ],
  "settings": {
    "foo": {
      "keyA": "package1-A",
      "keyB": "package1-B",
      "keyC": "package1-C",
      "keyD": "package1-D"
    }
  }
})";

const char* SETTINGS_PKG2 = R"({
  "type": "APL",
  "version": "1.1",
  "settings": {
    "foo": {
      "keyD": "package2-D",
      "keyE": "package2-E"
    }
  }
})";

/**
 * Extension settings cannot be accessed before document is ready.
 */
TEST_F(RequestedExtensionTest, SettingsNotReady) {

    content = Content::create(SETTINGS_WITH_PACKAGE, session);

    ASSERT_FALSE(content->isReady());

    // verify settings on the named extension
    auto es = content->getExtensionSettings("URI1");
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), es));
    ASSERT_TRUE(ConsoleMessage());
}

/**
 * Document provides extension settings that override import settings.
 */
TEST_F(RequestedExtensionTest, SettingsWithMultiPackage) {

    loadDocumentWithMultiPackage(SETTINGS_WITH_PACKAGE, SETTINGS_PKG1, SETTINGS_PKG2);

    // verify extensions available
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.foo}")));

    // verify settings on the named extension
    auto es = content->getExtensionSettings("URI1");
    ASSERT_FALSE(IsEqual(Object::NULL_OBJECT(), es));

    // verify package settings override main doc
    ASSERT_TRUE(IsEqual("main-A", es.get("keyA")));
    ASSERT_TRUE(IsEqual("main-B", es.get("keyB")));
    ASSERT_TRUE(IsEqual("package1-C", es.get("keyC")));
    ASSERT_TRUE(IsEqual("package1-D", es.get("keyD")));
    ASSERT_TRUE(IsEqual("package2-E", es.get("keyE")));
}

