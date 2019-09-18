/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
 * Basic visitor pattern, good for showing the hierarchy.
 */
template<class T>
struct Visitor
{
    virtual ~Visitor() {};

    virtual void visit(const T&) = 0;
    virtual void push() = 0;
    virtual void pop() = 0;
};

}

#endif // _APL_VISITOR_H