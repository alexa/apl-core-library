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

#include <string>

#include "apl/versioning/semanticgrammar.h"

namespace apl {

namespace svgrammar {

template<> const std::string sv_control<number>::error_message = "expected at least one digit";
template<> const std::string sv_control<pegtl::eof>::error_message = "unexpected end";

} // namespace svgrammar

} // namespace apl