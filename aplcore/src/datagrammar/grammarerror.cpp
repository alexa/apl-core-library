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

#include "apl/datagrammar/grammarerror.h"

namespace apl {

static const char *GRAMMAR_ERROR_STRING[] = {
    "invalid number format",                                 // INVALID_NUMBER_FORMAT,
    "unexpected token",                                      // UNEXPECTED_TOKEN,
    "unexpected token before eof",                           // UNEXPECTED_TOKEN_BEFORE_EOF,
    "expected an operand after a multiplicative operator",   // EXPECTED_OPERAND_AFTER_MULTIPLICATIVE,
    "expected an operand after an additive operator",        // EXPECTED_OPERAND_AFTER_ADDITIVE,
    "expected an operand after a comparison operator",       // EXPECTED_OPERAND_AFTER_COMPARISON,
    "expected an operand after an equality operator",        // EXPECTED_OPERAND_AFTER_EQUALITY,
    "expected an operand after a logical 'and' operator",    // EXPECTED_OPERAND_AFTER_LOGICAL_AND,
    "expected an operand after a logical 'or' operator",     // EXPECTED_OPERAND_AFTER_LOGICAL_OR,
    "expected an operand after a null coalescence operator", // EXPECTED_OPERAND_AFTER_NULLC,
    "expected an expression",                                // EXPECTED_EXPRESSION,
    "malformed array",                                       // MALFORMED_ARRAY,
    "unterminated single-quoted string",                     // UNTERMINATED_SS_STRING,
    "unterminated double-quoted string",                     // UNTERMINATED_DS_STRING,
    "expected KEY : VALUE map assignment",                   // EXPECTED_MAP_ASSIGNMENT,
    "missing VALUE in KEY : VALUE map assignment",           // EXPECTED_MAP_VALUE_ASSIGNMENT,
    "malformed map",                                         // MALFORMED_MAP,
    "malformed ternary expression X ? Y : Z",                // MALFORMED_TERNARY_EXPRESSION,
    "expected right parenthesis to end function call",       // EXPECTED_POSTFIX_RIGHT_PAREN,
};

static_assert(sizeof(GRAMMAR_ERROR_STRING)/sizeof(GRAMMAR_ERROR_STRING[0]) == _GRAMMAR_ERROR_COUNT,
              "Mismatched grammar error strings");

std::string
errorToString(GrammarError err)
{
    if (err >= 0 && err < GrammarError::_GRAMMAR_ERROR_COUNT)
        return GRAMMAR_ERROR_STRING[err];

    return "numerical error " + std::to_string(err);
}

} // namespace apl