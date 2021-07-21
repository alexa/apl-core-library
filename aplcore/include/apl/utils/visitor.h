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
 *
 * Templated visitor pattern
 */

#ifndef _APL_VISITOR_H
#define _APL_VISITOR_H

namespace apl {

/**
 * Basic visitor pattern, good for showing a hierarchy.  The visited class generally
 * implements an "accept" method and handles child recursion.  For example:
 *
 * class Tree {
 *     void accept(Visitor<Tree>& visitor) const {
 *         visitor.visit(*this);
 *         visitor.push();
 *         for (const auto& m : mChildren)
 *             m.accept(visitor);
 *         visitor.pop();
 *     }
 *
 *     std::vector<Tree> mChildren;
 * };
 *
 * A short-circuiting visitor is one that can be aborted in the middle of traversal.
 * For example, this can be used to search a hierarchy for a named component and
 * to terminate the search as soon as the component is found.
 *
 * To support a short-circuiting visitor check to see if the visitor has aborted before
 * calling accept().  For example:
 *
 * class Tree {
 *     void accept(Visitor<Tree>& visitor) const {
 *         visitor.visit(*this);
 *         visitor.push();
 *         for (auto it = mChildren.begin() ; !visitor.isAborted() && it != mChildren.end() ; it++)
 *             it->accept(visitor);
 *         visitor.pop();
 *     }
 *
 *     std::vector<Tree> mChildren;
 * };
 */
template<class T>
class Visitor
{
public:
    virtual ~Visitor() {};

    virtual void visit(const T&) {};
    virtual void push() {}
    virtual void pop() {}

    /**
     * Use when building a short-circuiting visitor.  Override this method to
     * return true when the visiting process should be short-circuited.  Please note
     * that objects supporting the visitor pattern should check this method during the
     * traversal.
     * @return True if the visit should be short-circuited.
     */
    virtual bool isAborted() const { return false; }
};

}

#endif // _APL_VISITOR_H