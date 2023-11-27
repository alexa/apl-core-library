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

#ifndef _APL_ROOT_CONTEXT_H
#define _APL_ROOT_CONTEXT_H

#include "apl/common.h"
#include "apl/content/configurationchange.h"
#include "apl/content/rootconfig.h"
#include "apl/content/settings.h"
#include "apl/document/displaystate.h"
#include "apl/engine/event.h"
#include "apl/engine/info.h"
#include "apl/focus/focusdirection.h"
#include "apl/primitives/keyboard.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/userdata.h"
#ifdef SCENEGRAPH
#include "apl/scenegraph/common.h"
#endif // SCENEGRAPH

#ifdef ALEXAEXTENSIONS
#include "apl/extension/extensionmediator.h"
#endif

namespace apl {

class EventManager;
class Metrics;
class RootConfig;
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
class RootContext : public UserData<RootContext>,
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
     * Construct a top-level root context.
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
     * Notify core of a configuration change. Internally this method will trigger the "onConfigChange"
     * event handler in the APL document.  A common behavior in the onConfigChange event handler is to
     * send a Reinflate (kEventTypeReinflate) event.
     *
     * @param change Configuration change information
     */
    virtual void configurationChange(const ConfigurationChange& change) = 0;

    /**
     * Update the display state of the document. Internally this method will trigger the
     * "onDisplayStateChange" event handler in the APL document, if the display state changed.
     *
     * @param displayState The new display state
     */
    virtual void updateDisplayState(DisplayState displayState) = 0;

    /**
     * Reinflate this context using the internally cached configuration changes.  This will terminate any
     * existing animations, remove any events on the queue, clear the dirty components, and create a new
     * component hierarchy.  After calling this method the view host should rebuild its visual hierarchy.
     *
     * This method should be called by the view host when it receives a Reinflate (kEventTypeReinflate) event.
     */
    virtual void reinflate() = 0;

    /**
     * Clear any pending timers that need to be processed and execute any layout passes that are required.
     * This method is called internally by hasEvent(), popEvent(), and isDirty() so you normally don't need
     * to call this directly.
     */
    virtual void clearPending() const = 0;

    /**
     * @return True if there is at least one queued event to be processed.
     */
    virtual bool hasEvent() const = 0;

    /**
     * @return The top event from the event queue.
     */
    virtual Event popEvent() = 0;

    ~RootContext() override = default;

    /**
     * @return The top-level context.
     */
    virtual Context& context() const = 0;

    /**
     * @return The top-level context as a shared pointer
     */
    virtual ContextPtr contextPtr() const = 0;

    /**
     * @return The top-level component
     */
    virtual ComponentPtr topComponent() const = 0;

    /**
     * @return Top Document context
     */
    virtual DocumentContextPtr topDocument() const = 0;

    /**
     * @return True if one or more components needs to be updated.
     */
    virtual bool isDirty() const = 0;

    /**
     * External routine to get the set of components that are dirty.
     * @return The dirty set.
     */
    virtual const std::set<ComponentPtr>& getDirty() = 0;

    /**
     * Clear all of the dirty flags.  This routine will clear all dirty
     * flags from child components.
     */
    virtual void clearDirty() {}

    /**
     * Identifies when the visual context of the top document may have changed. A call to
     * serializeVisualContext resets this value to false.
     * @return true if the visual context has changed since the last call to serializeVisualContext,
     * false otherwise.
     */
    virtual bool isVisualContextDirty() const = 0;

    /**
     * Clear the top document's visual context dirty flag
     */
    virtual void clearVisualContextDirty() = 0;

    /**
     * Retrieve top document's visual context as a JSON object. This method also clears
     * the visual context dirty flag
     * @param allocator Rapidjson allocator
     * @return The serialized visual context
     */
    virtual rapidjson::Value serializeVisualContext(rapidjson::Document::AllocatorType& allocator) = 0;

    /**
     * Identifies when the datasource context for the top document may have changed.  A call to
     * serializeDatasourceContext resets this value to false.
     * @return true if the datasource context has changed since the last call to
     * serializeDatasourceContext, false otherwise.
     */
    virtual bool isDataSourceContextDirty() const = 0;

