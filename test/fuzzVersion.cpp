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

#include <iostream>
#include <string>

#include "apl/apl.h"
#include "utils.h"

using namespace apl;

class VersionLogBridge : public LogBridge {
    void transport(LogLevel level, const std::string& log) override {
    }
};

void
usage(const std::string& msg="")
{
    if (!msg.empty())
        std::cout << msg << std::endl;
    std::cout << "Usage: fuzzVersion [options]" << std::endl
              << std::endl
              << "  Call fuzzVersion which tests variants of version field for Import Package" << std::endl
              << std::endl
              << "Options: " << std::endl
              << "  -h | --help               Print this help" << std::endl
              << "  -s | --seed SEED          Set the random number seed" << std::endl
              << "  -r | --repeat COUNT       Number of trials to execute (defaults to 1000)" << std::endl
              << "  -v | --verbose            Verbose display. May be repeated" << std::endl;
    exit(1);
}

int
main(int argc, char *argv[])
{
    unsigned long repeatCount = 1000;
    int verbose = 0;
    unsigned seed = 101;

    std::vector<std::string> args(argv + 1, argv + argc);
    for (auto iter = args.begin() ; iter != args.end() ; ) {
        if (*iter == "-h" || *iter == "--help")
            usage("");

        if (*iter == "-v" || *iter == "--verbose") {
            verbose += 1;
            iter = args.erase(iter);
        }
        else if (*iter == "-s" || *iter == "--seed") {
            iter = args.erase(iter);
            if (iter == args.end())
                usage("seed expects a value");
            seed = std::stoul(*iter);
            iter = args.erase(iter);
        }
        else if (*iter == "-r" || *iter == "--repeat") {
            iter = args.erase(iter);
            if (iter == args.end())
                usage("repeat count expects a value");
            repeatCount = std::stoul(*iter);
            iter = args.erase(iter);
        }
        else {
            iter++;
        }
    }

    LoggerFactory::instance().initialize(std::make_shared<VersionLogBridge>());
    auto session = apl::makeDefaultSession();

    std::srand(seed);
    ImportPackageUtils importPackageUtils;

    for (unsigned long i = 0 ; i < repeatCount ; i++) {
        std::string version = importPackageUtils.generateFuzzyVersion();;
        auto p = SemanticVersion::create(session, version);
        if (verbose > 0) {
            apl::streamer s;
            s << version;
            std::cout << i << " '" << version << "' " << std::endl;
        }
    }

    std::cout << "Successfully fuzzed " << repeatCount << " times" << std::endl;
}

