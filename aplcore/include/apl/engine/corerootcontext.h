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

#ifndef _APL_CORE_ROOT_CONTEXT_H
#define _APL_CORE_ROOT_CONTEXT_H

#include "apl/engine/rootcontext.h"
#include "apl/engine/context.h"

namespace apl {

/**
 * Core implementation of RootContext API.
 */
class CoreRootContext : public std::enable_shared_from_this<CoreRootContext>, public RootContext {
public:
    static RootContextPtr create(const Metrics& metrics,
                                 const ContentPtr& content,
                                 const RootConfig& config,
                                 std::function<void(const RootContextPtr&)> callback);

    /**
     * Public constructor.  Use the ::create method instead.
     * @param metrics Display metrics
     * @param content Processed APL content data
     * @param config Configuration information
     */
    CoreRootContext(const Metrics& metrics, const ContentPtr& content, const RootConfig& config);

    ~CoreRootContext() override;


    /// RootContext overrides
    void configurationChange(const ConfigurationChange& change) override;
    void updateDisplayState(DisplayState displayState) override;
    void reinflate() override;
    void clearPending() const override;
    bool hasEvent() const override;
    Event popEvent() override;
    Context& context() const override;
    ContextPtr contextPtr() const override;
    ComponentPtr topComponent() const override;
    DocumentContextPtr topDocument() const override;
    bool isDirty() const override;
    const std::set<ComponentPtr>& getDirty() override;
    void clearDirty() override;
    bool isVisualContextDirty() const override;
    void clearVisualContextDirty() override;
    rapidjson::Value serializeVisualContext(rapidjson::Document::AllocatorType& allocator) override;
    bool isDataSourceContextDirty() const override;
    void clearDataSourceContextDirty() override;
    rapidjson::Value serializeDataSourceContext(rapidjson::Document::AllocatorType& allocator) override;
    rapidjson::Value serializeDOM(bool extended, rapidjson::Document::AllocatorType& allocator) override;
    rapidjson::Value serializeContext(rapidjson::Document::AllocatorType& allocator) override;
    APL_DEPRECATED ActionPtr executeCommands(const Object& commands, bool fastMode) override;
    ActionPtr invokeExtensionEventHandler(const std::string& uri, const std::string& name,
                                          const ObjectMap& data, bool fastMode,
                                          std::string resourceId = "") override;
    void cancelExecution() override;
    void updateTime(apl_time_t elapsedTime) override;
    void updateTime(apl_time_t elapsedTime, apl_time_t utcTime) override;
    void setLocalTimeAdjustment(apl_duration_t adjustment) override {
        mLocalTimeAdjustment = adjustment;
    }
    void scrollToRectInComponent(const ComponentPtr& component,
                                 const Rect &bounds,
                                 CommandScrollAlign align) override;
    apl_time_t nextTime() override;
    apl_time_t currentTime() const override;
    bool screenLock() const override;
    const RootConfig&  rootConfig() const override;
    const Settings& settings() const override;
    const ContentPtr& content() const override;
    Info info() const override;
    void updateCursorPosition(Point cursorPosition) override;
    bool handlePointerEvent(const PointerEvent& pointerEvent) override;
    bool handleKeyboard(KeyHandlerType type, const Keyboard &keyboard) override;
    const RootConfig& getRootConfig() const override;
    std::string getTheme() const override;
    ComponentPtr findComponentById(const std::string& id) const override;
    UIDObject* findByUniqueId(const std::string& uid) const override;
    bool setFocus(FocusDirection direction, const Rect& origin, const std::string& targetId) override;
    bool nextFocus(FocusDirection direction, const Rect& origin) override;
    bool nextFocus(FocusDirection direction) override;
    void clearFocus() override;
    std::string getFocused() override;
    std::map<std::string, Rect> getFocusableAreas() override;
    void mediaLoaded(const std::string& source) override;
    void mediaLoadFailed(const std::string& source,
                         int errorCode = -1,
                         const std::string& error = std::string()) override;
    bool getAutoWidth() const;
    bool getAutoHeight() const;

#ifdef SCENEGRAPH
    sg::SceneGraphPtr getSceneGraph() override;
#endif // SCENEGRAPH

    /**
     * Create a suitable document-level data-binding context for evaluating a document-level
     * event.
     * @param handler The name of the handler.
     * @param optional optional data to add to the event.
     * @return The document-level data-binding context.
     */
    ContextPtr createDocumentContext(const std::string& handler, const ObjectMap& optional = {});

    /**
     * @return Text measurement pointer reference
     */
    const TextMeasurementPtr& measure() const;

    /**
     * @return The top-level context with payload binding. This context is used when executing document-level
     *         commands.
     */
    ContextPtr payloadContext() const;

    /**
     * @return Document-used sequencer.
     */
    Sequencer& sequencer() const;

    /**
     * @return PxToDp conversion for the top document
     */
    double getPxToDp() const;

private:
    friend class CoreDocumentContext;
    friend class ExtensionClient; // TODO: Required for backwards compatibility with V1 extension interface
    #ifdef ALEXAEXTENSIONS
        friend class ExtensionMediator;
    #endif

    /**
     * @return The current display state for this root context. Only exposed internally to friend
     *         classes.
     */
    DisplayState getDisplayState() const { return mDisplayState; }

    void init(const Metrics& metrics,
              const RootConfig& config,
              const ContentPtr& content);

    bool setup(bool reinflate);
    ObjectMapPtr createDocumentEventProperties(const std::string& handler) const;
    void clearPendingInternal(bool first) const;
    void updateTimeInternal(apl_time_t elapsedTime, apl_time_t utcTime);

private:
    SharedContextDataPtr mShared;
    std::shared_ptr<TimeManager> mTimeManager;
    apl_time_t mUTCTime = 0;  // Track the system UTC time
    apl_duration_t mLocalTimeAdjustment = 0;
    DisplayState mDisplayState;
    CoreDocumentContextPtr mTopDocument;
#ifdef SCENEGRAPH
    sg::SceneGraphPtr mSceneGraph;
#endif // SCENEGRAPH
};

} // namespace apl

#endif //_APL_CORE_ROOT_CONTEXT_H
