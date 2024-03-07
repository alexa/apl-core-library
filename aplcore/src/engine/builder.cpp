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
 *
 * Construct a virtual DOM hierarchy
 */

#include "apl/engine/builder.h"

#include "apl/component/containercomponent.h"
#include "apl/component/edittextcomponent.h"
#include "apl/component/framecomponent.h"
#include "apl/component/gridsequencecomponent.h"
#include "apl/component/hostcomponent.h"
#include "apl/component/imagecomponent.h"
#include "apl/component/pagercomponent.h"
#include "apl/component/scrollviewcomponent.h"
#include "apl/component/sequencecomponent.h"
#include "apl/component/textcomponent.h"
#include "apl/component/touchwrappercomponent.h"
#include "apl/component/vectorgraphiccomponent.h"
#include "apl/component/videocomponent.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/arrayify.h"
#include "apl/engine/binding.h"
#include "apl/engine/context.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/parameterarray.h"
#include "apl/engine/properties.h"
#include "apl/engine/typeddependant.h"
#include "apl/extension/extensioncomponent.h"
#include "apl/extension/extensionmanager.h"
#include "apl/livedata/layoutrebuilder.h"
#include "apl/livedata/livearray.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/primitives/object.h"
#include "apl/time/sequencer.h"
#include "apl/utils/identifier.h"
#include "apl/utils/log.h"
#include "apl/utils/path.h"
#include "apl/utils/session.h"
#include "apl/utils/tracing.h"

namespace apl {

const bool DEBUG_BUILDER = false;

static const std::map<std::string, MakeComponentFunc> sComponentMap = {
    {"Container",     ContainerComponent::create},
    {"Text",          TextComponent::create},
    {"Image",         ImageComponent::create},
    {"ScrollView",    ScrollViewComponent::create},
    {"EditText",      EditTextComponent::create},
    {"Frame",         FrameComponent::create},
    {"Sequence",      SequenceComponent::create},
    {"GridSequence",  GridSequenceComponent::create},
    {"TouchWrapper",  TouchWrapperComponent::create},
    {"Pager",         PagerComponent::create},
    {"VectorGraphic", VectorGraphicComponent::create},
    {"Video",         VideoComponent::create},
    {"Host",          HostComponent::create}
};


class BindingChangeImpl : public BindingChange {
public:
    static std::shared_ptr<BindingChangeImpl> create(Object&& commands)
    {
        return std::make_shared<BindingChangeImpl>(std::move(commands));
    }

    explicit BindingChangeImpl(Object&& commands) : BindingChange(std::move(commands)) {}

    void setComponent(const CoreComponentPtr& component) { mComponent = component; }

