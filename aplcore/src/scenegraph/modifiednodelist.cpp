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

#include "apl/scenegraph/modifiednodelist.h"
#include "apl/scenegraph/node.h"

namespace apl {
namespace sg {


static NodePtr sTerminal = std::make_shared<GenericNode>();

ModifiedNodeList::ModifiedNodeList()
{
    // Always start the list with a known terminal.
    mModified = sTerminal;
}

ModifiedNodeList::~ModifiedNodeList()
{
    clear();
}

void
ModifiedNodeList::clear()
{
    // Release all of the node chain
    auto *ptr = mModified.get();
    while (ptr) {
        ptr->clearFlags();
        auto *next = ptr->mNextModified.get();
        ptr->mNextModified.reset();
        ptr = next;
    }
}

void
ModifiedNodeList::contentChanged(Node* node)
{
    assert(node);
    node->setFlag(Node::kNodeFlagModified);

    if (!node->mNextModified) {
        node->mNextModified = mModified;
        mModified = node->shared_from_this();
    }
}

void
ModifiedNodeList::childChanged(Node* node)
{
    assert(node);
    node->setFlag(Node::kNodeFlagChildrenChanged);

    if (!node->mNextModified) {
        node->mNextModified = mModified;
        mModified = node->shared_from_this();
    }
}

} // namespace sg
} // namespace apl
