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
#include <vector>

#include "apl/apl.h"

using namespace apl;

class ColorLogBridge : public LogBridge {
    void transport(LogLevel level, const std::string& log) override {
    }
};

void
usage(const std::string& msg="")
{
    if (!msg.empty())
        std::cout << msg << std::endl;
    std::cout << "Usage: fuzzColor [options] expression" << std::endl
              << std::endl
              << "  Parse the expression and evaluate it as a color.  Each time it is parsed" << std::endl
              << "  random characters are deleted and added to the expression.  The initial" << std::endl
              << "  expression should be a valid color expression." << std::endl
              << std::endl
              << "Options: " << std::endl
              << "  -h | --help           Print this help" << std::endl
              << "  -s | --seed SEED      Set the random number seed" << std::endl
              << "  -r | --repeat COUNT   Number of trials to execute (defaults to 1000)" << std::endl
              << "  -v | --verbose        Verbose display. May be repeated" << std::endl;
    exit(1);
}

int randomLessThan(int max)
{
    return rand() % max;
}

const std::string LIKELY = "abcdefABCDEF0123456789()#%. ";

std::string
fuzz(const std::string& original) {
    std::string result = original;
    int count, deleteCount;

    // Insert random characters
    count = randomLessThan(10) + 1;
    std::string s;
    if (randomLessThan(4) == 0) {  // Completely random
        for (int i = 0; i < count; i++)
            s += randomLessThan(256);
    } else { // Random from LIKELY
        for (int i = 0; i < count; i++)
            s += LIKELY.at(randomLessThan(LIKELY.size()));
    }
    result.insert(randomLessThan(result.size()), s);

    // Delete some random characters
    deleteCount = randomLessThan(result.size());
    if (deleteCount > 0 && deleteCount > result.size()) {
        int index = randomLessThan(result.size() - deleteCount);
        result.erase(index, deleteCount);
    }

    return result;
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

    if (args.size() != 1)
        usage("Must pass an expression");

    LoggerFactory::instance().initialize(std::make_shared<ColorLogBridge>());
    auto session = apl::makeDefaultSession();

    std::srand(seed);
    const std::string expression(args[0]);
    auto origColor = Color(session, expression);

    for (unsigned long i = 0 ; i < repeatCount ; i++) {
        std::string variant = fuzz(expression);
        auto color = Color(session, variant);
        if (verbose > 1 ||
             (verbose > 0 && color != origColor && color != Color(Color::TRANSPARENT))) {
            apl::streamer s;
            s << color;
            std::cout << i << " '" << variant << "' " << s.str() << std::endl;
        }
    }
    std::cout << "Successfully fuzzed " << repeatCount << " times" << std::endl;
}
