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

#ifndef APL_BYTE_CODE_EVALUATOR_H
#define APL_BYTE_CODE_EVALUATOR_H

#include <vector>
#include <memory>

#include "apl/datagrammar/bytecode.h"
#include "apl/primitives/boundsymbolset.h"

namespace apl {
namespace datagrammar {

/**
 * Evaluation environment for byte code.  This should only be allocated on the stack.
 * The byte code reference must remain valid for the lifetime of the evaluator.
 */
class ByteCodeEvaluator {
public:
    ByteCodeEvaluator(const ByteCode& byteCode, BoundSymbolSet *symbols, int depth);

    /**
     * Start or continue executing the byte code.
     */
    void advance();

    /**
     * @return True if the byte code has finished executing
     */
    bool isDone() const { return mState == DONE; }

    /**
     * @return The result of executing the byte code.  If the return type is void, this
     *         method will return null.
     */
    Object getResult() const;

private:
    enum State { INIT, DONE, ERROR };

    const ByteCode& mByteCode;
    std::vector<Object> mStack;
    BoundSymbolSet *mSymbols;
    int mProgramCounter = 0;
    int mEvaluationDepth;
    State mState = INIT;
};

} // namespace datagrammar
} // namespace apl

#endif //APL_BYTE_CODE_EVALUATOR_H
