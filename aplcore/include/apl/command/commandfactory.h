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

#ifndef _APL_COMMANDFACTORY_H
#define _APL_COMMANDFACTORY_H

#include <memory>

#include "apl/action/action.h"
#include "apl/command/command.h"
#include "apl/engine/context.h"
#include "apl/component/corecomponent.h"
#include "apl/primitives/object.h"

namespace apl {

using CommandFunc = std::function<CommandPtr(const ContextPtr&, Properties&&, const CoreComponentPtr&, const std::string&)>;

class CoreCommand;

/**
 * The CommandFactory provides a singleton instance that is used to execute a series of commands within a context.
 *
 * This class is used by ArrayAction and SequentialAction and ultimately invoked by the sequencer.
 */
class CommandFactory {
public:
    static CommandFactory& instance();

    ActionPtr execute(const TimersPtr& timers,
                      const ContextPtr& context,
                      const Object& command,
                      const CoreComponentPtr& base,
                      bool fastMode);

    void reset();

    CommandFactory& set(const char *name, CommandFunc func);
    CommandFunc get(const char *name) const;

    CommandPtr inflate(const ContextPtr& context, const Object& command, const Properties& properties,
                      const CoreComponentPtr& base, const std::string& parentSequencer = "");

    CommandPtr inflate(const ContextPtr& context, const Object& command,
                       const CoreComponentPtr& base);

    CommandPtr inflate(const Object& command, const std::shared_ptr<const CoreCommand>& parent);
protected:
    CommandFactory() {};

private:
    CommandFactory(const CommandFactory&) = delete;
    CommandFactory& operator=(const CommandFactory&) = delete;

    CommandPtr expandMacro(const ContextPtr& context,
                          Properties& properties,
                          const rapidjson::Value& definition,
                          const CoreComponentPtr& base,
                          const std::string& parentSequencer);



    static CommandFactory *sInstance;

    std::map<std::string, CommandFunc> mCommandMap;

};


} // namespace apl

#endif // _APL_COMMANDFACTORY_H
