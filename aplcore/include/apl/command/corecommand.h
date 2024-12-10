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
#include "apl/component/corecomponent.h"
#include "apl/document/coredocumentcontext.h"
#include "apl/engine/context.h"
#include "apl/engine/event.h"
#include "apl/engine/propdef.h"
#include "apl/primitives/commanddata.h"
#include "apl/primitives/objectbag.h"
#include "apl/utils/bimap.h"

namespace apl {

extern Bimap<int, std::string> sCommandNameBimap;

using CommandCreateFunc = std::function<CommandPtr(
    const ContextPtr&, CommandData&&, Properties&&, const CoreComponentPtr&, const std::string&)>;
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
                CommandData&& commandData,
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
    virtual bool finishAllOnTerminate() const { return false; }

    void freeze() final;
    bool rehydrate(const CoreDocumentContext& context) final;

    const CommandData& data() const { return mCommandData; }

protected:
    bool validate();
    bool calculateProperties();

protected:
    ContextPtr        mContext;
    // Data that this command was created from and requires to operate properly.
    CommandData       mCommandData;
    Properties        mProperties;
    CoreComponentPtr  mBase;
    CommandBag        mValues;
    unsigned long     mDelay;
    CoreComponentPtr  mTarget;
    const bool        mScreenLock;
    std::string       mSequencer;

private:
    void logProperties();

private:
    std::string mBaseId;
    std::string mTargetId;
    rapidjson::Document mFrozenEventContext;
    bool mFrozen = false;
    bool mMissingTargetId = false;
};

/**
 * Template for command definition in order to avoid copying creation and constructor primitives.
 * @tparam Name Command name.
 */
template<class Name>
class TemplatedCommand : public CoreCommand {
public:
    static CommandPtr create(const ContextPtr& context,
                             CommandData&& commandData,
                             Properties&& properties,
                             const CoreComponentPtr& base,
                             const std::string& parentSequencer) {
        auto ptr = std::make_shared<Name>(context, std::move(commandData), std::move(properties), base, parentSequencer);
        return ptr->validate() ? ptr : nullptr;
    }

    TemplatedCommand<Name>(
        const ContextPtr& context,
        CommandData&& commandData,
        Properties&& properties,
        const CoreComponentPtr& base,
        const std::string& parentSequencer)
        : CoreCommand(context, std::move(commandData), std::move(properties), base, parentSequencer)
    {}
};

/// Constructor helper to use in conjunction with template above (templates can't properly define
/// constructors in C++11).
#define COMMAND_CONSTRUCTOR(NAME)           \
    NAME(                                   \
        const ContextPtr& context,          \
        CommandData&& commandData,          \
        Properties&& properties,            \
        const CoreComponentPtr& base,       \
        const std::string& parentSequencer) \
            : TemplatedCommand<NAME>(       \
              context,                      \
              std::move(commandData),       \
              std::move(properties),        \
              base,                         \
              parentSequencer)              \
    {}
} // namespace apl

#endif //_APL_COMMAND_CORE_COMMAND_H