    /**
     * Clear the top document's datasource context dirty flag
     */
    virtual void clearDataSourceContextDirty() = 0;

    /**
     * Retrieve top document's datasource context as a JSON array object. This method also clears the
     * datasource context dirty flag
     * @param allocator Rapidjson allocator
     * @return The serialized datasource context
     */
    virtual rapidjson::Value serializeDataSourceContext(rapidjson::Document::AllocatorType& allocator) = 0;

    /**
     * Serialize a complete version of the DOM
     * @param extended If true, serialize everything.  If false, just serialize external data
     * @param allocator Rapidjson allocator
     * @return The serialized DOM
     */
    virtual rapidjson::Value serializeDOM(bool extended, rapidjson::Document::AllocatorType& allocator) = 0;

    /**
     * Serialize the global values for developer tools
     * @param allocator Rapidjson allocator
     * @return The serialized global values
     */
    virtual rapidjson::Value serializeContext(rapidjson::Document::AllocatorType& allocator) = 0;

    /**
     * Execute an externally-driven command
     * @param commands The commands to execute
     * @param fastMode If true this handler will be invoked in fast mode
     * @deprecated Use corresponding API on top document's DocumentContext.
     */
    APL_DEPRECATED virtual ActionPtr executeCommands(const Object& commands, bool fastMode) = 0;

    /**
     * Invoke an extension event handler.
     * @param uri The URI of the custom document handler
     * @param name The name of the handler to invoke
     * @param data The data to associate with the handler
     * @param fastMode If true, this handler will be invoked in fast mode
     * @param resourceId handle associated with extension component if present
     * @deprecated Should not be used, consider switching to ExtensionRegistrar/Extension Proxy.
     * @return An ActionPtr
     */
    APL_DEPRECATED virtual ActionPtr invokeExtensionEventHandler(const std::string& uri,
                                                                 const std::string& name,
                                                                 const ObjectMap& data,
                                                                 bool fastMode,
                                                                 std::string resourceId = "") = 0;

    /**
     * Cancel any current commands in execution.  This is typically called
     * as a result of the user touching on the screen to interrupt.
     */
    virtual void cancelExecution() = 0;

    /**
     * Move forward in time. This method also advances UTC and Local time by the
     * same amount.
     * @param elapsedTime The time to move forward to.
     */
    virtual void updateTime(apl_time_t elapsedTime) = 0;

    /**
     * Move forward in time and separately update local/UTC time.
     * @param elapsedTime The time to move forward to
     * @param utcTime The current UTC time on your system
     */
    virtual void updateTime(apl_time_t elapsedTime, apl_time_t utcTime) = 0;

    /**
     * Set the local time zone adjustment.  This is the number of milliseconds added to the UTC time
     * that gives the correct local time including any DST changes.
     * @param adjustment The adjustment time in milliseconds
     */
    virtual void setLocalTimeAdjustment(apl_duration_t adjustment) = 0;

    /**
     * Generates a scroll event that will scroll the target component's sub bounds
     * to the correct place with the given alignment.
     * @param component The target component.
     * @param bounds The relative bounds within the target to scroll to.
     * @param align The alignment to scroll to.
     */
    virtual void scrollToRectInComponent(const ComponentPtr& component, const Rect &bounds,
                                         CommandScrollAlign align) = 0;

    /**
     * @return The next time an internal timer is scheduled to fire.  This may
     *         be as short as 1 tick past the currentTime().
     */
    virtual apl_time_t nextTime() = 0;

    /**
     * @return The current internal time of the system.
     */
    virtual apl_time_t currentTime() const = 0;

    /**
     * @return True if a command is executing that holds the screen lock.
     */
    virtual bool screenLock() const = 0;

    /**
     * @return the RootConfig used to initialize this context.
     */
    virtual const RootConfig& rootConfig() const = 0;

    /**
     * @return document-wide properties.
     * @deprecated Use Content->getDocumentSettings()
     */
    APL_DEPRECATED virtual const Settings& settings() const = 0;

