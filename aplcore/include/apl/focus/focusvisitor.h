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

#ifndef _APL_FOCUS_VISITOR_H
#define _APL_FOCUS_VISITOR_H

#include <vector>
#include <string>

#include "apl/common.h"
#include "apl/utils/visitor.h"

namespace apl {

/**
 * Simple visitor to search for focusable components in hierarchy. Ignores the one whose ID is provided.
 */
class FocusVisitor : public Visitor<CoreComponent> {
public:
    FocusVisitor(const std::string& rootId, bool fullTree) : mRootId(rootId), mFullTree(fullTree) {}

    void visit(const CoreComponent& component) override;
    void pop() override;
    bool isAborted() const override;

    std::vector<CoreComponentPtr> getResult() { return mFocusables; }

private:
    std::string mRootId;
    std::vector<CoreComponentPtr> mFocusables;
    bool mPruneBranch = false;
    bool mFullTree;
};

}

#endif //_APL_FOCUS_VISITOR_H
