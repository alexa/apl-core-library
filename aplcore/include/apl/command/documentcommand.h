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

#ifndef _APL_DOCUMENT_COMMAND_H
#define _APL_DOCUMENT_COMMAND_H

#include "apl/command/command.h"
#include "apl/engine/context.h"

namespace apl {

class Content;
class RootContext;

/**
 * Run a large number of commands in parallel with a "finally" clause.
 */
class DocumentCommand : public Command {
public:
    static CommandPtr create(PropertyKey propertyKey,
                             const std::string& handler,
                             const RootContextPtr& rootContext) {
        return std::make_shared<DocumentCommand>(propertyKey, handler, rootContext);
    }

    DocumentCommand(PropertyKey propertyKey,
                     const std::string& handler,
                     const RootContextPtr& rootContext)
        : mPropertyKey(propertyKey),
          mHandler(handler),
          mRootContext(rootContext)
    {
    }

    unsigned long delay() const override { return 0; }
    std::string name() const override { return "DocumentCommand"; }
    ActionPtr execute(const TimersPtr& timers, bool fastMode) override;

    CommandPtr getDocumentCommand();
    ActionPtr getComponentActions(const TimersPtr& timers, bool fastMode);

    ContextPtr context();

private:
    void collectChildCommands(const ComponentPtr& base, std::vector<CommandPtr>& collection);

private:
    PropertyKey mPropertyKey;    // Which property we will extract
    std::string mHandler;
    const std::weak_ptr<RootContext> mRootContext;
};

using TransitionCommandPtr = std::shared_ptr<DocumentCommand>;

}

#endif //_APL_DOCUMENT_COMMAND_H
