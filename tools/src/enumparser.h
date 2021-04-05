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

#ifndef _APL_ENUMPARSER_H
#define _APL_ENUMPARSER_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

namespace enums {

struct EnumParserP;

/**
 * A single name/value pair in an enumeration.
 */
struct EnumItem {
    std::string name;
    int value;
    std::string comment;
};

using EnumMap = std::map<std::string, std::vector<EnumItem>>;

/**
 * A parser of enumerations
 */
class EnumParser {
public:
    /**
     * Sets the verbosity of the parser.  The default is to output only error conditions.
     * 0=Only error messages
     * 1=n/a
     * 2=Report each processed enumeration
     * 3=Show internal processing of each enumeration
     */
    static int sVerbosity;

    EnumParser();
    ~EnumParser();

    /**
     * Add a new data file containing one or more enumerations to this parser.
     * @param input The input stream containing the data file.
     * @param msg An optional attached message.  The name of the data file is a good choice.
     */
    void add(std::istream&& input, std::string msg = "");

    /**
     * @return A map of all enumerations that have been found.
     */
    EnumMap enumerations();

private:
    std::unique_ptr<EnumParserP> mPrivate;
};

/**
 * Write a collection of enumerated items out as a Java file
 * @param out The output stream to write to.
 * @param package The name of the Java package
 * @param name The name of the Java class
 * @param items The ordered list of enumeration items
 * @return The output stream (for chaining)
 */
std::ostream& writeJava(std::ostream& out,
                        const std::string& package,
                        const std::string& name,
                        const std::vector<EnumItem>& items);

/**
 * Write a collection of enumerated items out as a TypeScript file
 * @param out The output stream to write to.
 * @param name The name of the Java class
 * @param items The ordered list of enumeration items
 * @return The output stream (for chaining)
 */
std::ostream& writeTypeScript(std::ostream& out,
                              const std::string& name,
                              const std::vector<EnumItem>& values);

} // namespace enums

#endif // _APL_ENUMPARSER_H
