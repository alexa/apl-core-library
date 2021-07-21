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

#include <utility>

#include "apl/command/corecommand.h"
#include "apl/component/componenteventtargetwrapper.h"

#include "apl/utils/dump_object.h"
#include "apl/utils/session.h"

#include "apl/command/animateitemcommand.h"
#include "apl/command/autopagecommand.h"
#include "apl/command/clearfocuscommand.h"
#include "apl/command/controlmediacommand.h"
#include "apl/command/idlecommand.h"
#include "apl/command/openurlcommand.h"
#include "apl/command/parallelcommand.h"
#include "apl/command/playmediacommand.h"
#include "apl/command/scrollcommand.h"
#include "apl/command/scrolltocomponentcommand.h"
#include "apl/command/scrolltoindexcommand.h"
#include "apl/command/selectcommand.h"
#include "apl/command/sendeventcommand.h"
#include "apl/command/sequentialcommand.h"
#include "apl/command/setfocuscommand.h"
#include "apl/command/setpagecommand.h"
#include "apl/command/setstatecommand.h"
#include "apl/command/setvaluecommand.h"
#include "apl/command/speakitemcommand.h"
#include "apl/command/speaklistcommand.h"
#include "apl/command/finishcommand.h"
#include "apl/command/reinflatecommand.h"

#include "apl/engine/layoutmanager.h"

namespace apl {

const static bool DEBUG_COMMAND_VALUES = false;

/*****************************************************************/

/**
 * Calculate a single property based on a command property definition
 * @param def The command property definition
 * @param context The evaluation context
 * @param properties All properties assigned to this command
 * @return A pair consisting of true/false if the property was well-formed followed by the value of the property.
 */
static std::pair<bool, Object>
calculate(const CommandPropDef& def,
          const ContextPtr& context,
          const Properties& properties)
{
    auto p = properties.find(def.names);

    if (p != properties.end()) {
        Object tmp = (def.flags & kPropEvaluated) != 0 ?
            evaluateRecursive(*context, p->second) :
            evaluate(*context, p->second);

        if (def.map) {
            int value = def.map->get(tmp.asString(), -1);
            if (value == -1)
                return {false, def.defvalue};
            return {true, value};
        } else {
            return {true, def.func(*context, tmp)};
        }
    }

    return {true, def.defvalue};
}

/*************************************************************/

CoreCommand::CoreCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base,
        const std::string& parentSequencer)
    : mContext(context),
      mProperties(std::move(properties)),
      mBase(base),
      mTarget(base),
      mScreenLock(mProperties.asBoolean(*context, "screenLock", false)),
      mSequencer(mProperties.asString(*context, "sequencer", parentSequencer.c_str()))
{
    int delay = mProperties.asNumber(*context, "delay", 0);
    mDelay = (delay > 0 ? delay : 0);
    // Store following for debuggers
    mValues.emplace(kCommandPropertyDelay, mDelay);
    mValues.emplace(kCommandPropertyScreenLock, mScreenLock);
    mValues.emplace(kCommandPropertySequencer, mSequencer);
}

const CommandPropDefSet&
CoreCommand::propDefSet() const
{
    static CommandPropDefSet sCommonProperties = CommandPropDefSet()
        .add({
                 {kCommandPropertyScreenLock, false, asBoolean},
                 {kCommandPropertySequencer,  "",    asString },
             });

    return sCommonProperties;
};

std::string
CoreCommand::name() const
{
    return sCommandNameBimap.at(type());
}

void
CoreCommand::prepare()
{
    if (mScreenLock) {
        context()->takeScreenLock();
    }
}

void
CoreCommand::complete()
{
    if (mScreenLock) {
        context()->releaseScreenLock();
    }
}

/**
 * Validate that all required properties are present.  Run this when you first create
 * the command.  If it returns false, discard the command.
 * @return True if all required properties are present
 */
bool
CoreCommand::validate()
{
    for (const auto& cpd : propDefSet()) {
        if ((cpd.second.flags & kPropRequired) != 0) {
            // Implicit Id properties are allowed if we have a base component
            if ((cpd.second.flags & kPropId) != 0 && mBase != nullptr)
                continue;

            auto p = mProperties.find(cpd.second.names);
            if (p == mProperties.end()) {
                CONSOLE_CTP(mContext) << "Missing required property '" << cpd.second.names << "' for " << name();
                return false;
            }
        }
    }

    return true;
}


