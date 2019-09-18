/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "testeventloop.h"

using namespace apl;

class MountTest : public DocumentWrapper {};

static const char *TRIVIAL =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Frame\","
        "      \"id\": \"frame\","
        "      \"backgroundColor\": \"blue\","
        "      \"onMount\":"
        "      ["
        "        {"
        "          \"type\": \"SetValue\","
        "          \"property\": \"backgroundColor\","
        "          \"value\": \"red\""
        "        },"
        "        {"
        "          \"type\": \"SendEvent\","
        "          \"arguments\": [ "
        "            \"${event.source.source}\","
        "            \"${event.source.handler}\","
        "            \"${event.source.id}\","
        "            \"${event.source.uid}\","
        "            \"${event.source.value}\""
        "          ]"
        "        }"
        "      ]"
        "    }"
        "  }"
        "}";

TEST_F(MountTest, Trivial)
{
    loadDocument(TRIVIAL);
    ASSERT_TRUE(component);

    // The background color change was immediate
    ASSERT_EQ(Object(Color(Color::RED)), component->getCalculated(kPropertyBackgroundColor));

    // No dirty properties should be set
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto args = event.getValue(kEventPropertyArguments);
    ASSERT_TRUE(args.isArray());
    ASSERT_EQ(5, args.size());
    ASSERT_EQ("Frame", args.at(0).asString());
    ASSERT_EQ("Mount", args.at(1).asString());
    ASSERT_EQ("frame", args.at(2).asString());
    ASSERT_TRUE(args.at(3).isString());
    ASSERT_TRUE(args.at(4).isNull());
}

static const char *ANIMATION =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"backgroundColor\": \"blue\","
    "      \"onMount\": ["
    "        {"
    "          \"type\": \"SetValue\","
    "          \"property\": \"backgroundColor\","
    "          \"value\": \"red\""
    "        },"
    "        {"
    "          \"type\": \"AnimateItem\","
    "          \"duration\": 1000,"
    "          \"value\": ["
    "            {"
    "              \"property\": \"opacity\","
    "              \"from\": 0,"
    "              \"to\": 1"
    "            }"
    "          ]"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";


TEST_F(MountTest, Animation)
{
    loadDocument(ANIMATION);

    ASSERT_EQ(Object(Color(Color::RED)), component->getCalculated(kPropertyBackgroundColor));
    ASSERT_EQ(0, component->getCalculated(kPropertyOpacity).asNumber());

    // No dirty properties should be set
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));

    auto startTime = root->currentTime();
    auto endTime = startTime + 1000;
    while (root->currentTime() < endTime) {
        root->updateTime(root->currentTime() + 100);
        ASSERT_TRUE(CheckDirty(component, kPropertyOpacity));
        ASSERT_TRUE(CheckDirty(root, component));
        ASSERT_NEAR((root->currentTime() - startTime)/1000.0,
                    component->getCalculated(kPropertyOpacity).asNumber(),
                    0.0001);
    }
}

static const char *MULTIPLE_ITEMS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Text\","
    "          \"text\": \"A\","
    "          \"id\": \"thing1\","
    "          \"color\": \"blue\","
    "          \"onMount\": {"
    "            \"type\": \"SetValue\","
    "            \"property\": \"color\","
    "            \"value\": \"red\","
    "            \"delay\": 500"
    "          }"
    "        },"
    "        {"
    "          \"type\": \"Text\","
    "          \"text\": \"B\","
    "          \"id\": \"thing2\","
    "          \"onMount\": {"
    "            \"type\": \"AnimateItem\","
    "            \"duration\": \"1000\","
    "            \"value\": ["
    "              {"
    "                \"property\": \"transform\","
    "                \"from\": {"
    "                  \"translateX\": 100"
    "                },"
    "                \"to\": {"
    "                  \"translateX\": 0"
    "                }"
    "              }"
    "            ]"
    "          }"
    "        }"
    "      ],"
    "      \"onMount\": ["
    "        {"
    "          \"type\": \"AnimateItem\","
    "          \"duration\": 1000,"
    "          \"value\": ["
    "            {"
    "              \"property\": \"opacity\","
    "              \"from\": 0,"
    "              \"to\": 1"
    "            }"
    "          ]"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";



