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

#include <chrono>

#include "utils.h"

#include "apl/datagrammar/bytecode.h"
#include "apl/engine/evaluate.h"
#include "apl/primitives/symbolreferencemap.h"
#include "apl/utils/dump_object.h"

static const char *USAGE_STRING = "parseExpression [OPTIONS] EXPRESSION*";

int
main(int argc, char *argv[])
{
    ArgumentSet argumentSet(USAGE_STRING);
    ViewportSettings settings(argumentSet);

    bool optimize = false;
    long repetitions = 0;
    bool verbose = false;

    argumentSet.add({
        Argument("-o",
                 "--optimize",
                 Argument::NONE,
                 "Run the byte code optimizer",
                 "",
                 [&](const std::vector<std::string>&) {
                     optimize = true;
                 }),
        Argument("-n",
                 "--number",
                 Argument::ONE,
                 "Measure performance over multiple repetitions",
                 "REPS",
                 [&](const std::vector<std::string>& value) {
                     repetitions = std::stol(value[0]);
                     if (repetitions < 0)
                         repetitions = 0;
                 }),
        Argument("-v",
                 "--verbose",
                 Argument::NONE,
                 "Verbose",
                 "",
                 [&](const std::vector<std::string>&) {
                     verbose = true;
                 })
    });

    std::vector<std::string> args(argv + 1, argv + argc);
    argumentSet.parse(args);

    auto c = settings.createContext();

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    c->putSystemWriteable("utcTime", now.count());

    if (repetitions > 0) {
        auto start = std::chrono::high_resolution_clock::now();

        for (const auto& m : args) {
            // When the optimize flag is turned on, we optimize the expression once
            // and evaluate it "N" times
            if (optimize) {
                auto cbc = apl::getDataBinding(*c, m);
                if (cbc.isByteCode()) {
                    apl::SymbolReferenceMap map;
                    cbc.symbols(map);
                }

                for (long i = 0 ; i < repetitions ; i++)
                    cbc.eval();
            }
            else {  // When the optimize flag is turned off, we evaluated the expression "N" times.
                for (long i = 0 ; i < repetitions ; i++)
                    apl::evaluate(*c, m);
            }
        }

        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        std::cout << u8"Duration (µs): " << duration.count() << std::endl
                  << u8"Average  (µs): " << (duration.count() / repetitions) << std::endl;
    }
    else {
        for (const auto& m : args) {
            if (verbose)
                std::cout << "parsing '" << m << "'" << std::endl;

            auto cbc = apl::getDataBinding(*c, m);

            if (verbose && cbc.isByteCode())
                cbc.getByteCode()->dump();

            std::cout << "Evaluates to " << cbc.eval().asString() << std::endl;

            if (optimize && cbc.isByteCode()) {
                apl::SymbolReferenceMap map;
                cbc.symbols(map);
                std::cout << std::endl << "Optimized version" << std::endl;
                cbc.getByteCode()->dump();
                std::cout << "optimized version evaluates to " << cbc.eval().asString() << std::endl;
            }
        }
    }
}
