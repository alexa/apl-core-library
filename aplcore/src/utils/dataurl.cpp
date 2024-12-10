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

#include "apl/utils/dataurl.h"
#include "apl/utils/dataurlgrammar.h"
#include "apl/utils/session.h"

namespace apl {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

DataUrlPtr
DataUrl::create(const SessionPtr& session, const std::string& url) {
    pegtl::memory_input<> in(url, "");
    dataurlgrammar::dataurl_state state(in);

    if (!pegtl::parse<dataurlgrammar::dataurl, dataurlgrammar::dataurl_action, dataurlgrammar::dataurl_control>(in, state) ||
        state.failed) {
        CONSOLE(session) << "Error: " << state.what() << ", parsing data URL: '" << url << "'";
        return nullptr;
    }
    return std::make_shared<DataUrl>(url, state.data, state.type, state.subtype);
}

} // namespace apl