TEST_F(MountTest, AnimateMultiple)
{
    loadDocument(MULTIPLE_ITEMS);

    auto thing1 = root->context().findComponentById("thing1");
    auto thing2 = root->context().findComponentById("thing2");

    ASSERT_EQ(Object(Color(Color::BLUE)), thing1->getCalculated(kPropertyColor));
    ASSERT_EQ(Object(Transform2D::translateX(100)), thing2->getCalculated(kPropertyTransform));
    ASSERT_EQ(Object(0), component->getCalculated(kPropertyOpacity));

    // No dirty properties should be set
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));

    auto startTime = root->currentTime();
    auto endTime = startTime + 1000;
    while (root->currentTime() < endTime) {
        root->updateTime(root->currentTime() + 100);
        auto delta = (root->currentTime() - startTime) / 1000.0;

        ASSERT_TRUE(CheckDirty(component, kPropertyOpacity));
        ASSERT_TRUE(CheckDirty(thing2, kPropertyTransform));
        if (delta >= 0.5 && delta < 0.55) {
            ASSERT_TRUE(CheckDirty(thing1, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke));
            ASSERT_TRUE(CheckDirty(root, component, thing1, thing2));
        }
        else {
            ASSERT_TRUE(CheckDirty(thing1));
            ASSERT_TRUE(CheckDirty(root, component, thing2));
        }

        ASSERT_EQ(Object(Color(delta >= 0.5 ? Color::RED : Color::BLUE)),
                  thing1->getCalculated(kPropertyColor));

        ASSERT_TRUE(IsEqual(Transform2D::translateX(100 * (1.0 - delta)),
                          thing2->getCalculated(kPropertyTransform).getTransform2D()));
        ASSERT_NEAR(delta,component->getCalculated(kPropertyOpacity).asNumber(),0.0001);
    }
}

static const char * DOCUMENT_ON_MOUNT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"id\": \"myText\""
    "    }"
    "  },"
    "  \"onMount\": "
    "  ["
    "    {"
    "      \"type\": \"SetValue\","
    "      \"componentId\": \"myText\","
    "      \"property\": \"text\","
    "      \"value\": \"Ha!\""
    "    },"
    "    {"
    "      \"type\": \"SendEvent\","
    "      \"arguments\": [ "
    "        \"${event.source.source}\","
    "        \"${event.source.handler}\","
    "        \"${event.source.id}\","
    "        \"${event.source.uid}\","
    "        \"${event.source.value}\""
    "      ]"
    "    }"
    "  ]"
    "}";

TEST_F(MountTest, DocumentOnMount)
{
    loadDocument(DOCUMENT_ON_MOUNT);
    ASSERT_TRUE(component);

    // The text value change was immediate
    ASSERT_EQ(Object("Ha!"), component->getCalculated(kPropertyText).asString());

    // No dirty properties should be set
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto args = event.getValue(kEventPropertyArguments);
    ASSERT_TRUE(args.isArray());
    ASSERT_EQ(5, args.size());
    ASSERT_EQ("Document", args.at(0).asString());
    ASSERT_EQ("Mount", args.at(1).asString());
    ASSERT_TRUE(args.at(2).isNull());
    ASSERT_TRUE(args.at(3).isNull());
    ASSERT_TRUE(args.at(4).isNull());
}

static const char *DOCUMENT_ON_MOUNT_DELAYED =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"id\": \"myText\","
    "      \"color\": \"red\","
    "      \"onMount\": ["
    "        {"
    "          \"type\": \"SetValue\","
    "          \"property\": \"text\","
    "          \"value\": \"uh-oh\","
    "          \"delay\": 1000"
    "        },"
    "        {"
    "          \"type\": \"SetValue\","
    "          \"property\": \"color\","
    "          \"value\": \"blue\","
    "          \"delay\": 1000"
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"onMount\": {"
    "    \"type\": \"SetValue\","
    "    \"componentId\": \"myText\","
    "    \"property\": \"text\","
    "    \"value\": \"Ha!\","
    "    \"delay\": 1000"
    "  }"
    "}";


TEST_F(MountTest, DocumentOnMountDelayed)
{
    loadDocument(DOCUMENT_ON_MOUNT_DELAYED);
    ASSERT_TRUE(component);

    // There should be a delay of 1000 before the first change
    ASSERT_EQ(Object(""), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::RED)), component->getCalculated(kPropertyColor));

    loop->updateTime(1000);
    ASSERT_EQ(Object("uh-oh"), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::RED)), component->getCalculated(kPropertyColor));

    loop->updateTime(2000);
    ASSERT_EQ(Object("uh-oh"), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::BLUE)), component->getCalculated(kPropertyColor));

    loop->updateTime(3000);
    ASSERT_EQ(Object("Ha!"), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::BLUE)), component->getCalculated(kPropertyColor));
}

TEST_F(MountTest, DocumentOnMountTerminated)
{
    loadDocument(DOCUMENT_ON_MOUNT_DELAYED);
    ASSERT_TRUE(component);

    // There should be a delay of 1000 before the first change
    ASSERT_EQ(Object(""), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::RED)), component->getCalculated(kPropertyColor));

    loop->updateTime(1000);
    ASSERT_EQ(Object("uh-oh"), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::RED)), component->getCalculated(kPropertyColor));

    root->cancelExecution();

    // The document onMount should have run in fast mode
    ASSERT_EQ(Object("Ha!"), component->getCalculated(kPropertyText).asString());

    // But the last component setvalue was skipped.
    ASSERT_EQ(Object(Color(Color::RED)), component->getCalculated(kPropertyColor));
}

