/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef _APL_DATAGRAMMAR_FUNCTIONS_H
#define _APL_DATAGRAMMAR_FUNCTIONS_H

#include <string>
#include <vector>

namespace apl {

class Context;
class Object;

namespace datagrammar {

// Pull these two out so we can identify them when extracting symbols from a Node.
Object ApplyArrayAccess(const std::vector<Object>& args);
Object ApplyFieldAccess(const std::vector<Object>& args);

// Use these to construct an Object which may be a node or a calculated value
extern Object UnaryPlus(std::vector<Object>&& );
extern Object UnaryMinus(std::vector<Object>&& );
extern Object UnaryNot(std::vector<Object>&& );

extern Object Multiply(std::vector<Object>&& );
extern Object Divide(std::vector<Object>&& );
extern Object Remainder(std::vector<Object>&& );
extern Object Add(std::vector<Object>&& );
extern Object Subtract(std::vector<Object>&& );

extern Object LessThan(std::vector<Object>&& );
extern Object GreaterThan(std::vector<Object>&& );
extern Object LessEqual(std::vector<Object>&& );
extern Object GreaterEqual(std::vector<Object>&& );
extern Object Equal(std::vector<Object>&& );
extern Object NotEqual(std::vector<Object>&& );
extern Object And(std::vector<Object>&& );
extern Object Or(std::vector<Object>&& );
extern Object Nullc(std::vector<Object>&& );
extern Object Ternary(std::vector<Object>&& );

extern Object Combine(std::vector<Object>&& );
extern Object Symbol(const Context&, std::vector<Object>&&, const std::string& );
extern Object FieldAccess(std::vector<Object>&& );
extern Object ArrayAccess(std::vector<Object>&& );
extern Object FunctionCall(std::vector<Object>&& );


} // datagrammar
} // apl

#endif //_APL_DATAGRAMMAR_FUNCTIONS_H
