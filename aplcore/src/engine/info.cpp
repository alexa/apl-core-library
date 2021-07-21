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

#include "apl/engine/context.h"
#include "apl/engine/info.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/engine/styles.h"

namespace apl {

size_t
Info::count(InfoType infoType) const
{
    switch (infoType) {
        case kInfoTypeCommand:
            return mCore->commands().size();
        case kInfoTypeGraphic:
            return mCore->graphics().size();
        case kInfoTypeLayout:
            return mCore->layouts().size();
        case kInfoTypeStyle:
            return mCore->styles()->size();
    }
    return 0;
}

std::pair<std::string, std::string>
Info::at(InfoType infoType, size_t index) const
{
    switch (infoType) {
        case kInfoTypeCommand:
            if (index < mCore->commands().size()) {
                auto it = std::next(mCore->commands().begin(), index);
                return { it->first, it->second.path().toString() };
            }
            break;

        case kInfoTypeGraphic:
            if (index < mCore->graphics().size()) {
                auto it = std::next(mCore->graphics().begin(), index);
                return { it->first, it->second.path().toString() };
            }
            break;

        case kInfoTypeLayout:
            if (index < mCore->layouts().size()) {
                auto it = std::next(mCore->layouts().begin(), index);
                return { it->first, it->second.path().toString() };
            }
            break;

        case kInfoTypeStyle:
            if (index < mCore->styles()->size()) {
                auto it = std::next(mCore->styles()->styleDefinitions().begin(), index);
                return { it->first, it->second->provenance().toString() };
            }
            break;
    }
    return {"", ""};
}

std::map<std::string, std::string>
Info::resources() const
{
    std::map<std::string, std::string> result;

    for (const auto& m : *mContext) {
        if (m.first.at(0) == '@')
            result.emplace(m.first, mContext->provenance(m.first));
    }

    return result;
}

} // namespace apl