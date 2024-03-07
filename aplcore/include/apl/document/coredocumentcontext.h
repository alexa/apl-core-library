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

#ifndef _APL_CORE_DOCUMENT_CONTEXT_H
#define _APL_CORE_DOCUMENT_CONTEXT_H

#include "apl/content/configurationchange.h"
#include "apl/content/documentconfig.h"
#include "apl/document/displaystate.h"
#include "apl/document/documentcontext.h"
#include "apl/document/documentcontextdata.h"

#ifdef ALEXAEXTENSIONS
#include "apl/extension/extensionmediator.h"
#endif

namespace apl {

class EventManager;
class Metrics;
class RootConfig;
class Sequencer;
struct PointerEvent;

using LayoutCallbackFunc = std::function<void()>;

/**
 * Core implementation of rendered document API.
 */
class CoreDocumentContext : public DocumentContext, public std::enable_shared_from_this<CoreDocumentContext> {
public:
    /**
     * Construct a document context.
     * @param shared Pointer to the SharedContextData.
     * @param metrics Display metrics
     * @param content Content to display
     * @param config Configuration information
     * @return A pointer to the document context.
     */
    static CoreDocumentContextPtr create(
                                 const SharedContextDataPtr& shared,
                                 const Metrics& metrics,
                                 const ContentPtr& content,
                                 const RootConfig& config);

    /**
     * Notify core of a configuration change. Internally this method will trigger the "onConfigChange"
     * event handler in the APL document.  A common behavior in the onConfigChange event handler is to
     * send a Reinflate (kEventTypeReinflate) event.
     * @see RootContext::configurationChange
     *
     * @param change Configuration change information
     * @param embedded true if embedded document change, false otherwise.
     */
    void configurationChange(const ConfigurationChange& change, bool embedded);
    
    /**
     * Update the display state of the document. Internally this method will trigger the
     * "onDisplayStateChange" event handler in the APL document, if the display state changed.
     * @see RootContext::updateDisplayState
     *
     * @param displayState The new display state
     */
    void updateDisplayState(DisplayState displayState);

    /**
     * Reinflate this context using the internally cached configuration changes.  This will terminate any
     * existing animations, remove any events on the queue, clear the dirty components, and create a new
     * component hierarchy.  After calling this method the view host should rebuild its visual hierarchy.
     * @see RootContext::reinflate
     * @param layoutCallback Callback executed when reinflation process finished.
     *
     * @return true if successful, false otherwise.
     */
    bool reinflate(const LayoutCallbackFunc& layoutCallback);

    /**
     * Start document reinflation. Extracts relevant status and stops any current processing.
     * @param preservedSequencers output for sequencers to preserve.
     * @return pair of success boolean and old top component, if any.
     */
    std::pair<bool, CoreComponentPtr> startReinflate(std::map<std::string, ActionPtr>& preservedSequencers);

    /**
     * Finish document reinflation. Relies on #startReinflate being called beforehand.
     * @param layoutCallback Callback executed when reinflation process finished.
     * @param oldTop Old top component, if any.
     * @param preservedSequencers Sequencers that were preserved, if any.
     *
     * @return true if successful, false otherwise.
     */
    bool finishReinflate(
        const LayoutCallbackFunc& layoutCallback,
        const CoreComponentPtr& oldTop,
        const std::map<std::string, ActionPtr>& preservedSequencers);

    /**
     * Trigger a resize based on stored configuration changes.
     * @see RootContext::resize
     */
    void resize();

    /**
     * Clear any pending onMount handlers and extension handlers.
     */
    void clearPending() const;

    /**
     * Public constructor.  Use the ::create method instead.
     * @param content Processed APL content data
     * @param config Configuration information
     */
    CoreDocumentContext(
        const ContentPtr& content,
        const RootConfig& config);

    ~CoreDocumentContext() override;

    /**
     * @return The top-level context.
     */
    Context& context() const { return *mContext; }

    /**
     * @return The top-level context as a shared pointer
     */
    ContextPtr contextPtr() const { return mContext; }

    /**
     * @return The top-level component for this document
     */
    ComponentPtr topComponent();

    /**
     * @return The top-level context with payload binding. This context is used when executing document-level
     *         commands.
     */
    ContextPtr payloadContext() const;

    /**
     * Invoke an extension event handler.
     * @param uri The URI of the custom document handler
     * @param name The name of the handler to invoke
     * @param data The data to associate with the handler
     * @param fastMode If true, this handler will be invoked in fast mode
     * @param resourceId handle associated with extension component if present
     * @return An ActionPtr
     */
    ActionPtr invokeExtensionEventHandler(const std::string& uri, const std::string& name,
                                          const ObjectMap& data, bool fastMode,
                                          std::string resourceId = "");

