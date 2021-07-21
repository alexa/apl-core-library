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

#ifndef _APL_DUMP_OBJECT_H
#define _APL_DUMP_OBJECT_H

#include <iomanip>

#include "apl/primitives/object.h"

namespace apl {

class DumpVisitor : public Visitor<Object> {
public:
    static void dump(const Object& object) {
        DumpVisitor dv;
        object.accept(dv);
    }

    DumpVisitor() : mIndent(0) {}

    void visit(const Object& object) {
        LOG(LogLevel::kDebug) << std::string(mIndent + 1,' ') << object;
    }

    void push() { mIndent += 2; }
    void pop() { mIndent -= 2; }

private:
    int mIndent;
};

}

#endif // _APL_DUMP_OBJECT_H