    /**
     * @return The content
     * @deprecated Use corresponding API on top document's DocumentContext
     */
    APL_DEPRECATED virtual const ContentPtr& content() const = 0;

    /**
     * @return Information about the elements defined within the content
     */
    virtual Info info() const = 0;

    /**
     * Update cursor position.
     * @param cursorPosition Cursor positon.
     * @deprecated use handlePointerEvent instead
     */
    APL_DEPRECATED virtual void updateCursorPosition(Point cursorPosition) = 0;

    /**
     * Handle a given PointerEvent with coordinates relative to the viewport.
     * @param pointerEvent The pointer event to handle.
     * @return true if was consumed and should not be passed through any platform handling.
     */
    virtual bool handlePointerEvent(const PointerEvent& pointerEvent) = 0;

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
    virtual bool handleKeyboard(KeyHandlerType type, const Keyboard &keyboard) = 0;

    /**
     * @return The root configuration provided by the viewhost
     */
    virtual const RootConfig& getRootConfig() const = 0;

    /**
     * Find a component somewhere in the DOM with the given id or uniqueId.
     * @param id The id or uniqueID to search for.
     * @return The component or nullptr if it is not found.
     */
    virtual ComponentPtr findComponentById(const std::string& id) const = 0;

    /**
     * Find a UID object
     * @param uid The uniqueId to search for.
     * @return The object or nullptr if it is not found.
     */
    virtual UIDObject* findByUniqueId(const std::string& uid) const = 0;

    /**
     * @return The current theme
     */
    virtual std::string getTheme() const = 0;

    /**
     * Get top level focusable areas available from APL Core. It's up to engine to decide if it needs to pass focus to
     * the any child of provided area.
     * All dimensions is in APL viewport coordinate space.
     * @return map from ID to focusable area.
     */
    virtual std::map<std::string, Rect> getFocusableAreas() = 0;

    /**
     * Pass focus from runtime to APL Core.
     * @param direction focus movement direction.
     * @param origin previously focused area in APL viewport coordinate space.
     * @param targetId ID of area selected by runtime from list provided by getFocusableAreas().
     * @return true if focus was accepted, false otherwise.
     */
    virtual bool setFocus(FocusDirection direction, const Rect& origin, const std::string& targetId) = 0;

    /**
     * Request to switch focus in provided direction. Different from setFocus above as actually defers decision on what
     * component to focus to core instead of giving choice to the runtime.
     * @param direction focus movement direction.
     * @param origin previously focused area in APL viewport coordinate space.
     * @return true if processed successfully, false otherwise.
     */
    virtual bool nextFocus(FocusDirection direction, const Rect& origin) = 0;

    /**
     * Request to switch focus in provided direction. If nothing is focused works similarly to
     * @see nextFocus(direction, origin) with origin defined as viewport edge opposite to the movement direction.
     * @param direction focus movement direction.
     * @return true if processed successfully, false otherwise.
     */
    virtual bool nextFocus(FocusDirection direction) = 0;

    /**
     * Force APL to release focus. Always succeeds.
     */
    virtual void clearFocus() = 0;

    /**
     * Check if core has anything focused.
     * @return ID of focused element if something focused, empty if not.
     */
    virtual std::string getFocused() = 0;

    /**
     * Notify core about requested media being loaded.
     * @param source requested source.
     */
    virtual void mediaLoaded(const std::string& source) = 0;

    /**
     * Notify core about requested media fail to load.
     * @param source requested source.
     * @param errorCode integer with the errorValue, to determine by the runtime.
     * @param error string with the error description.
     */
    virtual void mediaLoadFailed(const std::string& source, int errorCode = -1, const std::string& error = std::string()) = 0;

    /**
     * @return The size of the viewport, in dp
     */
    virtual Size getViewportSize() const = 0;

#ifdef SCENEGRAPH
    /**
     * This method returns the current scene graph.  It will clear all dirty properties as well.
     * @return The current scene graph
     */
    virtual sg::SceneGraphPtr getSceneGraph() = 0;
#endif // SCENEGRAPH

    friend streamer& operator<<(streamer& os, const RootContext& root);
};

} // namespace apl

#endif //_APL_ROOT_CONTEXT_H
