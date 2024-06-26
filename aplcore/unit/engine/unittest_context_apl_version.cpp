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
 *
 */

#include "../testeventloop.h"

using namespace apl;

/**
 * These tests are to verify that the data-binding context is created with the APL version specified
 * by the APL document.  A number of features are gated upon the version of APL requested by the
 * document.  The environment variables of the context report APL version in two places:
 *
 * environment.aplVersion
 *
 *     This is the reported APL version.  By default it is set to the current (most recent) version
 *     in core.  It can be overridden by calling:
 *
 *         rootConfig.set(RootProperty::kReportedVersion, STRING);
 *
 * environment.documentAPLVersion
 *
 *     This is the version of APL specified by the APL document.  This applies to data-binding
 *     contexts that have been created when the document is known.  Some contexts are only used
 *     for simple evaluations; in those cases they default to the current Core APL version.
 */
class ContextAPLVersionTest : public DocumentWrapper {};

static const char *BASIC = R"(
{
  "type": "APL",
  "version": "1.9",
  "background": "${environment.documentAPLVersion == '1.9' ? 'red' : 'blue' }",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "text": ""
    }
  }
})";

TEST_F(ContextAPLVersionTest, Basic)
{
    loadDocument(BASIC);
    auto context = component->getContext();
    ASSERT_EQ("1.9", context->getRequestedAPLVersion());
    ASSERT_TRUE(IsEqual("2024.2", evaluate(*context, "${environment.aplVersion}")));
    ASSERT_TRUE(IsEqual("1.9", evaluate(*context, "${environment.documentAPLVersion}")));

    // The document background is evaluated is a special data-binding context
    ASSERT_TRUE(IsEqual(content->getBackground(Metrics(), RootConfig()), Color(Color::RED)));
}
