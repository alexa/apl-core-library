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

#ifndef _APL_DATAGRAMMAR_FUNCTIONS_H
#define _APL_DATAGRAMMAR_FUNCTIONS_H

#include <string>
#include <vector>

namespace apl {

class Context;
class Object;

namespace datagrammar {

extern Object CalculateUnaryPlus(const Object& arg);
extern Object CalculateUnaryMinus(const Object& arg);
extern Object CalculateUnaryNot(const Object& arg);
extern Object CalculateMultiply(const Object& a, const Object& b);
extern Object CalculateDivide(const Object& a, const Object& b);
extern Object CalculateRemainder(const Object& a, const Object& b);
extern Object CalculateAdd(const Object& a, const Object& b);
extern Object CalculateSubtract(const Object& a, const Object& b);
extern Object CalcFieldAccess(const Object& a, const Object& b);
extern Object CalcArrayAccess(const Object& a, const Object& b);

extern int Compare(const Object& a, const Object& b);
extern Object MergeOp(const Object& a, const Object& b);

} // datagrammar
} // apl

#endif //_APL_DATAGRAMMAR_FUNCTIONS_H
