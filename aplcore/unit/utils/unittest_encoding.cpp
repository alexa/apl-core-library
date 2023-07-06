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

#include <cstring>

#include "gtest/gtest.h"

#include "../testeventloop.h"

using namespace apl;

class EncodingTest : public DocumentWrapper {};

static const char *ENCODING_BASIC = "{\"type\": \"APL\", \"version\": \"1.1\", \"mainTemplate\": {\"items\": {\"type\": \"Text\", \"item\": \"text\"}}}";
static const char16_t *ENCODING_UTF16 = u"{\"type\": \"APL\", \"version\": \"1.1\", \"mainTemplate\": {\"items\": {\"type\": \"Text\", \"item\": \"text\"}}}";

TEST_F(EncodingTest, Basic)
{
    createContent(ENCODING_BASIC, nullptr);
    ASSERT_TRUE(content);
}

TEST_F(EncodingTest, NonUtfSymbol)
{
    char buffer[100] = {0};
    std::string lstring(ENCODING_BASIC);
    size_t len = lstring.length();
    strncpy(buffer, ENCODING_BASIC, len);
    buffer[86] = 0xFE;
    createContent(buffer, nullptr);
    ASSERT_FALSE(content);
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(EncodingTest, Utf16)
{
    createContent((char*)ENCODING_UTF16, nullptr);
    ASSERT_FALSE(content);
    ASSERT_TRUE(ConsoleMessage());
}
