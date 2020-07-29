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

#ifndef _APL_COMMAND_CORE_COMMAND_H
#define _APL_COMMAND_CORE_COMMAND_H

#include "apl/command/command.h"
#include "apl/utils/bimap.h"
#include "apl/primitives/objectbag.h"
#include "apl/engine/context.h"
#include "apl/component/corecomponent.h"
#include "apl/engine/event.h"
#include "apl/engine/propdef.h"

namespace apl {

extern Bimap<int, std::string> sCommandNameBimap;

using CommandCreateFunc = std::function<CommandPtr(const ContextPtr&, Properties&&, const CoreComponentPtr&, const std::string&)>;
extern std::map<int, CommandCreateFunc> sCommandCreatorMap;

class CoreCommand;
using CmdExecFunc = std::function<ActionPtr(const CoreCommand&, bool)>;

using CommandPropDef = PropDef<CommandPropertyKey, sCommandPropertyBimap>;

class CommandPropDefSet : public PropDefSet<CommandPropertyKey, sCommandPropertyBimap, CommandPropDef>
{
public:
    CommandPropDefSet() = default;
    CommandPropDefSet(const CommandPropDefSet& rhs) = default;
    CommandPropDefSet(const CommandPropDefSet& other, const std::vector<CommandPropDef>& list)
        : CommandPropDefSet(other)
    {
        add(list);
    }

    CommandPropDefSet& add(const std::vector<CommandPropDef>& list) {
        addInternal(list);
        return *this;
    }
};

/**
 * Some notes on core command
 *
 * When the core command is constructed, we expect to have a context with "event.source"
 *
 * The only property we can calculate immediately is the "delay" property.
 * We need to make sure all REQUIRED properties exist, but we don't calculate them until after the delay.
 *
 * After the delay is up, we
 * (a) insert event.XXX values for current state of event source values
 * (b) insert event.target values for the current state of the target (if it exists)
 * (c) evaluate all of the properties and put them in a property bag.
 *
 * So first, we'll need to hold onto the Properties!  Fortunately, we can change the
 * signature from the CommandFactory to simply std::move(properties)
 */
class CoreCommand : public Command {

public:
    CoreCommand(const ContextPtr& context,
                Properties&& properties,
                const CoreComponentPtr& base,
                const std::string& parentSequencer);

    virtual CommandType type() const = 0;

    unsigned long delay() const override { return mDelay; }
    std::string name() const override;
    void prepare() override;
    void complete() override;
    std::string sequencer() const override { return mSequencer; }

    Object getValue(CommandPropertyKey key) const { return mValues.at(key); }
    ContextPtr context() const { return mContext; }
    CoreComponentPtr base() const { return mBase; }
    CoreComponentPtr target() const { return mTarget; }
    const Properties& properties() const { return mProperties; }

    virtual const CommandPropDefSet& propDefSet() const;

protected:
    bool validate();
    bool calculateProperties();

protected:
    ContextPtr        mContext;
    Properties        mProperties;
    CoreComponentPtr  mBase;
    CommandBag        mValues;
    unsigned long     mDelay;
    CoreComponentPtr  mTarget;
    const bool        mScreenLock;
    std::string       mSequencer;
 };

} // namespace apl

#endif //_APL_COMMAND_CORE_COMMAND_H
