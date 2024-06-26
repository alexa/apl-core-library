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

#ifndef _APL_CONSTANTS_H
#define _APL_CONSTANTS_H

#include <string>

namespace apl {

/*****************************************************************/

const std::string COMPONENT_UID = "uid";
const std::string COMPONENT_INDEX = "index";
const std::string COMPONENT_ORDINAL = "ordinal";
const std::string COMPONENT_LENGTH = "length";
const std::string COMPONENT_DATA_INDEX = "dataIndex";
const std::string COMPONENT_DATA = "data";
const std::string COMPONENT_CONTEXT_SOURCE = "__source";
const std::string COMPONENT_CONTEXT_NAME = "__name";

/*****************************************************************/

/// Context belongs to "firstItem" expanded component
const std::string REBUILD_IS_FIRST_ITEM = "__isFirstItem";
/// Context belongs to "lastItem" expanded component
const std::string REBUILD_IS_LAST_ITEM = "__isLastItem";
/// Object key for firstItem definition reference
const std::string REBUILD_FIRST_ITEMS = "__firstItems";
/// Object key for lastItem definition reference
const std::string REBUILD_LAST_ITEMS = "__lastItems";
/// Object key for item definition reference
const std::string REBUILD_ITEMS = "__items";
/// Source index value for non-live data controlled children
const std::string REBUILD_SOURCE_INDEX = "__sourceIndex";

} // namespace apl

#endif //_APL_CONSTANTS_H
