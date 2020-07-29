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
 * Testing color conversion
 */

#include "apl/engine/evaluate.h"
#include "apl/primitives/color.h"
#include "apl/utils/session.h"

#include <iostream>
#include <iomanip>

const char usage_string[] = "parseColor [-h|--help] COLOR+";

int
main(int argc, char *argv[])
{
    std::vector<std::string> args(argv + 1, argv + argc);

    if (args.size() == 0  || args[0] == "-h" || args[0] == "--help") {
        std::cout << usage_string << std::endl;
        exit(1);
    }

    auto session = apl::makeDefaultSession();
    for (const auto& m : args) {
        auto color = apl::Color(session, m);
        apl::streamer s;
        s << color;
        std::cout << s.str() << std::endl;
    }
}
