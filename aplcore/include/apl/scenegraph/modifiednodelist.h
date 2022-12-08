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

#ifndef _APL_MODIFIED_NODE_LIST_H
#define _APL_MODIFIED_NODE_LIST_H

#include "apl/scenegraph/common.h"

namespace apl {
namespace sg {

/**
 * Stores a simple linked list of nodes that have been modified.  This is an intrusive list;
 * the individual nodes have pointers to the next item in the list.  We guarantee that nodes
 * are only added once to the list.
 *
 * When the list is cleared, the flags of all the nodes in the list are cleared.
 */
class ModifiedNodeList {
public:
    ModifiedNodeList();
    ~ModifiedNodeList();

    void contentChanged(Node *node);
    void clear();

private:
    NodePtr mModified;
};

} // namespace sg
} // namespace apl

#endif // _APL_MODIFIED_NODE_LIST_H
