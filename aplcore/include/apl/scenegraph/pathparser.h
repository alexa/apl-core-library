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

#ifndef _APL_SG_PATH_PARSER_H
#define _APL_SG_PATH_PARSER_H

#include <string>
#include <vector>

namespace apl {
namespace sg {

/**
 * Given an AVG pathData string, calculate an optimal GeneralPath.  If the path data is
 * malformed the GeneralPath will be empty.
 *
 * @param path The AVG pathData string
 * @return A GeneralPath
 */
std::shared_ptr<GeneralPath> parsePathString(const std::string& path);

} // namespace sg
} // namespace apl

#endif // _APL_SG_PATH_PARSER_H
