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
#include "apl/utils/dump_object.h"

static const char *USAGE_STRING = "parseExpression [OPTIONS] EXPRESSION*";

void showSymbols(const apl::BoundSymbolSet& symbols)
{
    if (symbols.empty())
        std::cout << "No symbols referenced";
    else {
        std::cout << "Symbols referenced:";
        for (const auto& symbol : symbols)
            std::cout << " " << symbol.getName();
        std::cout << std::endl;
    }
}

int
main(int argc, char *argv[])
{
    ArgumentSet argumentSet(USAGE_STRING);
    ViewportSettings settings(argumentSet);

    bool optimize = false;
    long repetitions = 0;
    bool verbose = false;
    bool show_symbols = false;
    bool decompile = false;

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
                 }),
        Argument("-S",
                 "--symbols",
                 Argument::NONE,
                 "Show referenced symbols used when evaluating the expression",
                 "",
                 [&](const std::vector<std::string>&) {
                     show_symbols = true;
                 }),
        Argument("-D",
                 "--decompile",
                 Argument::NONE,
                 "Decompile the byte code and display",
                 "",
                 [&](const std::vector<std::string>&) {
                decompile = true;
            }),
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
            auto result = apl::parseAndEvaluate(*c, m, optimize);
            for (long i = 0; i < repetitions; i++)
                result.expression.eval();
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

            auto result = apl::parseAndEvaluate(*c, m, optimize);

            if (decompile && result.expression.is<apl::datagrammar::ByteCode>()) {
                for (const auto& m : result.expression.get<apl::datagrammar::ByteCode>()->disassemble()) {
                    std::cout << m << std::endl;
                }
            }

            std::cout << "Evaluates to " << result.value.toDebugString() << std::endl;
            if (show_symbols)
                showSymbols(result.symbols);
        }
    }
}
