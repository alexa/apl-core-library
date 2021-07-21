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

#ifndef _APL_DOCUMENT_H
#define _APL_DOCUMENT_H

#include <map>
#include <set>

#include "apl/content/configurationchange.h"
#include "apl/content/settings.h"
#include "apl/common.h"
#include "apl/engine/event.h"
#include "apl/engine/info.h"
#include "apl/content/rootconfig.h"
#include "apl/focus/focusdirection.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/userdata.h"
#include "apl/primitives/keyboard.h"

#ifdef ALEXAEXTENSIONS
#include "apl/extension/extensionmediator.h"
#endif

namespace apl {

class Metrics;
class RootConfig;
class RootContextData;
class TimeManager;
struct PointerEvent;

/**
 * Represents a top-level APL document.
 *
 * The RootContext is initially constructed from metrics and content.
 * Constructing the RootContext implicitly inflates the component hierarchy
 * and will make text-measurement callbacks.
 *
 * The customer is expected to walk the component hierarchy and inflate appropriate
 * native components.  The native components may hold onto the shared component pointers
 * or may choose to keep a mapping of component ID to native component.
 *
 * After creation, call topComponent() to return the top of the component hierarchy.
 *
 * During normal operation the customer is expected to implement the following loop:
 *
 *     unsigned long step(unsigned long currentTime) {
 *       // Move the clock forward
 *       updateTime(currentTime);
 *
 *       // Check for events
 *       while (root->hasEvent())
 *         handleEvent( root->popEvent() );
 *
 *       // Check for components that need to be updated
 *       if (root->isDirty()) {
 *         for (auto& c : root->getDirty())
 *           updateComponent(c);
 *         root->clearDirty();
 *       }
 *
 *       // Return the next requested clock time.
 *       return nextTime();
 *     }
 *
 * To execute a cloud-driven command, use executeCommands().
 *
 * To cancel any currently running commands, use cancelExecution().
 */
class RootContext : public std::enable_shared_from_this<RootContext>,
                    public UserData<RootContext>,
                    public NonCopyable {
public:
    /**
     * Construct a top-level root context.
     * @param metrics Display metrics
     * @param content Content to display
     * @return A pointer to the root context.
     */
    static RootContextPtr create(const Metrics& metrics,
                                 const ContentPtr& content);

    /**
     * Construct a top-level root context.
     * @param metrics Display metrics
     * @param content Content to display
     * @param config Configuration information
     * @return A pointer to the root context.
     */
    static RootContextPtr create(const Metrics& metrics,
                                 const ContentPtr& content,
                                 const RootConfig& config);

    /**
     * Construct a top-level root context.  This static method is mainly for testing
     * to support modifying the context before layout inflation.
     * @param metrics Display metrics
     * @param content Content to display
     * @param config Configuration information
     * @param callback Pre-layout callback
     * @return A pointer to the root context.
     */
    static RootContextPtr create(const Metrics& metrics,
                                 const ContentPtr& content,
                                 const RootConfig& config,
                                 std::function<void(const RootContextPtr&)> callback);

    /**
     * Inform the view host of a configuration change. Internally this method will trigger the "onConfigChange"
     * event handler in the APL document.  A common behavior in the onConfigChange event handler is to
     * send a Reinflate (kEventTypeReinflate) event.
     *
     * @param change Configuration change information
     */
    void configurationChange(const ConfigurationChange& change);

    /**
     * Reinflate this context using the internally cached configuration changes.  This will terminate any
     * existing animations, remove any events on the queue, clear the dirty components, and create a new
     * component hierarchy.  After calling this method the view host should rebuild its visual hierarchy.
     *
     * This method should be called by the view host when it receives a Reinflate (kEventTypeReinflate) event.
     */
    void reinflate();

    /**
     * Trigger a resize based on stored configuration changes.  This is normally not called by the view host; the
     * RootContext::configurationChange() method handles resizing automatically.
     */
    void resize();

    /**
     * Clear any pending timers that need to be processed and execute any layout passes that are required.
     * This method is called internally by hasEvent(), popEvent(), and isDirty() so you normally don't need
     * to call this directly.
     */
    void clearPending() const;

    /**
     * @return True if there is at least one queued event to be processed.
     */
    bool hasEvent() const;

    /**
     * @return The top event from the event queue.
     */
    Event popEvent();

    /**
     * Public constructor.  Use the ::create method instead.
     * @param metrics Display metrics
     * @param json Processed APL document file
     * @param config Configuration information
     */
    RootContext(const Metrics& metrics, const ContentPtr& content, const RootConfig& config);

    ~RootContext() override;

    /**
     * @return The top-level context.
     */
    Context& context() const { return *mContext; }

    /**
     * @return The top-level context as a shared pointer
     */
    ContextPtr contextPtr() const { return mContext; }

    /**
     * @return The top-level component
     */
    ComponentPtr topComponent();

    /**
     * @return The top-level context with payload binding. This context is used when executing document-level
     *         commands.
     */
    ContextPtr payloadContext() const;

    /**
     * @return True if one or more components needs to be updated.
     */
    bool isDirty() const;

    /**
     * External routine to get the set of components that are dirty.
     * @return The dirty set.
     */
    const std::set<ComponentPtr>& getDirty();

    /**
     * Clear all of the dirty flags.  This routine will clear all dirty
     * flags from child components.
     */
    void clearDirty();

    /**
     * Identifies when the visual context may have changed.  A call to serializeVisualContext resets this value to false.
     * @return true if the visual context has changed since the last call to serializeVisualContext, false otherwise.
     */
    bool isVisualContextDirty() const;

    /**
     * Clear the visual context dirty flag
     */
    void clearVisualContextDirty();

    /**
     * Retrieve component's visual context as a JSON object. This method also clears the
     * visual context dirty flag
     * @param allocator Rapidjson allocator
     * @return The serialized visual context
     */
    rapidjson::Value serializeVisualContext(rapidjson::Document::AllocatorType& allocator);

    /**
     * Identifies when the datasource context may have changed.  A call to serializeDatasourceContext resets this value to false.
     * @return true if the datasource context has changed since the last call to serializeDatasourceContext, false otherwise.
     */
    bool isDataSourceContextDirty() const;

    /**
     * Clear the datasource context dirty flag
     */
    void clearDataSourceContextDirty();

    /**
     * Retrieve datasource context as a JSON array object. This method also clears the
     * datasource context dirty flag
     * @param allocator Rapidjson allocator
     * @return The serialized datasource context
     */
    rapidjson::Value serializeDataSourceContext(rapidjson::Document::AllocatorType& allocator);

    /**
     * Execute an externally-driven command
     * @param commands
     * @param fastMode
     */
    ActionPtr executeCommands(const Object& commands, bool fastMode);

    /**
     * Invoke an extension event handler.
     * @param uri The URI of the custom document handler
     * @param name The name of the handler to invoke
     * @param data The data to associate with the handler
     * @param fastMode If true, this handler will be invoked in fast mode
     * @return An ActionPtr
     */
    ActionPtr invokeExtensionEventHandler(const std::string& uri, const std::string& name,
        const ObjectMap& data, bool fastMode);

    /**
     * Cancel any current commands in execution.  This is typically called
     * as a result of the user touching on the screen to interrupt.
     */
    void cancelExecution();

    /**
     * Move forward in time. This method also advances UTC and Local time by the
     * same amount.
     * @param elapsedTime The time to move forward to.
     */
    void updateTime(apl_time_t elapsedTime);

    /**
     * Move forward in time and separately update local/UTC time.
     * @param elapsedTime The time to move forward to
     * @param utcTime The current UTC time on your system
     */
    void updateTime(apl_time_t elapsedTime, apl_time_t utcTime);

    /**
     * Set the local time zone adjustment.  This is the number of milliseconds added to the UTC time
     * that gives the correct local time including any DST changes.
     * @param adjustment The adjustment time in milliseconds
     */
    void setLocalTimeAdjustment(apl_duration_t adjustment) {
        mLocalTimeAdjustment = adjustment;
    }

    /**
     * Generates a scroll event that will scroll the target component's sub bounds
     * to the correct place with the given alignment.
     * @param component The target component.
     * @param bounds The relative bounds within the target to scroll to.
     * @param align The alignment to scroll to.
     */
    void scrollToRectInComponent(const ComponentPtr& component, const Rect &bounds,
                                 CommandScrollAlign align);

    /**
     * @return The next time an internal timer is scheduled to fire.  This may
     *         be as short as 1 tick past the currentTime().
     */
    apl_time_t nextTime();

    /**
     * @return The current internal time of the system.
     */
    apl_time_t currentTime();

    /**
     * @return True if a command is executing that holds the screen lock.
     */
    bool screenLock();

    /**
     * @return the RootConfig used to initialize this context.
     */
     const RootConfig&  rootConfig();

    /**
     * @deprecated Use Content->getDocumentSettings()
     * @return document-wide properties.
     */
    const Settings& settings();

    /**
     * @return The content
     */
    const ContentPtr& content() const { return mContent; }

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
    ContextPtr createKeyboardDocumentContext(const std::string& handler, const ObjectMapPtr& keyboard);

    /**
     * @return Information about the elements defined within the content
     */
    Info info() const { return Info(mContext, mCore); }

    /**
     * Update cursor position.
     * @param cursor Cursor positon.
     * @deprecated use handlePointerEvent instead
     */
    void updateCursorPosition(Point cursorPosition);

    /**
     * Handle a given PointerEvent with coordinates relative to the viewport.
     * @param pointerEvent The pointer event to handle.
     * @return true if was consumed and should not be passed through any platform handling.
     */
    bool handlePointerEvent(const PointerEvent& pointerEvent);

    /**
     * An update message from the viewhost called when a key is pressed.  The
     * keyboard message is directed to the focused component, or the document
     * if no component is in focus.  If the key event is handled and not propagated
     * this method returns True. False will be returned if the key event is not handled,
     * or the event is handled and propagation of the event is permitted.
     * @param type The key handler type.
     * @param keyboard The keyboard message.
     * @return True, if the key was consumed.
     */
    virtual bool handleKeyboard(KeyHandlerType type, const Keyboard &keyboard);

    /**
     * @return The current logging session
     */
    const SessionPtr& getSession() const;

    /**
     * @return The root configuration provided by the viewhost
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
     * Get top level focusable areas available from APL Core. It's up to engine to decide if it needs to pass focus to
     * the any child of provided area.
     * All dimensions is in APL viewport coordinate space.
     * @return map from ID to focusable area.
     */
    std::map<std::string, Rect> getFocusableAreas();

    /**
     * Pass focus from runtime to APL Core.
     * @param direction focus movement direction.
     * @param origin previously focused area in APL viewport coordinate space.
     * @param targetId ID of area selected by runtime from list provided by getFocusableAreas().
     * @return true if focus was accepted, false otherwise.
     */
    bool setFocus(FocusDirection direction, const Rect& origin, const std::string& targetId);

    /**
     * Request to switch focus in provided direction. Different from setFocus above as actually defers decision on what
     * component to focus to core instead of giving choice to the runtime.
     * @param direction focus movement direction.
     * @param origin previously focused area in APL viewport coordinate space.
     * @return true if processed successfully, false otherwise.
     */
    bool nextFocus(FocusDirection direction, const Rect& origin);

    /**
     * Request to switch focus in provided direction. If nothing is focused works similarly to
     * @see nextFocus(direction, origin) with origin defined as viewport edge opposite to the movement direction.
     * @param direction focus movement direction.
     * @return true if processed successfully, false otherwise.
     */
    bool nextFocus(FocusDirection direction);

    /**
     * Force APL to release focus. Always succeeds.
     */
    void clearFocus();

    /**
     * Check if core has anything focused.
     * @return ID of focused element if something focused, empty if not.
     */
    std::string getFocused();

    /**
     * Notify core about requested media being loaded.
     * @param source requested source.
     */
    void mediaLoaded(const std::string& source);

    /**
     * Notify core about requested media fail to load.
     * @param source requested source.
     */
    void mediaLoadFailed(const std::string& source);

    friend streamer& operator<<(streamer& os, const RootContext& root);

private:
    void init(const Metrics& metrics, const RootConfig& config, bool reinflation);
    bool setup(const CoreComponentPtr& top);
    bool verifyAPLVersionCompatibility(const std::vector<std::shared_ptr<Package>>& ordered,
                                       const APLVersion& compatibilityVersion);
    bool verifyTypeField(const std::vector<std::shared_ptr<Package>>& ordered, bool enforce);
    ObjectMapPtr createDocumentEventProperties(const std::string& handler) const;
    void scheduleTickHandler(const Object& handler, double delay);
    void processTickHandlers();
    void clearPendingInternal(bool first) const;

private:
    ContentPtr mContent;
    ContextPtr mContext;
    std::shared_ptr<RootContextData> mCore;  // When you die, make sure to tell the data to terminate itself.
    std::shared_ptr<TimeManager> mTimeManager;
    apl_time_t mUTCTime;  // Track the system UTC time
    apl_duration_t mLocalTimeAdjustment;
    ConfigurationChange mActiveConfigurationChanges;
};

} // namespace apl

#endif //_APL_DOCUMENT_H
