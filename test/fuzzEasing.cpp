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
#include <ctime>

#include "apl/apl.h"
#include "apl/animation/easing.h"
#include "apl/engine/evaluate.h"

using namespace apl;

class EasingLogBridge : public LogBridge {
    void transport(LogLevel level, const std::string& log) override {
    }
};

void
usage(const std::string& msg="")
{
    if (!msg.empty())
        std::cout << msg << std::endl;
    std::cout << "Usage: fuzzEasing [options] EXPR" << std::endl
              << std::endl
              << "  Parse a fuzzed easing curve.  Each time the easing is parsed" << std::endl
              << "  random characters are deleted and added to the easing curve.  The initial" << std::endl
              << "  easing should be a valid easing curve such as 'path(0.02,0.5)'." << std::endl
              << std::endl
              << "Options: " << std::endl
              << "  -h | --help               Print this help" << std::endl
              << "  -s | --seed SEED          Set the random number seed" << std::endl
              << "  -r | --repeat COUNT       Number of trials to execute (defaults to 1000)" << std::endl
              << "  -d | --duration SECONDS   Run for a number of seconds as given by wall time" << std::endl
              << "  -v | --verbose            Verbose display. May be repeated" << std::endl;
    exit(1);
}

int randomLessThan(int max)
{
    return rand() % max;
}

const std::string LIKELY = "abcdefghijklmnopqrstuvwxyz0123456789(),. -";

std::string
fuzz(const std::string& original) {
    std::string result = original;
    int count, deleteCount;

    // Insert random characters
    count = randomLessThan(5) + 1;
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
main(int argc, char *argv[]) {
    unsigned long repeatCount = 1000;
    int verbose = 0;
    unsigned seed = 101;
    unsigned long duration = 0;

    std::vector<std::string> args(argv + 1, argv + argc);
    for (auto iter = args.begin(); iter != args.end();) {
        if (*iter == "-h" || *iter == "--help")
            usage("");

        if (*iter == "-v" || *iter == "--verbose") {
            verbose += 1;
            iter = args.erase(iter);
        } else if (*iter == "-s" || *iter == "--seed") {
            iter = args.erase(iter);
            if (iter == args.end())
                usage("seed expects a value");
            seed = std::stoul(*iter);
            iter = args.erase(iter);
        } else if (*iter == "-r" || *iter == "--repeat") {
            iter = args.erase(iter);
            if (iter == args.end())
                usage("repeat count expects a value");
            repeatCount = std::stoul(*iter);
            iter = args.erase(iter);
        } else if (*iter == "-d" || *iter == "--duration") {
            iter = args.erase(iter);
            if (iter == args.end())
                usage("duration expects a value in seconds");
            duration = std::stoul(*iter);
            iter = args.erase(iter);
        } else {
            iter++;
        }
    }

    if (args.size() != 1)
        usage("Must pass an easing curve as a starting point");

    LoggerFactory::instance().initialize(std::make_shared<EasingLogBridge>());

    std::srand(seed);
    std::string easing = args[0];

    std::cout << "Starting fuzzing run of '" << easing << "'";
    if (duration > 0)
        std::cout << " duration=" << duration << std::endl;
    else
        std::cout << " iterations=" << repeatCount << std::endl;

    auto session = makeDefaultSession();
    auto original = Easing::parse(session, easing);
    auto startTime = std::time(nullptr);
    auto stopTime = startTime + duration;

    unsigned long i = 0;
    while (duration > 0 || i < repeatCount) {
        std::string variant = fuzz(easing);
        auto result = Easing::parse(session, variant);

        if (verbose > 1 || (verbose > 0 && result != original)) {
            apl::streamer s;
            s << result->toDebugString();
            std::cout << i << " '" << variant << "' " << s.str() << std::endl;
        }

        if (duration > 0 && i % 10 == 0 && time(nullptr) >= stopTime)
            break;
        i++;
    }

    std::cout << "Successfully fuzzed '" << easing << "' " << i << " times in "
              << time(nullptr) - startTime << " seconds" << std::endl;
}
