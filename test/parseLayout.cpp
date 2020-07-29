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
 * Loading and displaying results of parsing a layout file.
 */

#include "utils.h"

#include <iostream>

static const char *USAGE_STRING = "parseLayout [OPTIONS] LAYOUT DATA*";

int
main(int argc, char *argv[])
{
    ArgumentSet argumentSet(USAGE_STRING);
    ViewportSettings settings(argumentSet);

    std::vector<std::string> args(argv + 1, argv + argc);
    argumentSet.parse(args);

    auto c = createContext(args, settings);

    auto componentPtr = c->topComponent();
    apl::streamer s;
    s << settings;
    std::cout << s.str() << std::endl;
    MyVisitor::dump(*std::static_pointer_cast<apl::CoreComponent>(componentPtr));
}