    /**
     * Update context time-related variables.
     * @param elapsedTime The time to move forward to
     * @param utcTime The current UTC time on your system
     */
    void updateTime(apl_time_t utcTime, apl_duration_t localTimeAdjustment);

    /**
     * @return current time.
     */
    apl_time_t currentTime();

    /**
     * @return the RootConfig used to initialize this context.
     */
     const RootConfig& rootConfig();

    /**
     * @return The content
     */
    const ContentPtr& content() const override { return mContent; }

    /**
     * Create a suitable document-level data-binding context for evaluating a document-level
     * event.
     * @param handler The name of the handler.
     * @param optional optional data to add to the event.
     * @return The document-level data-binding context.
     */
    ContextPtr createDocumentContext(const std::string& handler, const ObjectMap& optional = {});

    /**
     * Create a suitable document-level data-binding context for evaluating a document-level
     * keyboard event.
     * @param handler The name of the handler.
     * @param keyboard The keyboard event.
     * @return The document-level data-binding context.
     */
    ContextPtr createKeyEventContext(const std::string& handler, const ObjectMapPtr& keyboard);

    /**
     * @return The current logging session
     */
    const SessionPtr& getSession() const;

    /**
     * @return The root configuration used to create this context.
     */
    const RootConfig& getRootConfig() const;

    /**
     * @return The current theme
     */
    std::string getTheme() const;

    /**
     * @return Text measurement pointer reference
     */
    const TextMeasurementPtr& measure() const;

    /**
     * Find a component somewhere in the DOM with the given id or uniqueId.
     * @param id The id or uniqueID to search for.
     * @return The component or nullptr if it is not found.
     */
    ComponentPtr findComponentById(const std::string& id) const;

    /**
     * Find a UID object
     * @param uid The uniqueId to search for.
     * @return The object or nullptr if it is not found.
     */
    UIDObject* findByUniqueId(const std::string& uid) const;

    /**
     * @return true if corresponds to embedded document, false otherwise.
     */
    bool isEmbedded() const { return mCore->embedded(); }

    friend streamer& operator<<(streamer& os, const CoreDocumentContext& root);

    bool setup(const CoreComponentPtr& top);

    void processOnMounts();

    void flushDataUpdates();

    /**
     * Refresh content evaluation state.
     * @return true if content requires resolution, after refresh, false otherwise.
     */
    bool refreshContent();

    const ConfigurationChange& activeChanges() const { return mActiveConfigurationChanges; }

    const Metrics& currentMetrics() const;
    const RootConfig& currentConfig() const;

    /**
     * @param documentContext Pointer to cast.
     * @return Casted pointer to this type
     */
    static CoreDocumentContextPtr cast(const DocumentContextPtr& documentContext);

    /// DocumentContext overrides
    bool isVisualContextDirty() const override;
    void clearVisualContextDirty() override;
    rapidjson::Value serializeVisualContext(rapidjson::Document::AllocatorType& allocator) override;
    bool isDataSourceContextDirty() const override;
    void clearDataSourceContextDirty() override;
    rapidjson::Value serializeDataSourceContext(rapidjson::Document::AllocatorType& allocator) override;
    rapidjson::Value serializeDOM(bool extended, rapidjson::Document::AllocatorType& allocator) override;
    rapidjson::Value serializeContext(rapidjson::Document::AllocatorType& allocator) override;
    ActionPtr executeCommands(const Object& commands, bool fastMode) override;

private:
    #ifdef ALEXAEXTENSIONS
        friend class ExtensionMediator;
    #endif

    void init(const Metrics& metrics,
              const RootConfig& config,
              const SharedContextDataPtr& sharedData,
              bool reinflation);

    bool verifyAPLVersionCompatibility(const std::vector<PackagePtr>& ordered,
                                       const APLVersion& compatibilityVersion);
    bool verifyTypeField(const std::vector<PackagePtr>& ordered, bool enforce);
    ObjectMapPtr createDocumentEventProperties(const std::string& handler) const;

private:
    friend class CoreRootContext;

    ContentPtr mContent;
    ContextPtr mContext;
    DocumentContextDataPtr mCore;  // When you die, make sure to tell the data to terminate itself.
    ConfigurationChange mActiveConfigurationChanges;
    ConfigurationChange mResultingConfigurationChange;
    DisplayState mDisplayState;

    apl_time_t mUTCTime;
    apl_duration_t mLocalTimeAdjustment;
};

} // namespace apl

#endif //_APL_CORE_DOCUMENT_CONTEXT_H
