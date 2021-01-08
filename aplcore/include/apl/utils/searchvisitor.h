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

#ifndef APL_SEARCHVISITOR_H
#define APL_SEARCHVISITOR_H

#include "apl/common.h"
#include "apl/primitives/point.h"

namespace apl {

/**
 * Visitor used when waking the component hierarchy that will find the "first" component in the linear ordering of the
 * traversal that satisfies the following:
 * - that component satisfies the spot condition
 * - that component and all ancestors satisfy the universal condition
 *
 * For a component hierarchy we typically are interested in the "topmost" component which is ordered last in the set of
 * children.  The appropriate linear ordering is reverse post order which can be achieved by reverse iterating through
 * the children and handling the root after all children are completed.
 *
 * To assist in the writing of universal and spot conditions, this visitor transforms the coordinates of the input point
 * such that at any point, it is defined in the same coordinate system as the component when they are arguments to the
 * condition functions.
 *
 * As the tree is traversed, the universal condition is applied to proactively prune branches before the leaves have
 * been reached once a parent that fails the condition is encountered.  Conversely, the spot condition is only required
 * on components that are themselves leaves or have no descendants that satisfy the spot condition.
 *
 * Correct application relies on a traversal driver function similar to:
 *
 * class Tree {
 *     void accept(Visitor<Tree>& visitor) const {
 *         visitor.visit(*this);
 *         visitor.push();
 *         for (auto it = mChildren.rbegin() ; !visitor.isAborted() && it != mChildren.rend() ; it++)
 *             it->accept(visitor);
 *         visitor.pop();
 *     }
 *
 *     std::vector<Tree> mChildren;
 * };
 *
 * The isAborted() call means that iteration should end for this set of children.  It may mean that a result has been
 * found, in which case the call stack will quickly pop out, or that this branch should be pruned.
 *
 * This specialization of the base Visitor class exposes the result of its search which can be retrieved upon completion
 * by calling getResult on the visitor.
 *
 * It is possible in the future that we might want to make these templated, e.g. SearchVisitor<NodeType, ArgType>
 */
class SearchVisitor : public Visitor<CoreComponent> {
public:
    /**
     * @param point The point at which to search
     */
    explicit SearchVisitor(const Point& point)
        : mGlobalPoint(point) {}

    void visit(const CoreComponent& component) override;

    void push() override;

    void pop() override;

    bool isAborted() const override;

    /**
     * @return The found component or nullptr if no satisfactory component could be found.
     */
    CoreComponentPtr getResult() const;

    /**
     * A condition that the resulting component and all its ancestors must satisfy.  SearchVisitor will take care of
     * ensuring all ancestors meet the condition.  Implementations should only test the component passed in as argument.
     *
     * @param component The trial component.
     * @param point The point in the coordinate space of the component.
     * @return Whether or not the condition passes for this specific node.
     */
    virtual bool universalCondition(const CoreComponent& component, const Point& point) = 0;

    /**
     * A condition that the resulting component must satisfy. Implementations should only test the component passed in
     * as argument.
     *
     * @param component The trial component.
     * @param point The point in the coordinate space of the component.
     * @return Whether or not the condition passes for this specific node.
     */
    virtual bool spotCondition(const CoreComponent& component, const Point& point) = 0;

private:
    bool             mPruneBranch = false;
    bool             mResultFound = false;
    CoreComponentPtr mPotentialResult = nullptr;
    Point            mGlobalPoint;
};

/**
 * Finds the top most touchable component at a given position.
 */
class TouchableAtPosition : public SearchVisitor {
public:
    explicit TouchableAtPosition(const Point& point) : SearchVisitor(point) {};

    bool universalCondition(const CoreComponent& component, const Point& point) override;

    bool spotCondition(const CoreComponent& component, const Point& point) override;
};

/**
 * Finds the topmost component at a given position
 */
class TopAtPosition : public SearchVisitor {
public:
    explicit TopAtPosition(const Point& point) : SearchVisitor(point) {};

    bool universalCondition(const CoreComponent& component, const Point& point) override;

    bool spotCondition(const CoreComponent& component, const Point& point) override { return true; };
};
}

#endif //APL_SEARCHVISITOR_H
