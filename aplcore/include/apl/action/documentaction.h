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

#ifndef _APL_DOCUMENT_ACTION_H
#define _APL_DOCUMENT_ACTION_H

#include "apl/action/action.h"
#include "apl/primitives/object.h"

namespace apl {

class DocumentCommand;

/**
 * Execute an aggregate command that includes a fail case
 */
class DocumentAction : public Action {
public:
    static std::shared_ptr<DocumentAction> make(const TimersPtr& timers,
                                                 const std::shared_ptr<DocumentCommand>& command,
                                                 bool fastMode);

    DocumentAction(const TimersPtr& timers,
                    const std::shared_ptr<DocumentCommand>& command,
                    bool fastMode);

private:
    void start();
    void advance();

    std::shared_ptr<DocumentCommand> mCommand;
    ActionPtr mCurrentAction;
    bool mFastMode;
    bool mStateFinally;
};


} // namespace apl

#endif //_APL_DOCUMENT_ACTION_H
