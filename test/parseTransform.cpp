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

#include "utils.h"

#include "apl/primitives/transform2d.h"

static const char *USAGE_STRING = "parseTransform [OPTIONS] EXPRESSION";

int
main(int argc, char *argv[]) {
    ArgumentSet argumentSet(USAGE_STRING);
    ViewportSettings settings(argumentSet);

    std::vector<std::string> args(argv + 1, argv + argc);
    argumentSet.parse(args);

    auto c = settings.createContext();
    auto session = apl::makeDefaultSession();

    for (const auto &m : args) {
        auto p = apl::Transform2D::parse(session, m).get();
        std::cout << p.at(0) << ", " << p.at(2) << ", " << p.at(4) << std::endl
                  << p.at(1) << ", " << p.at(3) << ", " << p.at(5) << std::endl;
    }
}
