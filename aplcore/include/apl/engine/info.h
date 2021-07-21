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

#ifndef _APL_INFO_H
#define _APL_INFO_H

#include <memory>
#include <string>

namespace apl {

class RootContextData;
class Context;

/**
 * Information about the elements defined within a root context.  This
 * class serves as a wrapper for programs that wish to display the types
 * and sources of the different visual elements.
 */
class Info {
public:
    /// Type of object that information will be returned about
    enum InfoType {
        /// Commands
        kInfoTypeCommand,
        /// Vector graphics
        kInfoTypeGraphic,
        /// Custom layouts
        kInfoTypeLayout,
        /// Styles
        kInfoTypeStyle
    };

    Info(const ContextPtr& context, const std::shared_ptr<RootContextData>& core)
        : mContext(context), mCore(core) {}

    /**
     * Return the number of named items of a particular type.
     * @param infoType The type of item to return.
     * @return The number of items.
     */
    size_t count(InfoType infoType) const;

    /**
     * Return the name and provenance of an item by index
     * @param infoType The type of object to return
     * @param index The index of the object
     * @return A pair containing the name of the item and the provenance path.
     */
    std::pair<std::string, std::string> at(InfoType infoType, size_t index) const;

    /**
     * Return a map of all defined resources and provenance
     */
    std::map<std::string, std::string> resources() const;

private:
    ContextPtr mContext;
    std::shared_ptr<RootContextData> mCore;
};

} // namespace apl

#endif //_APL_INFO_H
