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
/*
 * Test expression parsing
 */

#include "utils.h"

#include "apl/engine/evaluate.h"
#include "apl/utils/dump_object.h"

static const char *USAGE_STRING = "parseExpression [OPTIONS] EXPRESSION*";

int
main(int argc, char *argv[])
{
    ArgumentSet argumentSet(USAGE_STRING);
    ViewportSettings settings(argumentSet);

    bool verbose = false;
    argumentSet.add({
        Argument("-v", "--verbose", Argument::NONE, "Display the parsed node tree", "",
                 [&](const std::vector<std::string>&){
          verbose = true;
        })
    });

    std::vector<std::string> args(argv + 1, argv + argc);
    argumentSet.parse(args);

    auto c = settings.createContext();

    for (const auto& m : args) {
        auto parsed = parseDataBinding(*c, m);

        if (verbose)
            apl::DumpVisitor::dump(parsed);

        auto result = parsed.eval();
        std::cout << result.toDebugString() << std::endl;
    }
}
