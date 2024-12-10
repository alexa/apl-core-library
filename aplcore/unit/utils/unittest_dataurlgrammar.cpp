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

#include <tao/pegtl.hpp>

#include "../testeventloop.h"

using namespace apl;
namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

class DatagrammarTest : public DocumentWrapper {};

struct DataurlTest {
    std::string data;
    bool result;
    std::string details;
};

static const std::vector<DataurlTest> DATAURL_TESTS{
    {R"(data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyuvUUlvON)", true, "Basic valid"},
    {R"(data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyuvUUlvO=)", true, "Single padded character"},
    {R"(data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyuvUUlv==)", true, "Two padded characters"},
    {R"(data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyuvUUl===)", false, "Invalid padding"},
    {R"(data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAA_AMAAwAAAC8IyPqcvt3-CcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyuvUUlvON)", false, "Unsupported characters"},
    {R"(data:video/mp4;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyuvUUlvON)", false, "Wrong type"},
    {R"(data:,A%20brief%20note)", false, "Valid data url, but not base64 image"},
    {R"(data:image/png;,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyuvUUlvON)", false, "Not valid base64 extension"},
    {R"(data:image/png;charset=iso-8859-7;potatoes=yes;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyuvUUlvO=)", true, "Valid base64 with optional parameters"},

};

TEST_F(DatagrammarTest, Basic)
{
    for (const auto& m : DATAURL_TESTS) {
        auto dataurl = DataUrl::create(session, m.data);
        session->dumpAndClear();
        ASSERT_EQ(m.result, dataurl != nullptr) << m.details;
    }
}

TEST_F(DatagrammarTest, Extract)
{
    auto dataurl = DataUrl::create(session, R"(data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyuvUUlvON)");
    ASSERT_TRUE(dataurl != nullptr);
    ASSERT_EQ(R"(data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyuvUUlvON)", dataurl->getUrl());
    ASSERT_EQ(R"(R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyuvUUlvON)", dataurl->getData());
    ASSERT_EQ("image", dataurl->getType());
    ASSERT_EQ("png", dataurl->getSubtype());
}