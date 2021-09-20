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

#include "apl/datagrammar/bytecodeoptimizer.h"
#include "apl/datagrammar/bytecode.h"
#include "apl/datagrammar/functions.h"
#include "apl/datagrammar/boundsymbol.h"
#include "apl/engine/context.h"
#include "apl/utils/log.h"

namespace apl {
namespace datagrammar {

struct BasicBlock {
    int entry;  // The starting point of the basic block after code reduction
    int count;  // The number of commands in the basic block after code reduction
    int jumpEntries;   // Number of jump commands that point to this BB after code reduction

    std::string toString() const {
        return "BasicBlock<entry=" + std::to_string(entry) +
               " count=" + std::to_string(count) +
               " jumpEntries=" + std::to_string(jumpEntries) + ">";
    }
};

std::map<int, BasicBlock>
findBasicBlocks(const std::vector<ByteCodeInstruction>& commands)
{
    std::map<int, BasicBlock> result;
    result.emplace(0, BasicBlock{0});

    auto command_len = static_cast<int>(commands.size());

    // Ensure that there is a basic block that we can jump to at the end
    // This final block will never have any instructions
    result.emplace(command_len, BasicBlock{command_len, 0, 0});

    for (int pc = 0; pc < command_len; pc++) {
        auto& cmd = commands.at(pc);
        switch (cmd.type) {
            case BC_OPCODE_JUMP:
            case BC_OPCODE_JUMP_IF_FALSE_OR_POP:
            case BC_OPCODE_JUMP_IF_TRUE_OR_POP:
            case BC_OPCODE_JUMP_IF_NOT_NULL_OR_POP:
            case BC_OPCODE_POP_JUMP_IF_FALSE: {
                auto entry = pc + cmd.value + 1;
                if (entry >= 0 && entry < command_len)
                    result.emplace(entry, BasicBlock{0, 0, 0});
            }
                break;
            default:
                break;
        }
    }

    return result;
}

void
ByteCodeOptimizer::simplifyOperands()
{
    std::vector<Object> operands;

    for (auto& cmd : mByteCode.mInstructions) {
        switch (cmd.type) {
            case BC_OPCODE_LOAD_DATA:
            case BC_OPCODE_ATTRIBUTE_ACCESS:
            case BC_OPCODE_LOAD_BOUND_SYMBOL: {
                auto& object = mByteCode.mData.at(cmd.value);

                // If the value is already in an operand, use that one
                auto it = std::find(operands.begin(), operands.end(), object);
                if (it != operands.end())
                    cmd.value = std::distance(operands.begin(), it);
                else {
                    cmd.value = operands.size();
                    operands.emplace_back(std::move(object));
                }
            }
                break;
            default:
                break;
        }
    }

    mByteCode.mData = operands;
}

using UnaryFunction = Object (*)(const Object&);
using BinaryFunction = Object (*)(const Object&, const Object&);

static bool DEBUG_OPTIMIZER = false;

/**
 * Peephole optimization
 *
 * Reduction rules
 *
 *   LoadGlobal(A)                               -> Load(A)     if A non-mutable
 *   Load(A) Load(B) BinaryOp(*)                 -> Load(A*B)   if A,B known
 *   Load(A) UnaryOp(*)                          -> Load(*A)    if A known
 *   Load(A) Attribute(B)                        -> Load(A.B)   if A known
 *   Load(A) Load(B) ArrayAccess()               -> Load(A[B])  if A, B known
 *   Load(F) Load(A1)...Load(AN) CallFunction(n) -> Load(f(a1,..,an)) if all known and pure function
 */
void
ByteCodeOptimizer::simplifyOperations()
{
    auto context = mByteCode.getContext();
    assert(context);

    auto& instructions = mByteCode.mInstructions;
    auto& operands = mByteCode.mData;

    auto basicBlocks = findBasicBlocks(instructions);
    auto bbIter = basicBlocks.begin();

    std::vector<ByteCodeInstruction> output;
    bciValueType out_constants = 0;
    auto program_length = instructions.size();
    auto blockHasEnded = false;

    // Retrieve a value in the existing output stack.  Pass '-1' or '-2' generally
    auto getValueOffsetFromEnd = [&](int offset) {
        auto len = output.size();
        while (len + offset >= 0) {
            auto& cmd = output[len + offset];
            switch (cmd.type) {
                case BC_OPCODE_LOAD_CONSTANT:
                    return getConstant(static_cast<ByteCodeConstant>(cmd.value));
                case BC_OPCODE_LOAD_IMMEDIATE:
                    return Object(cmd.value);
                case BC_OPCODE_LOAD_DATA:
                    return operands.at(cmd.value);
                default:
                    LOG(LogLevel::kError) << "Illegal offset in stack at " << offset;
                    assert(false);
                    return Object::NULL_OBJECT();
            }
        }

        LOG(LogLevel::kError) << "Too many DUPLICATE commands on the stack for constant value retrieval";
        assert(false);
        return Object::NULL_OBJECT();
    };

    auto storeLoadInstruction = [&](int popCount, Object&& value) -> void {
        out_constants -= popCount;
        while (popCount-- > 0)
            output.pop_back();

        if (value.isNull()) {
            output.back() = ByteCodeInstruction{BC_OPCODE_LOAD_CONSTANT, BC_CONSTANT_NULL};
            return;
        }

        if (value.isBoolean()) {
            output.back() = ByteCodeInstruction{BC_OPCODE_LOAD_CONSTANT,
                                                value.asBoolean() ? BC_CONSTANT_TRUE : BC_CONSTANT_FALSE};
            return;
        }

        if (value.isNumber()) {
            auto db = value.asNumber();
            if (fitsInBCI(db)) {
                output.back() = ByteCodeInstruction{BC_OPCODE_LOAD_IMMEDIATE, asBCI(db)};
                return;
            }
        }

        operands.emplace_back(std::move(value));
        output.back() = ByteCodeInstruction{BC_OPCODE_LOAD_DATA, asBCI(operands.size() - 1)};
    };

    auto checkUnary = [&](int pc, UnaryFunction function) {
        if (out_constants < 1) {
            output.emplace_back(instructions[pc]);
            out_constants = 0;
        } else {
            LOG_IF(DEBUG_OPTIMIZER) << "Reducing unary function at " << pc;
            storeLoadInstruction(0, function(getValueOffsetFromEnd(-1)));
        }
    };

    // TODO: Add better constant folding/propogation to include operations like "x+0", "p/1", etc.
    auto checkBinary = [&](int pc, BinaryFunction function) {
        if (out_constants < 2) {
            output.emplace_back(instructions[pc]);
            out_constants = 0;
        } else {
            LOG_IF(DEBUG_OPTIMIZER) << "Reducing binary function at " << pc;
            // Construct a new operand to hold the result of the binary operation
            storeLoadInstruction(1, function(getValueOffsetFromEnd(-2), getValueOffsetFromEnd(-1)));
        }
    };

    auto checkCompare = [&](int pc, ByteCodeComparison bcc) {
        if (out_constants < 2) {
            output.emplace_back(instructions[pc]);
            out_constants = 0;
        } else {
            LOG_IF(DEBUG_OPTIMIZER) << "Reducing compare function at " << pc;
            storeLoadInstruction(1, CompareOp(bcc, getValueOffsetFromEnd(-2), getValueOffsetFromEnd(-1)));
        }
    };

    auto checkJumpIfOrPop = [&](int pc, bool (*f)(const Object&)) {
        auto& cmd = instructions[pc];
        auto bbKey = asBCI(pc + cmd.value + 1);

        if (out_constants < 1) {
            output.emplace_back(ByteCodeInstruction{cmd.type, bbKey});
            basicBlocks[bbKey].jumpEntries += 1;
            out_constants = 0;
        } else {
            auto value = getValueOffsetFromEnd(-1);
            if (f(value)) {
                // If TRUE, we jump ahead and keep the constant on the stack
                // JUMP index values are replaced with the start of the basic block they point to
                LOG_IF(DEBUG_OPTIMIZER) << "Reducing jump or pop TRUE " << pc;
                output.emplace_back(ByteCodeInstruction{BC_OPCODE_JUMP, asBCI(bbKey)});
                basicBlocks[bbKey].jumpEntries += 1;
                out_constants = 0;
                blockHasEnded = true;  // An unconditional JUMP ends the basic block
            } else {
                LOG_IF(DEBUG_OPTIMIZER) << "Reducing jump or pop FALSE " << pc;
                // Remove the old constant value (the "POP")
                output.pop_back();
                out_constants -= 1;
            }
        }
    };

    auto checkFunction = [&](int pc) {
        auto& cmd = instructions[pc];
        auto item_count = cmd.value + 1;
        if (out_constants >= item_count) {// Number of arguments + the function name
            auto offset = -item_count;  // Offset to the name of the function
            auto f = getValueOffsetFromEnd(offset++);
            if (f.isFunction() && f.isPure()) {
                LOG_IF(DEBUG_OPTIMIZER) << "Reducing function at " << pc;
                std::vector<Object> args(cmd.value);  // Reserve enough space
                for (int i = 0; i < cmd.value; i++)
                    args[i] = getValueOffsetFromEnd(offset++);
                storeLoadInstruction(item_count - 1, f.call(std::move(args)));
                return;
            }
        }
        output.emplace_back(cmd);
        out_constants = 0;
    };

    // Count one past the program length so that the final "actual" block is correctly
    // populated.
    for (int pc = 0; pc <= program_length ; pc++) {
        if (bbIter != basicBlocks.end() && std::next(bbIter)->first == pc) {
            // Store how many instructions were actually stored in this block
            bbIter->second.count = output.size() - bbIter->second.entry;
            bbIter++;
            bbIter->second.entry = output.size();  // Store the new entry point
            out_constants = 0;
            blockHasEnded = false;
        }

        if (pc == program_length)
            break;

        if (blockHasEnded)
            continue;

        auto& cmd = instructions.at(pc);
        switch (cmd.type) {
            case BC_OPCODE_NOP:  // Don't copy over NOP instructions
                break;
            case BC_OPCODE_CALL_FUNCTION:
                checkFunction(pc);
                break;
            case BC_OPCODE_LOAD_CONSTANT:
                output.emplace_back(cmd);
                out_constants++;
                break;
            case BC_OPCODE_LOAD_IMMEDIATE:
                output.emplace_back(cmd);
                out_constants++;
                break;
            case BC_OPCODE_LOAD_DATA:
                output.emplace_back(cmd);
                out_constants++;
                break;
            case BC_OPCODE_LOAD_BOUND_SYMBOL:
                output.emplace_back(cmd);
                out_constants = 0;
                break;
            case BC_OPCODE_ATTRIBUTE_ACCESS:   // Attributes may be replaced if they refer to an operand
                if (out_constants > 0) {
                    LOG_IF(DEBUG_OPTIMIZER) << "Load attribute replaced with an operand " << pc;
                    operands.emplace_back(CalcFieldAccess(getValueOffsetFromEnd(-1), operands.at(cmd.value)));
                    output.back() = ByteCodeInstruction{BC_OPCODE_LOAD_DATA, asBCI(operands.size() - 1)};
                } else {
                    output.emplace_back(cmd);
                    out_constants = 0;
                }
                break;
            case BC_OPCODE_ARRAY_ACCESS:
                checkBinary(pc, CalcArrayAccess);
                break;
            case BC_OPCODE_UNARY_PLUS:
                checkUnary(pc, CalculateUnaryPlus);
                break;
            case BC_OPCODE_UNARY_MINUS:
                checkUnary(pc, CalculateUnaryMinus);
                break;
            case BC_OPCODE_UNARY_NOT:
                checkUnary(pc, CalculateUnaryNot);
                break;
            case BC_OPCODE_BINARY_MULTIPLY:
                checkBinary(pc, CalculateMultiply);
                break;
            case BC_OPCODE_BINARY_DIVIDE:
                checkBinary(pc, CalculateDivide);
                break;
            case BC_OPCODE_BINARY_REMAINDER:
                checkBinary(pc, CalculateRemainder);
                break;
            case BC_OPCODE_BINARY_ADD:
                checkBinary(pc, CalculateAdd);
                break;
            case BC_OPCODE_BINARY_SUBTRACT:
                checkBinary(pc, CalculateSubtract);
                break;
            case BC_OPCODE_COMPARE_OP:
                checkCompare(pc, static_cast<ByteCodeComparison>(cmd.value));
                break;
            case BC_OPCODE_JUMP:
                output.emplace_back(ByteCodeInstruction{BC_OPCODE_JUMP, asBCI(pc + cmd.value + 1)});
                basicBlocks[pc + cmd.value + 1].jumpEntries += 1;
                out_constants = 0;
                blockHasEnded = true;  // An unconditional JUMP ends the basic block
                break;
            case BC_OPCODE_JUMP_IF_FALSE_OR_POP:
                checkJumpIfOrPop(pc, [](const Object& object) { return !object.truthy(); });
                break;
            case BC_OPCODE_JUMP_IF_TRUE_OR_POP:
                checkJumpIfOrPop(pc, [](const Object& object) { return object.truthy(); });
                break;
            case BC_OPCODE_JUMP_IF_NOT_NULL_OR_POP:
                checkJumpIfOrPop(pc, [](const Object& object) { return !object.isNull(); });
                break;
            case BC_OPCODE_POP_JUMP_IF_FALSE:
                if (out_constants > 0) {
                    if (!getValueOffsetFromEnd(-1).truthy()) {
                        LOG_IF(DEBUG_OPTIMIZER) << "PopJumpIfFalse replaced by JUMP " << pc;
                        output.pop_back();
                        output.emplace_back(ByteCodeInstruction{BC_OPCODE_JUMP, asBCI(pc + cmd.value + 1)});
                        basicBlocks[pc + cmd.value + 1].jumpEntries += 1;
                        out_constants = 0;
                        blockHasEnded = true;  // An unconditional JUMP ends the basic block
                    } else {
                        LOG_IF(DEBUG_OPTIMIZER) << "PopJumpIfFalse replaced by POP " << pc;
                        output.pop_back();
                        out_constants -= 1;
                    }
                } else {
                    output.emplace_back(ByteCodeInstruction{BC_OPCODE_POP_JUMP_IF_FALSE, asBCI(pc + cmd.value + 1)});
                    basicBlocks[pc + cmd.value + 1].jumpEntries += 1;
                    out_constants = 0;
                }
                break;
            case BC_OPCODE_MERGE_STRING:
                LOG_IF(DEBUG_OPTIMIZER) << "MergeString " << out_constants << " cmd.value=" << (int) cmd.value;
                if (out_constants < cmd.value) {
                    output.emplace_back(cmd);
                    out_constants = 0;
                } else {
                    auto result = getValueOffsetFromEnd(-1);
                    for (int i = 2; i <= cmd.value; i++)
                        result = MergeOp(getValueOffsetFromEnd(-i), result);
                    storeLoadInstruction(cmd.value - 1, std::move(result));
                }
                break;
            case BC_OPCODE_APPEND_ARRAY:
            case BC_OPCODE_APPEND_MAP:
                output.emplace_back(cmd);
                out_constants = 0;
                break;
        }
    }

    if (DEBUG_OPTIMIZER) {
        LOG(LogLevel::kDebug) << "Basic blocks located at: ";
        for (const auto& m : basicBlocks)
            LOG(LogLevel::kDebug) << m.first << ": " << m.second.toString();
    }

    LOG_IF(DEBUG_OPTIMIZER) << "Scanning for dead code blocks";

    // Dead code removal - remove blocks with no entry points.
    int stripped = 0;  // Count number of lines removed (use this to shift following block entry points

    for (auto it = basicBlocks.begin(); it != basicBlocks.end(); it++) {
        it->second.entry -= stripped; // Update the entry point of this block

        // Check if the last instruction from the preceding block was a JUMP to this block
        auto fallInto = true;   // Assume we fall through (that's the most common case)
        auto pc = it->second.entry - 1;

        if (pc >= 0) {
            auto& cmd = output.at(pc);
            if (cmd.type == BC_OPCODE_JUMP) {
                fallInto = cmd.value == it->first;
                if (fallInto) {
                    LOG_IF(DEBUG_OPTIMIZER) << "Removing unneeded JUMP at " << pc;
                    output.erase(output.begin() + pc); // Erase the JUMP
                    stripped++;
                    it->second.entry--;                // Shorten our entry point
                    auto it2 = std::prev(it);
                    while (it2->second.entry > pc) { // Update entry points in intermediate blocks
                        assert(it2->second.count == 0);  // These MUST be empty
                        it2->second.entry--;
                        it2 = std::prev(it2);
                    }
                    it2->second.count--; // it2 points to the block that contained the jump.
                }
            }
        }

        if (!fallInto && it->second.jumpEntries == 0) {  // Remove this block; there is no entry point
            LOG_IF(DEBUG_OPTIMIZER) << "Removing unused block at " << it->second.entry << " old=" << it->first;
            stripped += it->second.count;
            output.erase(output.begin() + it->second.entry,
                         output.begin() + it->second.entry + it->second.count);
            it->second.count = 0;
        }
    }

    LOG_IF(DEBUG_OPTIMIZER) << "Fixing up jump pointers";

    // The JUMP pointers are currently the indices of basic blocks.  Replace these with proper offsets.
    for (int pc = 0; pc < output.size(); pc++) {
        auto& cmd = output.at(pc);
        switch (cmd.type) {
            case BC_OPCODE_JUMP:
            case BC_OPCODE_JUMP_IF_FALSE_OR_POP:
            case BC_OPCODE_JUMP_IF_TRUE_OR_POP:
            case BC_OPCODE_JUMP_IF_NOT_NULL_OR_POP:
            case BC_OPCODE_POP_JUMP_IF_FALSE:
                cmd.value = basicBlocks[cmd.value].entry - pc - 1;
                break;

            default:
                break;
        }
    }

    instructions = output;
}

ByteCodeOptimizer::ByteCodeOptimizer(ByteCode &byteCode)
    : mByteCode(byteCode)
{
}

void
ByteCodeOptimizer::optimize(ByteCode& byteCode)
{
    if (!byteCode.mInstructions.empty()) {
        ByteCodeOptimizer bco(byteCode);
        bco.simplifyOperations();
        bco.simplifyOperands();
    }
}


} // namespace datagrammar
} // namespace apl