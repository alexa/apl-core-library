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

#include "apl/utils/dataurlgrammar.h"

namespace apl {

namespace dataurlgrammar {

template<> const std::string dataurl_control<base64extension>::error_message = "Not a base64 extension";
template<> const std::string dataurl_control<pegtl::eof>::error_message = "Unexpected end";

} // namespace dataurlgrammar

} // namespace apl