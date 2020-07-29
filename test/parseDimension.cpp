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
 * Testing dimension conversion
 */

#include "utils.h"

#include "apl/primitives/dimension.h"

static const char USAGE_STRING[] = "parseDimension [OPTIONS] DIMENSION+";

int
main(int argc, char *argv[])
{
    ArgumentSet argumentSet(USAGE_STRING);
    ViewportSettings settings(argumentSet);

    std::vector<std::string> args(argv + 1, argv + argc);
    argumentSet.parse(args);

    if (args.size() == 0) {
        std::cerr << "No dimensions listed" << std::endl;
        exit(1);
    }

    apl::streamer s;
    s << settings;
    std::cout << s.str() << std::endl;

    auto c = settings.createContext();

    for (const auto& m : args) {
        s.reset();
        auto dimen = apl::Dimension(*c, m);
        s << dimen;
        std::cout << s.str() << std::endl;
    }
}
