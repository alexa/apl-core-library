/*
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
/*
 * Test expression parsing
 */

#include "utils.h"

#include "apl/datagrammar/databindingrules.h"
#include "apl/utils/dump_object.h"

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

static const char *USAGE_STRING = "parseExpression [OPTIONS] EXPRESSION*";

int
main(int argc, char *argv[])
{
    ArgumentSet argumentSet(USAGE_STRING);
    ViewportSettings settings(argumentSet);

    bool verbose = false;
    argumentSet.add({
        Argument("", "--tree", Argument::NONE, "Display the parsed node tree", "", [&](const std::string&){
          verbose = true;
        })
    });

    std::vector<std::string> args(argv + 1, argv + argc);
    argumentSet.parse(args);

    apl::streamer s;
    s << settings;
    std::cout << s.str() << std::endl;
    s.reset();
    auto c = settings.createContext();

    for (const auto& m : args) {
        apl::datagrammar::Stacks stacks(*c);
        pegtl::string_input<> in(m, "");
        pegtl::parse< apl::datagrammar::grammar, apl::datagrammar::action >(in, stacks);
        auto tree = stacks.finish();

        if (verbose)
            apl::DumpVisitor::dump(tree);

        auto result = tree.eval();
        s << result;
        std::cout << s.str() << std::endl;
    }
}
