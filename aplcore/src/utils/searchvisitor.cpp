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


#include "apl/component/corecomponent.h"
#include "apl/content/rootconfig.h"
#include "apl/utils/visitor.h"
#include "apl/utils/searchvisitor.h"

namespace apl {

static const bool DEBUG_SEARCH = false;

/*
 * SearchVisitor base class IMPL
 */

void
SearchVisitor::visit(const CoreComponent& component) {
    Point pointInCurrent = component.toLocalPoint(mGlobalPoint);
    if (!pointInCurrent.isFinite()) {
        mPruneBranch = true;
        return;
    }

    LOG_IF(DEBUG_SEARCH) << "Checking " << component.toDebugSimpleString()
                                   << " bounds=" << component.getCalculated(kPropertyBounds).toDebugString()
                                   << " point=" << pointInCurrent.toString()
                                   << " scrollPosition=" << component.scrollPosition();

    if (!universalCondition(component, pointInCurrent)) {
        mPruneBranch = true;
        return;
    }

    if (spotCondition(component, pointInCurrent)) {
        // if the component satisfies the spot condition, cache it as a potential result so we can avoid a stack
        mPotentialResult =
            std::const_pointer_cast<CoreComponent>(component.shared_from_corecomponent());
        LOG_IF(DEBUG_SEARCH) << "Found potential result " << component.toDebugSimpleString();
    }

    // If we reached the a leaf node, then keep the best result encountered so far. We specifically
    // avoid backtracking and exploring a different subtree because components can overlap and mask
    // each other.
    if (component.getChildCount() == 0) {
        mResultFound = true;
    }
}

void
SearchVisitor::push() {}

void
SearchVisitor::pop() {
    if (!isAborted()) {
        mResultFound = (mPotentialResult != nullptr);
    }
    mPruneBranch = false;
}

bool
SearchVisitor::isAborted() const {
    return mPruneBranch || mResultFound;
}

CoreComponentPtr
SearchVisitor::getResult() const {
    return mResultFound ? mPotentialResult : nullptr;
}

/*
 * TouchableAtPosition IMPL
 */

bool
TouchableAtPosition::universalCondition(const CoreComponent& component,
                                        const Point& pointInCurrent) {
    return component.containsLocalPosition(pointInCurrent)
           && component.getCalculated(kPropertyDisplay).asInt() == kDisplayNormal
           && component.getCalculated(kPropertyOpacity).asNumber() > 0.0;
}

bool
TouchableAtPosition::spotCondition(const CoreComponent& component, const Point& point) {
    return component.isActionable();
}

/*
 * TopAtPosition IMPL
 */

bool
TopAtPosition::universalCondition(const CoreComponent& component, const Point& pointInCurrent) {
    return component.containsLocalPosition(pointInCurrent)
           && component.getCalculated(kPropertyDisplay).asInt() == kDisplayNormal
           && component.getCalculated(kPropertyOpacity).asNumber() > 0.0;
}
}
