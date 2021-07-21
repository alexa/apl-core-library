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

#ifndef _APL_GRAMMAR_ERROR_H
#define _APL_GRAMMAR_ERROR_H

#include <string>

namespace apl {

enum GrammarError {
    INVALID_NUMBER_FORMAT,
    UNEXPECTED_TOKEN,
    UNEXPECTED_TOKEN_BEFORE_EOF,
    EXPECTED_OPERAND_AFTER_MULTIPLICATIVE,
    EXPECTED_OPERAND_AFTER_ADDITIVE,
    EXPECTED_OPERAND_AFTER_COMPARISON,
    EXPECTED_OPERAND_AFTER_EQUALITY,
    EXPECTED_OPERAND_AFTER_LOGICAL_AND,
    EXPECTED_OPERAND_AFTER_LOGICAL_OR,
    EXPECTED_OPERAND_AFTER_NULLC,
    EXPECTED_EXPRESSION,
    MALFORMED_ARRAY,
    UNTERMINATED_SS_STRING,
    UNTERMINATED_DS_STRING,
    EXPECTED_MAP_ASSIGNMENT,
    EXPECTED_MAP_VALUE_ASSIGNMENT,
    MALFORMED_MAP,
    MALFORMED_TERNARY_EXPRESSION,
    EXPECTED_POSTFIX_RIGHT_PAREN,
    _GRAMMAR_ERROR_COUNT  // Mark end of the Grammar errors (see grammarerror.cpp)
};

extern std::string errorToString(GrammarError err);

} // namespace apl

#endif // _APL_GRAMMAR_ERROR_H
