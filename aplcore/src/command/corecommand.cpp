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

#include "apl/command/corecommand.h"

#include <utility>

#include "apl/command/animateitemcommand.h"
#include "apl/command/autopagecommand.h"
#include "apl/command/clearfocuscommand.h"
#include "apl/command/controlmediacommand.h"
#include "apl/command/finishcommand.h"
#include "apl/command/idlecommand.h"
#include "apl/command/insertitemcommand.h"
#include "apl/command/openurlcommand.h"
#include "apl/command/parallelcommand.h"
#include "apl/command/playmediacommand.h"
#include "apl/command/reinflatecommand.h"
#include "apl/command/removeitemcommand.h"
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
#include "apl/component/componenteventtargetwrapper.h"
#include "apl/component/selector.h"
#include "apl/engine/layoutmanager.h"
#include "apl/engine/rootcontext.h"
#include "apl/utils/dump_object.h"
#include "apl/utils/session.h"

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
        Object tmp = (def.flags & kPropEvaluated) != 0 ? evaluateNested(*context, p->second) :
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

CoreCommand::CoreCommand(const ContextPtr& context, CommandData&& commandData,
                         Properties&& properties, const CoreComponentPtr& base,
                         const std::string& parentSequencer)
    : mContext(context),
      mCommandData(std::move(commandData)),
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
}

void
CoreCommand::freeze()
{
    if (mFrozen) return;

    if (mBase) {
        mBaseId = mBase->getId();
    }
    if (mTarget) {
        mTargetId = mTarget->getId();
    }

    mMissingTargetId = mTarget && mTargetId.empty();

    // TODO: Need to see if any "better" re-evaluation required. We effectively keep copy of current
    //  context, so any bindings may be "obsolete" after rehydration.
    auto event = mContext->opt("event");

    rapidjson::Document serializer;
    auto eventValue = event.serialize(serializer.GetAllocator());
    mFrozenEventContext = rapidjson::Document();
    mFrozenEventContext.CopyFrom(eventValue, mFrozenEventContext.GetAllocator());

    mContext = nullptr;
    mBase = nullptr;
    mTarget = nullptr;
    mFrozen = true;
}

bool
CoreCommand::rehydrate(const CoreDocumentContext& context)
{
    if (!mFrozen) return true;

    mContext = context.contextPtr();
    mContext->putConstant("event", Object(std::move(mFrozenEventContext)));

    if (!mBaseId.empty()) {
        mBase = CoreComponent::cast(context.findComponentById(mBaseId));
        if (!mBase) return false;
    }
    if (!mTargetId.empty()) {
        mTarget = CoreComponent::cast(context.findComponentById(mTargetId));
        if (!mTarget) return false;
    }

    if (mMissingTargetId) {
        return false;
    }

    mFrozenEventContext = nullptr;
    mFrozen = false;
    return true;
}

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
                CONSOLE(mContext) << "Missing required property '" << cpd.second.names << "' for " << name();
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
    const auto& pds = propDefSet();
    // Check for a valid target component. Not all commands need one.
    auto cpd = pds.find(kCommandPropertyComponentId);
    if (cpd != pds.end()) {
        auto p = mProperties.find(cpd->second.names);
        std::string id;
        if (p != mProperties.end()) {
            id = evaluate(*mContext, p->second).asString();
            mTarget = Selector::resolve(id, mContext, mBase);
        }

        if (mTarget == nullptr && (cpd->second.flags & kPropRequired)) {
            // TODO: Try full inflation, we may be missing deep component inflated. Quite inefficient, especially if done
            //  in onMount, revisit.
            LOG(LogLevel::kWarn).session(mContext) << "Trying to scroll to uninflated component. Flushing pending layouts.";
            auto& lm = mContext->layoutManager();
            lm.flushLazyInflation();
            mTarget = Selector::resolve(id, mContext, mBase);
            if (mTarget == nullptr) {
                CONSOLE(mContext) << "Illegal command " << name() << " - need to specify a target componentId";
                return false;
            }
        }

        if (mTarget != nullptr) mValues.emplace(kCommandPropertyComponentId, mTarget->getUniqueId());
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
    for (const auto& it : pds) {
        if (it.second.key != kCommandPropertyComponentId) {
            auto result = calculate(it.second, context, mProperties);

            // Enumerated properties must be valid
            if (!result.first) {
                CONSOLE(context) << "Invalid enumerated property for '" << it.second.names << "'";
                return false;
            }

            mValues.emplace(it.second.key, result.second);
        }
    }

    if (DEBUG_COMMAND_VALUES) {
        for (const auto& m : mValues) {
            LOG(LogLevel::kDebug).session(mContext) << "Property: " << sCommandPropertyBimap.at(m.first) << "("
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
    {kCommandTypeInsertItem,        InsertItemCommand::create},
    {kCommandTypeRemoveItem,        RemoveItemCommand::create},
};

}