/**
 * Calculate all of the values and store them for later use.
 * This method should be run AFTER the command delay and just BEFORE executing the command.
 * @return True if this command is okay; false if it is invalid
 */
bool
CoreCommand::calculateProperties()
{
    // Check for a valid target component. Not all commands need one.
    auto cpd = propDefSet().find(kCommandPropertyComponentId);
    if (cpd != propDefSet().end()) {
        auto p = mProperties.find(cpd->second.names);
        std::string id;
        if (p != mProperties.end()) {
            id = evaluate(*mContext, p->second).asString();
            mTarget = std::dynamic_pointer_cast<CoreComponent>(mContext->findComponentById(id));
        }

        if (mTarget == nullptr) {
            // TODO: Try full inflation, we may be missing deep component inflated. Quite inefficient, especially if done
            //  in onMount, revisit.
            auto& lm = mContext->layoutManager();
            lm.flushLazyInflation();
            mTarget = std::dynamic_pointer_cast<CoreComponent>(mContext->findComponentById(id));
            if (mTarget == nullptr) {
                CONSOLE_CTP(mContext) << "Illegal command - need to specify a target componentId";
                return false;
            }
        }

        mValues.emplace(kCommandPropertyComponentId, mTarget->getUniqueId());
    }

    // When we have a target component, we need to update the context "event" property to include
    // an "event.target" element.  To avoid modifying the original context, we copy the properties from the
    // old "event" property into a new ObjectMap, add a "target" property to that object map, and then
    // set the new object map as "event" property of the new context.
    ContextPtr context = mContext;
    if (mTarget) {
        context = Context::createFromParent(mContext);
        auto event = mContext->opt("event");
        assert(event.isMap());
        auto map = std::make_shared<ObjectMap>(event.getMap());  // Copy out the existing event
        map->emplace("target", Object(ComponentEventTargetWrapper::create(mTarget)));
        context->putConstant("event", Object(std::move(map)));
    }

    // Evaluate all of the properties, including componentId (we store it for the debugger)
    for (const auto& it : propDefSet()) {
        if (it.second.key != kCommandPropertyComponentId) {
            auto result = calculate(it.second, context, mProperties);

            // Enumerated properties must be valid
            if (!result.first) {
                CONSOLE_CTP(context) << "Invalid enumerated property for '" << it.second.names << "'";
                return false;
            }

            mValues.emplace(it.second.key, result.second);
        }
    }

    if (DEBUG_COMMAND_VALUES) {
        for (const auto& m : mValues) {
            LOG(LogLevel::kDebug) << "Property: " << sCommandPropertyBimap.at(m.first) << "("
                      << m.first << ")";
            DumpVisitor::dump(m.second);
        }
    }

    return true;
}

/*************************************************************/

std::map<int, CommandCreateFunc> sCommandCreatorMap = {
    {kCommandTypeAutoPage,          AutoPageCommand::create},
    {kCommandTypeControlMedia,      ControlMediaCommand::create},
    {kCommandTypeIdle,              IdleCommand::create},
    {kCommandTypeOpenURL,           OpenURLCommand::create},
    {kCommandTypeParallel,          ParallelCommand::create},
    {kCommandTypePlayMedia,         PlayMediaCommand::create},
    {kCommandTypeScroll,            ScrollCommand::create},
    {kCommandTypeScrollToIndex,     ScrollToIndexCommand::create},
    {kCommandTypeScrollToComponent, ScrollToComponentCommand::create},
    {kCommandTypeSelect,            SelectCommand::create},
    {kCommandTypeSendEvent,         SendEventCommand::create},
    {kCommandTypeSequential,        SequentialCommand::create},
    {kCommandTypeSetPage,           SetPageCommand::create},
    {kCommandTypeSetState,          SetStateCommand::create},
    {kCommandTypeSetValue,          SetValueCommand::create},
    {kCommandTypeSpeakItem,         SpeakItemCommand::create},
    {kCommandTypeSpeakList,         SpeakListCommand::create},
    {kCommandTypeAnimateItem,       AnimateItemCommand::create},
    {kCommandTypeSetFocus,          SetFocusCommand::create},
    {kCommandTypeClearFocus,        ClearFocusCommand::create},
    {kCommandTypeFinish,            FinishCommand::create},
    {kCommandTypeReinflate,         ReinflateCommand::create},
};

}
