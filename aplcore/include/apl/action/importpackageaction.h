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

#ifndef APL_IMPORT_PACKAGE_ACTION_H
#define APL_IMPORT_PACKAGE_ACTION_H

#include "apl/action/action.h"

namespace apl {

class CoreCommand;

class ImportPackageAction : public Action {
public:
    static std::shared_ptr<ImportPackageAction> make(const TimersPtr& timers,
                                                     const std::shared_ptr<CoreCommand>& command,
                                                     ActionPtr&& startAction);

    ImportPackageAction(const TimersPtr& timers,
                        const std::shared_ptr<CoreCommand>& command,
                        ActionPtr&& startAction);

    void onLoad(const std::string& version);
    void onFail(const std::string& nameVersionSource, const std::string& errorMessage, int code);

private:
    std::shared_ptr<CoreCommand> mCommand;
    ActionPtr mCurrentAction;
};

} // namespace apl

#endif // APL_IMPORT_PACKAGE_ACTION_H