    void execute(const Object& value, const Object& previous) override {
        auto component = mComponent.lock();
        if (component) {
            auto context = component->createEventContext("Change",
                                                         std::make_shared<ObjectMap>(ObjectMap{{"current",  value},
                                                                                               {"previous", previous}}));
            component->getContext()->sequencer().executeCommands(commands(),
                                                                 context,
                                                                 component->shared_from_corecomponent(),
                                                                 true);
        }
    }

private:
    std::weak_ptr<CoreComponent> mComponent;
};

void
Builder::populateSingleChildLayout(const ContextPtr& context,
                                   const Object& item,
                                   const CoreComponentPtr& layout,
                                   const Path& path,
                                   bool fullBuild,
                                   bool useDirtyFlag)
{
    LOG_IF(DEBUG_BUILDER).session(context) << "call";
    APL_TRACE_BLOCK("Builder:populateSingleChildLayout");
    auto child = expandSingleComponentFromArray(context,
                                                arrayifyProperty(*context, item, "item", "items"),
                                                Properties(),
                                                layout,
                                                path.addProperty(item, "item", "items"),
                                                fullBuild,
                                                useDirtyFlag);
    layout->appendChild(child, useDirtyFlag);
}

void
Builder::populateLayoutComponent(const ContextPtr& context,
                                 const Object& item,
                                 const CoreComponentPtr& layout,
                                 const Path& path,
                                 bool fullBuild,
                                 bool useDirtyFlag)
{
    LOG_IF(DEBUG_BUILDER).session(context) << path;
    APL_TRACE_BLOCK("Builder:populateLayoutComponent");

    auto child = expandSingleComponentFromArray(context,
                                                arrayifyProperty(*context, item, "firstItem"),
                                                Properties(),
                                                layout,
                                                path.addProperty(item, "firstItem"),
                                                fullBuild,
                                                useDirtyFlag);
    bool hasFirstItem = false;
    if (child && child->isValid()) {
        hasFirstItem = true;
        layout->appendChild(child, useDirtyFlag);
    }

    bool numbered = layout->getCalculated(kPropertyNumbered).asBoolean();
    int ordinal = 1;
    int index = 0;

    std::shared_ptr<LayoutRebuilder> layoutBuilder = nullptr;  // Reserve space for now.  In the future, move all logic in

    const auto items = arrayifyProperty(*context, item, "item", "items");
    if (!items.empty()) {
        auto childPath = path.addProperty(item, "item", "items");
        auto data = arrayifyPropertyAsObject(*context, item, "data");

        auto liveData = data.getLiveDataObject();
        if (liveData && liveData->asArray()) {
            layoutBuilder = LayoutRebuilder::create(context, layout, mOld, liveData->asArray(), items, childPath, numbered);
            layoutBuilder->build(useDirtyFlag);
        }
        else {
            auto dataItems = evaluateNested(*context, data);
            if (!dataItems.empty()) {
                LOG_IF(DEBUG_BUILDER).session(context) << "data size=" << dataItems.size();

                // Transform data into LiveData and use rebuilder to have more control over its content.
                auto rawArray = ObjectArray();
                for (const auto& dataItem : dataItems.getArray()) {
                    rawArray.emplace_back(dataItem);
                }
                liveData = LiveDataObject::create(
                        LiveArray::create(std::move(rawArray)),
                        context,
                        "__data" + layout->getUniqueId());

                layoutBuilder = LayoutRebuilder::create(context, layout, mOld, liveData->asArray(), items, childPath, numbered);
                layoutBuilder->build(useDirtyFlag);
            }
            else {
                LOG_IF(DEBUG_BUILDER).session(context) << "items size=" << items.size();
                auto length = items.size();
                for (int i = 0; i < length; i++) {
                    const auto& element = items.at(i);
                    auto childContext = Context::createFromParent(context);
                    childContext->putConstant("__source", "index");
                    childContext->putConstant("index", index);
                    childContext->putConstant("length", length);
                    if (numbered)
                        childContext->putConstant("ordinal", ordinal);

                    // TODO: Numbered, spacing, ordinal changes
                    child = expandSingleComponentFromArray(childContext,
                                                           arrayify(*context, element),
                                                           Properties(),
                                                           layout,
                                                           childPath.addIndex(i),
                                                           fullBuild,
                                                           useDirtyFlag);
                    // TODO: Full or not full here?
                    if (child && child->isValid()) {
                        layout->appendChild(child, useDirtyFlag);
                        index++;

                        if (numbered) {
                            int numbering = child->getCalculated(kPropertyNumbering).getInteger();
                            if (numbering == kNumberingNormal) ordinal++;
                            else if (numbering == kNumberingReset) ordinal = 1;
                        }
                    }
                }
            }
        }
    }

    child = expandSingleComponentFromArray(context,
                                           arrayifyProperty(*context, item, "lastItem"),
                                           Properties(),
                                           layout,
                                           path.addProperty(item, "lastItem"),
                                           fullBuild,
                                           useDirtyFlag);

    bool hasLastItem = false;
    if (child && child->isValid()) {
        hasLastItem = true;
        layout->appendChild(child, useDirtyFlag);
    }

    // Chance to get final child dependent set-up before actual layout happened.
    layout->finalizePopulate();

    if (layoutBuilder)
        layoutBuilder->setFirstLast(hasFirstItem, hasLastItem);
}

MakeComponentFunc
Builder::findComponentBuilderFunc(const ContextPtr& context, const std::string &type) {
    auto method = sComponentMap.find(type);
    if (method != sComponentMap.end()) {
        return method->second;
    }

    return context->extensionManager().findAndCreateExtensionComponent(type);
}

/**
 * Expand a single component or layout by type.
 *
 * @param context Current data-binding context
 * @param item The JSON-definition of the item to expand.
 * @param properties The user-specified properties for this component
 * @param parent The parent component of this component
 * @param path The path of this component
 * @param fullBuild Build full tree.
 * @param useDirtyFlag true to notify runtime about changes with dirty properties
 */
CoreComponentPtr
Builder::expandSingleComponent(const ContextPtr& context,
                               const Object& item,
                               Properties&& properties,
                               const CoreComponentPtr& parent,
                               const Path& path,
                               bool fullBuild,
                               bool useDirtyFlag)
{
    LOG_IF(DEBUG_BUILDER).session(context) << path.toString();
    APL_TRACE_BLOCK("Builder:expandSingleComponent");

    std::string type = propertyAsString(*context, item, "type");
    if (type.empty()) {
        CONSOLE(context) << "Invalid type in component";
        return nullptr;
    }

    if (auto method = findComponentBuilderFunc(context, type)) {
        LOG_IF(DEBUG_BUILDER).session(context) << "Expanding primitive " << type;

        // Copy the items into the properties map.
        properties.emplace(item);

        // Create a new context and fill out the binding
        ContextPtr expanded = Context::createFromParent(context);
        expanded->putConstant("__source", "component");
        expanded->putConstant("__name", type);

        auto bindingChangeList = attachBindings(expanded, item, BindingChangeImpl::create);

        // Construct the component
        CoreComponentPtr component = CoreComponentPtr(method(expanded, std::move(properties), path));

        bool releaseAndReturnNull = false;
        if (!component || !component->isValid()) {
            CONSOLE(context) << "Unable to inflate component";
            releaseAndReturnNull = true;
        } else if (parent == nullptr && component->isDisallowed()) {
            CONSOLE(context) << "Component type " << component->name() << " is top-level and disallowed, not inflating" ;
            releaseAndReturnNull = true;
        }

        if (releaseAndReturnNull) {
            if (component)
                component->release();
            return nullptr;
        }

        // Assign the component to any bindings that had a "onChange" method
        for (const auto& m : bindingChangeList)
            std::static_pointer_cast<BindingChangeImpl>(m)->setComponent(component);

        CoreComponentPtr oldComponent;
        if(mOld) {
            oldComponent = CoreComponent::cast(
                mOld->findComponentById(component->getId(), false));
            copyPreservedBindings(component, oldComponent);
        }

        if (fullBuild) {
            if (component->singleChild()) {
                populateSingleChildLayout(expanded, item, component, path, true, useDirtyFlag);
            } else if (component->multiChild()) {
                populateLayoutComponent(expanded, item, component, path, true, useDirtyFlag);
            }
        } else {
            expanded->putConstant("_item", item);
        }

        if(oldComponent) {
            // Copy preserved properties other than local bindings
            copyPreservedProperties(component, oldComponent);
        }

        LOG_IF(DEBUG_BUILDER).session(context) << "Returning component " << *component;
        return component;
    }

    LOG_IF(DEBUG_BUILDER).session(context) << "Expanding layout '" << type << "'";
    auto resource = context->getLayout(type);
    if (!resource.empty()) {
        properties.emplace(item);
        return expandLayout(type, context, properties, resource.json(), parent, resource.path(), fullBuild, useDirtyFlag);
    }

    CONSOLE(context) << "Unable to find layout or component '" << type << "'";
    return nullptr;
}

/**
 * Expand a single component from a "when" list of possible components
 *
 * @param context Current data-binding context
 * @param items A vector of the components to expand
 * @param properties The user-specified properties for this component
 * @param parent The parent component of this component
 * @param path The path description of this component
 * @param fullBuild Build full tree.
 * @param useDirtyFlag true to notify runtime about changes with dirty properties
 */
CoreComponentPtr
Builder::expandSingleComponentFromArray(const ContextPtr& context,
                                        const std::vector<Object>& items,
                                        Properties&& properties,
                                        const CoreComponentPtr& parent,
                                        const Path& path,
                                        bool fullBuild,
                                        bool useDirtyFlag)
{
    LOG_IF(DEBUG_BUILDER).session(context) << path;
    for (int index = 0; index < items.size(); index++) {
        const auto& item = items.at(index);
        if (!item.isMap())
            continue;

        if (propertyAsBoolean(*context, item, "when", true)) {
            return expandSingleComponent(context, item, std::move(properties), parent, path.addIndex(index), fullBuild, useDirtyFlag);
        }
    }

    return nullptr;
}

/**
 * Expand a layout defined either as the "mainTemplate" or a named layout
 * from the "layouts" section of a package.
 *
 * @param name The name of the layout
 * @param context Current data-binding context.
 * @param properties The user-specified properties for this layout.
 * @param layout The JSON definition of the layout object.
 * @param parent The parent component of this layout.
 * @param fullBuild Build full tree.
 * @param useDirtyFlag true to notify runtime about changes with dirty properties
 */
CoreComponentPtr
Builder::expandLayout(const std::string& name,
                      const ContextPtr& context,
                      Properties& properties,
                      const rapidjson::Value& layout,
                      const CoreComponentPtr& parent,
                      const Path& path,
                      bool fullBuild,
                      bool useDirtyFlag)
{
    LOG_IF(DEBUG_BUILDER).session(context) << path;
    if (!layout.IsObject()) {
        std::string errorMessage = "Layout inflation for one of the components failed. Path: " + path.toString();
        CONSOLE(context) << errorMessage;
        return nullptr;
    }
    APL_TRACE_BLOCK("Builder:expandLayout");

    // Build a new context for this layout.
    ContextPtr cptr = Context::createFromParent(context);
    cptr->putConstant("__source", "layout");
    cptr->putConstant("__name", name);

    // Add each parameter to the context.  It's either going to come from
    // a property or its default value.  This will remove the matching property from
    // the property map.
    ParameterArray params(layout);
    for (const auto& param : params) {
        LOG_IF(DEBUG_BUILDER).session(context) << "Parsing parameter: " << param.name;
        properties.addToContext(cptr, param, true);
    }

    auto bindingChangeList = attachBindings(cptr, layout, BindingChangeImpl::create);

    if (DEBUG_BUILDER) {
        for (ConstContextPtr p = cptr; p; p = p->parent()) {
            for (const auto& m : *p)
                LOG(LogLevel::kDebug).session(context) << m.first << ": " << m.second;
        }
    }
    auto component = expandSingleComponentFromArray(cptr,
                                                    arrayifyProperty(*cptr, layout, "item", "items"),
                                                    std::move(properties),
                                                    parent,
                                                    path.addProperty(layout, "item", "items"),
                                                    fullBuild,
                                                    useDirtyFlag);
    if (component) {
        // Assign the component to any bindings that had a "onChange" method
        for (const auto& m : bindingChangeList)
            std::static_pointer_cast<BindingChangeImpl>(m)->setComponent(component);
    }
    return component;
}

/**
 * Copy preserved properties and local bindings from one component to another
 *
 * @param newComponent The component to copy properties to
 * @param originalComponent The component to copy properties from
 * @param copyLocalBindings if true, copy local bindings, true by default
 * @param copyComponentProperties if true, copy local bindings, true by default
 */
static inline void copyPreserved(const CoreComponentPtr& newComponent, const CoreComponentPtr& originalComponent,
                                 bool copyLocalBindings, bool copyComponentProperties)
{
    if(!(copyLocalBindings || copyComponentProperties))
        return;

    if (!originalComponent || !newComponent)
        return;

    const auto& preserved = newComponent->getCalculated(kPropertyPreserve).getArray();
    if (preserved.empty())
        return;

    // We have an original component and we have at least one property that should be restored
    auto originalContext = originalComponent->getContext();
    for (const auto& m : preserved) {
        auto property = m.asString();
        bool isPropLocal = originalContext->hasLocal(property);
        if(!isPropLocal && copyComponentProperties) {
            newComponent->setProperty(property, originalComponent->getProperty(property));
        } else if(copyLocalBindings && isPropLocal) {
            auto oldRef = originalContext->find(property);
            if(!oldRef.empty())
                newComponent->setProperty(property, oldRef.object().value());
        }
    }
}

/**
 * Copy preserved local bindings from one component to other.
 *
 * @param newComponent The component to copy bindings to.
 * @param originalComponent The component to copy bindings from.
 */
void
Builder::copyPreservedBindings(const CoreComponentPtr& newComponent, const CoreComponentPtr& originalComponent)
{
    APL_TRACE_BLOCK("Builder:copyPreservedBindings");
    LOG_IF(DEBUG_BUILDER).session(newComponent) << newComponent << " old=" << mOld;
    copyPreserved(newComponent, originalComponent, true, false);
}

/**
 * Copy preserved component properties from one component to other.
 *
 * @param newComponent The component to copy bindings to.
 * @param originalComponent The component to copy bindings from.
 */
void
Builder::copyPreservedProperties(const CoreComponentPtr& newComponent, const CoreComponentPtr& originalComponent)
{
    APL_TRACE_BLOCK("Builder:copyPreservedProperties");
    LOG_IF(DEBUG_BUILDER).session(newComponent) << newComponent << " old=" << mOld;
    copyPreserved(newComponent, originalComponent, false, true);
}

CoreComponentPtr
Builder::inflate(const ContextPtr& context,
                 Properties& mainProperties,
                 const rapidjson::Value& mainDocument)
{
    APL_TRACE_BLOCK("Builder:inflate");
    return expandLayout("mainTemplate", context, mainProperties, mainDocument, nullptr,
        Path(context->getRootConfig().getProperty(RootProperty::kTrackProvenance).getBoolean()
                                 ? std::string(Path::MAIN) + "/mainTemplate" : ""), true, false);
}

CoreComponentPtr
Builder::inflate(const ContextPtr& context,
                 const Object& component)
{
    assert(component.isMap() || component.isArray());
    if (component.isMap())
        return expandSingleComponent(context, component, Properties(), nullptr,
            Path(context->getRootConfig().getProperty(RootProperty::kTrackProvenance).getBoolean()
                                              ? "_virtual" : ""), true, false);
    else if (component.isArray())
        return expandSingleComponentFromArray(context, component.getArray(), Properties(), nullptr,
            Path(context->getRootConfig().getProperty(RootProperty::kTrackProvenance).getBoolean()
                                                       ? "_virtual" : ""), true, false);
    else
        return nullptr;
}

} // namespace apl