static const char *DOCUMENT_ON_MOUNT_TERMINATED_2 =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"id\": \"myText\","
    "      \"color\": \"red\","
    "      \"onMount\": ["
    "        {"
    "          \"type\": \"SetValue\","
    "          \"property\": \"text\","
    "          \"value\": \"uh-oh\","
    "          \"delay\": 1000"
    "        },"
    "        {"
    "          \"type\": \"SetValue\","
    "          \"property\": \"color\","
    "          \"value\": \"blue\","
    "          \"delay\": 1000"
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"onMount\": ["
    "    {"
    "      \"type\": \"SetValue\","
    "      \"componentId\": \"myText\","
    "      \"property\": \"text\","
    "      \"value\": \"Ha!\","
    "      \"delay\": 1000"
    "    },"
    "    {"
    "      \"type\": \"SetValue\","
    "      \"componentId\": \"myText\","
    "      \"property\": \"text\","
    "      \"value\": \"Ha-Ha!\","
    "      \"delay\": 1000"
    "    },"
    "    {"
    "      \"type\": \"SetValue\","
    "      \"componentId\": \"myText\","
    "      \"property\": \"text\","
    "      \"value\": \"Ha-Ha-Ha!\","
    "      \"delay\": 1000"
    "    }"
    "  ]"
    "}";

TEST_F(MountTest, DocumentOnMountLong)
{
    loadDocument(DOCUMENT_ON_MOUNT_TERMINATED_2);
    ASSERT_TRUE(component);

    // Starting condition
    ASSERT_EQ(Object(""), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::RED)), component->getCalculated(kPropertyColor));

    // Ending condition
    loop->updateTime(5000);
    ASSERT_EQ(Object("Ha-Ha-Ha!"), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::BLUE)), component->getCalculated(kPropertyColor));
}

TEST_F(MountTest, TerminateInComponents)
{
    loadDocument(DOCUMENT_ON_MOUNT_TERMINATED_2);
    ASSERT_TRUE(component);

    // Starting condition
    ASSERT_EQ(Object(""), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::RED)), component->getCalculated(kPropertyColor));

    loop->updateTime(1000);
    ASSERT_EQ(Object("uh-oh"), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::RED)), component->getCalculated(kPropertyColor));

    root->cancelExecution();
    loop->runPending();

    ASSERT_EQ(Object("Ha-Ha-Ha!"), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::RED)), component->getCalculated(kPropertyColor));
}

TEST_F(MountTest, TerminateInDocument)
{
    loadDocument(DOCUMENT_ON_MOUNT_TERMINATED_2);
    ASSERT_TRUE(component);

    // Starting condition
    ASSERT_EQ(Object(""), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::RED)), component->getCalculated(kPropertyColor));

    loop->updateTime(3000);
    ASSERT_EQ(Object("Ha!"), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::BLUE)), component->getCalculated(kPropertyColor));

    // Terminating in the middle of running the Ha-Ha! onMount command.  The last command should run in fast mode.
    root->cancelExecution();
    loop->runPending();

    ASSERT_EQ(Object("Ha-Ha-Ha!"), component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Color(Color::BLUE)), component->getCalculated(kPropertyColor));
}

static const char *DOCUMENT_ON_MOUNT_TERMINATED_NO_DOCUMENT_CMD =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Text\","
        "      \"id\": \"myText\","
        "      \"color\": \"red\","
        "      \"onMount\": ["
        "        {"
        "          \"type\": \"SetValue\","
        "          \"property\": \"text\","
        "          \"value\": \"uh-oh\","
        "          \"delay\": 1000"
        "        },"
        "        {"
        "          \"type\": \"SetValue\","
        "          \"property\": \"color\","
        "          \"value\": \"blue\","
        "          \"delay\": 1000"
        "        }"
        "      ]"
        "    }"
        "  }"
        "}";


TEST_F(MountTest, TerminateNoDocumentCommand)
{
    loadDocument(DOCUMENT_ON_MOUNT_TERMINATED_NO_DOCUMENT_CMD);
    ASSERT_TRUE(component);

    // Starting condition
    ASSERT_TRUE(IsEqual("", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyColor)));

    loop->updateTime(1000);
    ASSERT_TRUE(IsEqual("uh-oh", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyColor)));

    root->cancelExecution();
    loop->runPending();

    ASSERT_TRUE(IsEqual("uh-oh", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyColor)));
}

TEST_F(MountTest, TerminateUnexpectedly)
{
    loadDocument(DOCUMENT_ON_MOUNT_TERMINATED_NO_DOCUMENT_CMD);
    ASSERT_TRUE(component);

    // Starting condition
    ASSERT_TRUE(IsEqual("", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyColor)));

    loop->updateTime(1000);
    ASSERT_TRUE(IsEqual("uh-oh", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyColor)));

    // Now terminate without giving a chance to clean up.  This test case was added
    // because a bug in DocumentAction would attempt to execute "finally" commands
    // on termination even though the DocumentCommand no longer had a valid context.
}