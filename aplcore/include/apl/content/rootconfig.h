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

#ifndef _APL_ROOT_CONFIG_H
#define _APL_ROOT_CONFIG_H

#include <memory>
#include <string>

#include "apl/common.h"
#include "apl/content/aplversion.h"
#include "apl/content/extensioncommanddefinition.h"
#include "apl/content/extensioneventhandler.h"
#include "apl/content/extensionfilterdefinition.h"
#include "apl/livedata/liveobject.h"
#include "apl/primitives/color.h"
#include "apl/primitives/dimension.h"
#include "apl/component/componentproperties.h"

namespace apl {

class LiveDataObject;
class CoreEasing;
class TimeManager;
class LiveDataObjectWatcher;

using LiveDataObjectWatcherPtr = std::shared_ptr<LiveDataObjectWatcher>;
using LiveDataObjectWatcherWPtr = std::weak_ptr<LiveDataObjectWatcher>;

/**
 * Configuration settings used when creating a RootContext.
 *
 * This is normally used as:
 *
 *     RootConfig config = RootConfig().agent("MyApplication", "1.0")
 *                                     .measure( measureObject )
 *                                     .timeManager( timeManager );
 */
class RootConfig {

public:
    enum AnimationQuality {
        kAnimationQualityNone,
        kAnimationQualitySlow,
        kAnimationQualityNormal
    };

    enum ScreenMode {
        kScreenModeNormal,
        kScreenModeHighContrast
    };

    enum ExperimentalFeature {
        /// Core internal scrolling and paging.
        kExperimentalFeatureHandleScrollingAndPagingInCore
    };

    /**
     * Default constructor
     */
    RootConfig();

    /**
     * Configure the agent name and version
     * @param agentName
     * @param agentVersion
     * @return This object for chaining
     */
    RootConfig& agent(const std::string& agentName, const std::string& agentVersion) {
        mAgentName = agentName;
        mAgentVersion = agentVersion;
        return *this;
    }

    /**
     * Add a text measurement object for calculating the size of blocks
     * of text and calculating the baseline of text.
     * @param textMeasurementPtr The text measurement object.
     * @return This object for chaining.
     */
    RootConfig& measure(const TextMeasurementPtr& textMeasurementPtr) {
        mTextMeasurement = textMeasurementPtr;
        return *this;
    }

    /**
     * Specify the time manager.
     * @param timeManager The time manager
     * @return This object for chaining.
     */
    RootConfig& timeManager(const std::shared_ptr<TimeManager>& timeManager) {
        mTimeManager = timeManager;
        return *this;
    }

    /**
     * Set if the OpenURL command is supported
     * @param allowed If true, the OpenURL command is supported.
     * @return This object for chaining
     */
    RootConfig& allowOpenUrl(bool allowed) {
        mAllowOpenUrl = allowed;
        return *this;
    }

    /**
     * Set if video is supported
     * @param disallowed If true, the Video component is disabled
     * @return This object for chaining
     */
    RootConfig& disallowVideo(bool disallowed) {
        mDisallowVideo = disallowed;
        return *this;
    }

    /**
     * Set the quality of animation expected.  If set to kAnimationQualityNone,
     * all animation commands are disabled (include onMount).
     * @param quality The expected quality of animation playback.
     * @return This object for chaining
     */
    RootConfig& animationQuality(AnimationQuality quality) {
        mAnimationQuality = quality;
        return *this;
    }

    /**
     * Set the default idle timeout.
     * @param idleTimeout Device wide idle timeout..
     * @return This object for chaining
     */
    RootConfig& defaultIdleTimeout(int idleTimeout) {
        mDefaultIdleTimeout = idleTimeout;
        return *this;
    }

    /**
     * Set how APL spec version check should be enforced.
     * @param version @see APLVersion::Value.
     * @return This object for chaining
     */
    RootConfig& enforceAPLVersion(APLVersion::Value version) {
        mEnforcedAPLVersion = version;
        return *this;
    }

    /**
     * Set the reported APL version of the specification that is supported
     * by this application.  This value will be reported in the data-binding
     * context under "environment.aplVersion".
     * @param version The version string to report.
     * @return This object for chaining.
     */
    RootConfig& reportedAPLVersion(const std::string& version) {
        mReportedAPLVersion = version;
        return *this;
    }

    /**
     * Sets whether the "type" field of an APL document should be enforced.
     * Type should always be "APL", but for backwards compatibility, this is
     * optionally ignored.
     * @param enforce `true` to enforce that the "type" field is set to "APL"
     * @return This object for chaining
     */
    RootConfig& enforceTypeField(bool enforce) {
        mEnforceTypeField = enforce;
        return *this;
    }

    /**
     * Set the default font color. This is the fallback color for all themes.
     * This color will only be applied if there is not a theme-defined default color.
     * @param color The font color
     * @return This object for chaining
     */
    RootConfig& defaultFontColor(Color color) {
        mDefaultFontColor = color;
        return *this;
    }

    /**
     * Set the default font color for a particular theme.
     * @param theme The named theme (must match exactly)
     * @param color The font color
     * @return This object for chaining
     */
    RootConfig& defaultFontColor(const std::string& theme, Color color) {
        mDefaultThemeFontColor[theme] = color;
        return *this;
    }

    /**
     * Set the default font family. This is usually locale-based.
     * @param fontFamily The font family.
     * @return This object for chaining.
     */
    RootConfig& defaultFontFamily(const std::string& fontFamily) {
        mDefaultFontFamily = fontFamily;
        return *this;
    }

    /**
     * Enable or disable tracking of resource, style, and component provenance
     * @param trackProvenance True if provenance should be tracked.
     * @return This object for chaining.
     */
    RootConfig& trackProvenance(bool trackProvenance) {
        mTrackProvenance = trackProvenance;
        return *this;
    }

    /**
     * Set the default size of a built-in component.  This applies to both horizontal and vertical components
     * @param type The component type.
     * @param width The new default width.
     * @param height The new default height.
     * @return This object for chaining.
     */
    RootConfig& defaultComponentSize(ComponentType type, Dimension width, Dimension height) {
        defaultComponentSize(type, true, width, height);
        return *this;
    }

    /**
     * Set the default size of a built-in component with a particular direction.
     * @param type The component type
     * @param isVertical If true, this applies to a vertical scrolling component.  If false, it applies to a horizontal.
     * @param width The new default width.
     * @param height The new default height.
     * @return This object for chaining.
     */
    RootConfig& defaultComponentSize(ComponentType type, bool isVertical, Dimension width, Dimension height) {
        mDefaultComponentSize[std::pair<ComponentType, bool>{type, isVertical}] = std::pair<Dimension, Dimension>{width,
                                                                                                                  height};
        return *this;
    }

    /**
     * Set pager layout cache in both directions. 1 is default and results in 1 page ensured before and one after
     * current one.
     * @param cache Number of pages to ensure before and after current one.
     * @return This object for chaining.
     */
    RootConfig& pagerChildCache(int cache) {
        mPagerChildCache = cache;
        return *this;
    }

    /**
     * Set sequence layout cache in both directions. 1 is default and results in 1 page of children ensured before and
     * one after current one.
     * @param cache Number of pages to ensure before and after current one.
     * @return This object for chaining.
     */
    RootConfig& sequenceChildCache(int cache) {
        mSequenceChildCache = cache;
        return *this;
    }

    /**
     * Add DataSource provider implementation.
     * @param type Type name of DataSource.
     * @param dataSourceProvider provider implementation.
     * @return This object for chaining.
     */
    RootConfig& dataSourceProvider(const std::string& type, const DataSourceProviderPtr& dataSourceProvider) {
        mDataSources.emplace(type, dataSourceProvider);
        return *this;
    }

    /**
     * Set the session
     * @param session The session
     * @return This object for chaining.
     */
    RootConfig& session(const SessionPtr& session) {
        mSession = session;
        return *this;
    }

    /*
     * Set the current UTC time in milliseconds since the epoch.
     * @param time Milliseconds.
     * @return This object for chaining.
     */
    RootConfig& utcTime(apl_time_t time) {
        mUTCTime = time;
        return *this;
    }

    /**
     * Set the local time zone adjustment in milliseconds.  When added to the current UTC time,
     * this will give the local time. This includes any daylight saving time adjustment.
     * @param adjustment Milliseconds
     * @return This object for chaining
     */
    RootConfig& localTimeAdjustment(apl_duration_t adjustment) {
        mLocalTimeAdjustment = adjustment;
        return *this;
    }

    /**
     * Assign a LiveObject to the top-level context
     * @param name The name of the LiveObject
     * @param object The object itself
     * @return This object for chaining
     */
    RootConfig& liveData(const std::string& name, const LiveObjectPtr& object) {
        mLiveObjectMap.emplace(name, object);
        return *this;
    }

    /**
     * Register watcher for live data object with given name.
     * @param name Name of LiveData object.
     * @param watcher Watcher.
     * @return
     */
    RootConfig& liveDataWatcher(const std::string& name, const std::shared_ptr<LiveDataObjectWatcher>& watcher) {
        mLiveDataObjectWatchersMap.emplace(name, watcher);
        return *this;
    }

    /**
     * Register an extension event handler.  The name should be something like 'onDomainAction'.
     * This method will also register the extension as a supported extension.
     * @param uri The extension URI this handler is registered to
     * @param name The name of the handler to support.
     * @return This object for chaining.
     */
    RootConfig& registerExtensionEventHandler(ExtensionEventHandler handler) {
        auto uri = handler.getURI();
        if (mSupportedExtensions.find(uri) == mSupportedExtensions.end()) {
            registerExtension(uri);
        }
        mExtensionHandlers.emplace_back(std::move(handler));
        return *this;
    }

    /**
     * Register an extension command that can be executed in the document.  The name should be something like 'DomainEvent'.
     * This method will also register the extension as a supported extension.
     * @param commandDef The definition of the custom command (includes the name, URI, etc).
     * @return This object for chaining
     */
    RootConfig& registerExtensionCommand(ExtensionCommandDefinition commandDef) {
        auto uri = commandDef.getURI();
        if (mSupportedExtensions.find(uri) == mSupportedExtensions.end()) {
            registerExtension(uri);
        }
        mExtensionCommands.emplace_back(std::move(commandDef));
        return *this;
    }

    /**
     * Register an extension filter that can be executed in Image components.
     * This method will also register the extension as a supported extension.
     * @param filterDef The definition of the custom filter (includes the name, URI, properties)
     * @return This object for chaining
     */
    RootConfig& registerExtensionFilter(ExtensionFilterDefinition filterDef) {
        const auto& uri = filterDef.getURI();
        if (mSupportedExtensions.find(uri) == mSupportedExtensions.end())
            registerExtension(uri);
        mExtensionFilters.emplace_back(std::move(filterDef));
        return *this;
    }

    /**
     * Register an environment for an extension.  The document may access the extension environment by
     * the extension name in the “environment.extension” environment property.
     * Any previously registered environment is overwritten.
     * This method will also register the extension as a supported extension.
     *
     * @param uri The URI of the extension
     * @param environment values
     * @return This object for chaining
     */
    RootConfig& registerExtensionEnvironment(const std::string& uri, const Object& environment) {
        registerExtension(uri, environment);
        return *this;
    }

    /**
     * Report a supported extension. Any previously registered configuration is overwritten.
     * @param uri The URI of the extension
     * @param optional configuration value(s) supported by this extension.
     * @return This object for chaining
     */
    RootConfig& registerExtension(const std::string& uri, const Object& config = Object::TRUE_OBJECT()) {
        mSupportedExtensions[uri] = config;
        return *this;
    }

    /**
     * Register a value to be reported in the data-binding "environment" context.
     * @param name The name of the value
     * @param value The value to report.  This will be read-only.
     * @return This object for chaining
     */
    RootConfig& setEnvironmentValue(const std::string& name, const Object& value) {
        mEnvironmentValues[name] = value;
        return *this;
    }

    /**
     * Set double press timeout.
     * @param timeout new double press timeout. Default is 500 ms.
     * @return This object for chaining
     */
    RootConfig& doublePressTimeout(apl_duration_t timeout) {
        mDoublePressTimeout = timeout;
        return *this;
    }

    /**
     * Set long press timeout.
     * @param timeout new long press timeout. Default is 1000 ms.
     * @return This object for chaining
     */
    RootConfig& longPressTimeout(apl_duration_t timeout) {
        mLongPressTimeout = timeout;
        return *this;
    }

    /**
     * Set pressed duration timeout.  This is the duration to show the "pressed" state of a component
     * when programmatically invoked.
     * @param timeout Duration in milliseconds.  Default is 64 ms.
     * @return This object for chaining
     */
    RootConfig& pressedDuration(apl_duration_t timeout) {
        mPressedDuration = timeout;
        return *this;
    }

    /**
     * Set the tap or scroll timeout.  This is the maximum amount of time that can pass before the
     * system has to commit to this being a touch event instead of a scroll event.
     * @param timeout Duration in milliseconds.  Default is 100 ms.
     * @return This object for chaining
     */
    RootConfig& tapOrScrollTimeout(apl_duration_t timeout) {
        mTapOrScrollTimeout = timeout;
        return *this;
    }

    /**
     * Set SwipeAway gesture trigger distance threshold in dp. Initial movement below this threshold does not trigger the
     * gesture.
     * @param distance threshold distance.
     * @return This object for chaining
     */
    RootConfig& swipeAwayTriggerDistanceThreshold(float distance) {
        mSwipeAwayTriggerDistanceThreshold = distance;
        return *this;
    }

    /**
     * Set SwipeAway gesture fulfill distance threshold in percents. Gesture requires swipe to be performed above this
     * threshold for it to be considered complete.
     * @param distance threshold distance.
     * @return This object for chaining
     */
    RootConfig& swipeAwayFulfillDistancePercentageThreshold(float distance) {
        mSwipeAwayFulfillDistancePercentageThreshold = distance;
        return *this;
    }

    /**
     * Set SwipeAway gesture animation easing.
     * @param easing Easing string to use for gesture animation. Should be according to current APL spec.
     * @return This object for chaining
     */
    RootConfig& swipeAwayAnimationEasing(const std::string& easing);

    /**
     * Set SwipeAway (and any related gesture) gesture swipe speed threshold.
     * @param velocity swipe velocity threshold in dp per second.
     * @return This object for chaining
     */
    RootConfig& swipeVelocityThreshold(float velocity) {
        mSwipeVelocityThreshold = velocity;
        return *this;
    }

    /**
     * Set SwipeAway gesture tolerance in degrees when determining whether a swipe was triggered.
     * This is provided as a convenience API in addition to @c swipeAngleSlope.
     *
     * @param degrees swipe direction tolerance, in degrees.
     * @return This object for chaining
     */
    RootConfig& swipeAngleTolerance(float degrees);

    /**
     * Set the max pointer slope allowed during SwipeAway gesture. Also see
     * @c swipeAngleTolerance for an alternative way to specify this value.
     *
     * @param slope max pointer slope
     * @return This object for chaining
     */
    RootConfig& swipeAngleSlope(float slope) {
        mSwipeAngleSlope = slope;
        return *this;
    }

    /**
     * Set max animation duration, in ms, for SwipeAway animations.
     * @param duration the maximum duration for animations, in ms.
     * @return This object for chaining
     */
    RootConfig& maxSwipeAnimationDuration(apl_duration_t duration) {
        mMaxSwipeAnimationDuration = duration;
        return *this;
    }

    /**
     * Set the fling velocity threshold.  The user must fling at least this fast to start a fling action.
     * @param velocity Fling velocity in dp per second.
     * @return This object for chaining
     */
    RootConfig& minimumFlingVelocity(float velocity) {
        mMinimumFlingVelocity = velocity;
        return *this;
    }

    /**
     * Set a tick handler update limit in ms. Default is 16ms (60 FPS).
     * @param updateLimit update limit in ms. Should be > 0.
     * @return  This object for chaining
     */
    RootConfig& tickHandlerUpdateLimit(apl_duration_t updateLimit) {
        mTickHandlerUpdateLimit = std::max(updateLimit, 1.0);
        return *this;
    }

    /**
     * Set the requested font scaling factor for the document.
     * @param scale The scaling factor.  Default is 1.0
     * @return This object for chaining
     */
    RootConfig& fontScale(float scale) {
        mFontScale = scale;
        return *this;
    }

    /**
     * Set the screen display mode for accessibility (normal or high-contrast)
     * @param screenMode The screen display mode
     * @return This object for chaining
     */
    RootConfig& screenMode(ScreenMode screenMode) {
        mScreenMode = screenMode;
        return *this;
    }

    /**
     * Inform that a screen reader is turned on.
     * @param enabled True if the screen reader is enabled
     * @return This object for chaining
     */
    RootConfig& screenReader(bool enabled) {
        mScreenReaderEnabled = enabled;
        return *this;
    }

    /**
     * Enable core internal scrolling and paging.
     * @experimental
     * @deprecated Use enableExperimentalFeature(kHandleScrollingAndPagingInCore)
     * @param flag true to enable, false by default.
     * @return This object for chaining
     */
    RootConfig& handleScrollAndPagingInCore(bool flag) {
        if (flag) {
            enableExperimentalFeature(kExperimentalFeatureHandleScrollingAndPagingInCore);
        }
        return *this;
    }

    /**
     * Set pointer inactivity timeout. Pointer considered stale after pointer was not updated for this time.
     * @param timeout inactivity timeout in ms.
     * @return This object for chaining
     */
    RootConfig& pointerInactivityTimeout(apl_duration_t timeout) {
        mPointerInactivityTimeout = timeout;
        return *this;
    }

    /**
     * Set fling gestures velocity limit.
     * @param velocity fling gesture velocity in dp per second.
     * @return This object for chaining
     */
    RootConfig& maximumFlingVelocity(float velocity) {
        mMaximumFlingVelocity = velocity;
        return *this;
    }

    /**
     * Set the gesture distance threshold in dp. Initial movement below this threshold does not trigger
     * gestures.
     * @param distance threshold distance.
     * @return This object for chaining
     */
    RootConfig& pointerSlopThreshold(float distance) {
        mPointerSlopThreshold = distance;
        return *this;
    }

    /**
     * Set scroll commands duration.
     * @param duration duration in ms.
     * @return This object for chaining
     */
    RootConfig& scrollCommandDuration(apl_duration_t duration) {
        mScrollCommandDuration = duration;
        return *this;
    }

    /**
     * Set scroll snap duration.
     * @param duration duration in ms.
     * @return This object for chaining
     */
    RootConfig& scrollSnapDuration(apl_duration_t duration) {
        mScrollSnapDuration = duration;
        return *this;
    }

    /**
     * Set default pager page switch animation duration.
     * @param duration duration in ms.
     * @return This object for chaining
     */
    RootConfig& defaultPagerAnimationDuration(apl_duration_t duration) {
        mDefaultPagerAnimationDuration = duration;
        return *this;
    }

    /**
     * Enable experimental feature. @see enum ExperimentalFeatures for available set.
     * None of the features enabled by default.
     * @experimental Not guaranteed to work for any of available features and can change Engine behaviors drastically.
     * @param feature experimental feature to enable.
     * @return This object for chaining
     */
    RootConfig& enableExperimentalFeature(ExperimentalFeature feature) {
        mEnabledExperimentalFeatures.emplace(feature);
        return *this;
    }

    /**
     * @return The configured text measurement object.
     */
    TextMeasurementPtr getMeasure() const { return mTextMeasurement; }

    /**
     * @return The time manager object
     */
    std::shared_ptr<TimeManager> getTimeManager() const { return mTimeManager; }

    /**
     * @return The agent name string
     */
    std::string getAgentName() const { return mAgentName; }

    /**
     * @return The agent version string
     */
    std::string getAgentVersion() const { return mAgentVersion; }

    /**
     * @return The expected animation quality
     */
    AnimationQuality getAnimationQuality() const { return mAnimationQuality; }

    /**
     * @return The string name of the current animation quality
     */
    const char* getAnimationQualityString() const {
        switch (mAnimationQuality) {
            case kAnimationQualityNone:
                return "none";
            case kAnimationQualityNormal:
                return "normal";
            case kAnimationQualitySlow:
                return "slow";
        }
        return "none";
    }

    /**
     * @return True if the OpenURL command is supported
     */
    bool getAllowOpenUrl() const { return mAllowOpenUrl; }

    /**
     * @return True if the video component is not supported.
     */
    bool getDisallowVideo() const { return mDisallowVideo; }

    /**
     * @return Time in ms for default IdleTimeut value.
     */
    int getDefaultIdleTimeout() const { return mDefaultIdleTimeout; };

    /**
     * @return The version or versions of the specification that should be enforced.
     */
    APLVersion getEnforcedAPLVersion() const { return mEnforcedAPLVersion; }

    /**
     * @return The reported version of APL used during document inflation
     */
    std::string getReportedAPLVersion() const { return mReportedAPLVersion; }

    /**
     * @return true if the type field value of an APL doc should be enforced
     */
    bool getEnforceTypeField() const { return mEnforceTypeField; }

    /**
     * @return The default font color
     */
    Color getDefaultFontColor(const std::string& theme) const {
        auto it = mDefaultThemeFontColor.find(theme);
        if (it != mDefaultThemeFontColor.end())
            return it->second;
        return mDefaultFontColor;
    }

    /**
     * @return The default text highlight color
     */
    Color getDefaultHighlightColor(const std::string& theme) const {
        auto it = mDefaultThemeHighlightColor.find(theme);
        if (it != mDefaultThemeHighlightColor.end())
            return it->second;
        return mDefaultHighlightColor;
    }

    /**
     * @return The default font family
     */
    std::string getDefaultFontFamily() const { return mDefaultFontFamily; }

    /**
     * @return True if provenance of resources, styles, and components will be calculated.
     */
    bool getTrackProvenance() const { return mTrackProvenance; }

    /**
     * Return the default width for this component type.
     * @param type The component type.
     * @param isVertical If true, this applies to a vertical scrolling component. If false, it applies to a horizontal.
     * @return The default width.
     */
    Dimension getDefaultComponentWidth(ComponentType type, bool isVertical = true) const {
        auto it = mDefaultComponentSize.find({type, isVertical});
        if (it != mDefaultComponentSize.end())
            return it->second.first;

        return {};   // Return an auto dimension
    }

    /**
     * Return the default height for this component type.
     * @param type The component type.
     * @param isVertical If true, this applies to a vertical scrolling component. If false, it applies to a horizontal.
     * @return The default height.
     */
    Dimension getDefaultComponentHeight(ComponentType type, bool isVertical = true) const {
        auto it = mDefaultComponentSize.find({type, isVertical});
        if (it != mDefaultComponentSize.end())
            return it->second.second;

        return {};   // Return an auto dimension
    }

    /**
     * Return number of pages to ensure around current page.
     * @return number of pages.
     */
    int getPagerChildCache() const {
        return mPagerChildCache;
    }

    /**
     * Return number of pages to ensure around current one.
     * @return number of pages.
     */
    int getSequenceChildCache() const {
        return mSequenceChildCache;
    }

    /**
     * @return The current session pointer
     */
    SessionPtr getSession() const {
        return mSession;
    }

    /*
     * @return The starting UTC time in milliseconds past the epoch.
     */
    apl_time_t getUTCTime() const {
        return mUTCTime;
    }

    /**
     * @return The local time zone adjustment. This is the duration in milliseconds, which added
     *         to the current time in UTC gives the local time.  This includes any daylight saving
     *         adjustment.
     */
    apl_duration_t getLocalTimeAdjustment() const {
        return mLocalTimeAdjustment;
    }

    /**
     * @return A reference to the map of live data sources
     */
    const std::map<std::string, LiveObjectPtr>& getLiveObjectMap() const {
        return mLiveObjectMap;
    }

    /**
     * @param LiveData name.
     * @return Registered LiveDataWatcher for provided name.
     */
    const std::vector<LiveDataObjectWatcherPtr> getLiveDataWatchers(const std::string& name) const {
        std::vector<LiveDataObjectWatcherPtr> result;
        auto watchers = mLiveDataObjectWatchersMap.equal_range(name);
        for (auto it = watchers.first; it != watchers.second; it++) {
            auto ptr = it->second.lock();
            if (ptr)
                result.emplace_back(ptr);
        }
        return result;
    }

    /**
     * Get data source provider for provided type.
     * @param type DataSource type.
     * @return DataSourceProvider if registered, nullptr otherwise.
     */
    DataSourceProviderPtr getDataSourceProvider(const std::string& type) const {
        auto provider = mDataSources.find(type);
        if (provider != mDataSources.end()) {
            return provider->second;
        }
        return nullptr;
    }

    /**
     * @param type DataSource type.
     * @return true if registered, false otherwise.
     */
    bool isDataSource(const std::string& type) const {
        return (mDataSources.find(type) != mDataSources.end());
    }

    /**
     * @return The registered extension commands
     */
    const std::vector<ExtensionCommandDefinition>& getExtensionCommands() const {
        return mExtensionCommands;
    }

    /**
     * @return The registered extension events
     */
    const std::vector<ExtensionEventHandler>& getExtensionEventHandlers() const {
        return mExtensionHandlers;
    }

    /**
     * @return The registered extension filters
     */
    const std::vector<ExtensionFilterDefinition>& getExtensionFilters() const {
        return mExtensionFilters;
    }

    /**
     * @return The collection of supported extensions and their config values. These are the extensions that have
     *         been marked in the root config as "supported".
     */
    const ObjectMap& getSupportedExtensions() const {
        return mSupportedExtensions;
    }

    /**
     * @param uri The URI of the extension.
     * @return The environment Object for a supported extensions, Object::NULL_OBJECT if no environment exists.
     */
    Object getExtensionEnvironment(const std::string& uri) const {
        auto it = mSupportedExtensions.find(uri);
        if (it != mSupportedExtensions.end())
            return it->second;
        return Object::NULL_OBJECT();
    }

    /**
     * @return A map of values to be reported in the data-binding environment context
     */
    const ObjectMap& getEnvironmentValues() const {
        return mEnvironmentValues;
    }

    /**
     * @return Double press timeout in milliseconds.
     */
    apl_duration_t getDoublePressTimeout() const {
        return mDoublePressTimeout;
    }

    /**
     * @return Long press timeout in milliseconds.
     */
    apl_duration_t getLongPressTimeout() const {
        return mLongPressTimeout;
    }

    /**
     * @return Duration to show the "pressed" state of a component when programmatically invoked
     */
    apl_duration_t getPressedDuration() const {
        return mPressedDuration;
    }

    /**
     * @return Maximum time to wait before deciding that a touch event starts a scroll or paging gesture.
     */
    apl_duration_t getTapOrScrollTimeout() const {
        return mTapOrScrollTimeout;
    }

    /**
     * @return SwipeAway trigger distance threshold.
     */
    float getSwipeAwayTriggerDistanceThreshold() const {
        return mSwipeAwayTriggerDistanceThreshold;
    }

    /**
     * @return SwipeAway fulfill distance threshold in percents.
     */
    float getSwipeAwayFulfillDistancePercentageThreshold() const {
        return mSwipeAwayFulfillDistancePercentageThreshold;
    }

    /**
     * @return Animation easing for SwipeAway gesture.
     */
    EasingPtr getSwipeAwayAnimationEasing() const {
        return mSwipeAwayAnimationEasing;
    }

    /**
     * @return Swipe velocity threshold.
     */
    float getSwipeVelocityThreshold() const {
        return mSwipeVelocityThreshold;
    }

    /**
     * @return Max allowed pointer movement angle during swipe gestures, as a slope.
     */
    float getSwipeAngleSlope() const {
        return mSwipeAngleSlope;
    }

    /**
     * @return Max animation duration for SwipeAway gestures, in ms.
     */
    apl_duration_t getMaxSwipeAnimationDuration() const {
        return mMaxSwipeAnimationDuration;
    }

    /**
     * @return Fling velocity threshold
     */
    float getMinimumFlingVelocity() const {
        return mMinimumFlingVelocity;
    }

    /**
     * @return Tick handler update limit.
     */
    apl_duration_t getTickHandlerUpdateLimit() const {
        return mTickHandlerUpdateLimit;
    }

    /**
     * @return The requested scaling factor for fonts
     */
    float getFontScale() const {
        return mFontScale;
    }

    /**
     * @return The current screen mode (high-contrast or normal)
     */
    const char * getScreenMode() const {
        switch (mScreenMode) {
            case kScreenModeNormal:
                return "normal";
            case kScreenModeHighContrast:
                return "high-contrast";
        }
        return "normal";
    }

    /**
     * @return True if an accessibility screen reader is enabled
     */
    bool getScreenReaderEnabled() const {
        return mScreenReaderEnabled;
    }

    /**
     * @return Pointer inactivity timeout.
     */
    apl_duration_t getPointerInactivityTimeout() const {
        return mPointerInactivityTimeout;
    }

    /**
     * @return Maximum fling speed.
     */
    float getMaximumFlingVelocity() const {
        return mMaximumFlingVelocity;
    }

    /**
     * @return Pointer slop threshold.
     */
    float getPointerSlopThreshold() const {
        return mPointerSlopThreshold;
    }

    /**
     * @return Scroll command duration.
     */
    apl_duration_t getScrollCommandDuration() const {
        return mScrollCommandDuration;
    }

    /**
     * @return Scroll snap duration.
     */
    apl_duration_t getScrollSnapDuration() const {
        return mScrollSnapDuration;
    }

    /**
     * @return Default pager animation duration.
     */
    apl_duration_t getDefaultPagerAnimationDuration() const {
        return mDefaultPagerAnimationDuration;
    }

    /**
     * @param feature feature to check.
     * @return true if feature enabled, false otherwise.
     */
    bool experimentalFeatureEnabled(ExperimentalFeature feature) const {
        return mEnabledExperimentalFeatures.count(feature) > 0;
    }

private:
    TextMeasurementPtr mTextMeasurement;
    std::shared_ptr<TimeManager> mTimeManager;
    std::map<std::pair<ComponentType, bool>, std::pair<Dimension, Dimension>> mDefaultComponentSize;

    SessionPtr mSession;
    ObjectMap mEnvironmentValues;
    EasingPtr mSwipeAwayAnimationEasing;

    // Data sources and live maps
    std::map<std::string, DataSourceProviderPtr> mDataSources;
    std::map<std::string, LiveObjectPtr> mLiveObjectMap;
    std::multimap<std::string, LiveDataObjectWatcherWPtr> mLiveDataObjectWatchersMap;

    // Extensions
    ObjectMap mSupportedExtensions; // URI -> config
    std::vector<ExtensionEventHandler> mExtensionHandlers;
    std::vector<ExtensionCommandDefinition> mExtensionCommands;
    std::vector<ExtensionFilterDefinition> mExtensionFilters;

    // Constants with initializers, ordered by type to help bit-packing
    std::string mAgentName = "Default agent";
    std::string mAgentVersion = "1.0";
    std::string mReportedAPLVersion = "1.5";
    std::string mDefaultFontFamily = "sans-serif";

    apl_time_t mUTCTime = 0;
    apl_duration_t mLocalTimeAdjustment = 0;
    apl_duration_t mDefaultIdleTimeout = 30000;
    apl_duration_t mDoublePressTimeout = 500;
    apl_duration_t mLongPressTimeout = 1000;
    apl_duration_t mPressedDuration = 64;
    apl_duration_t mTapOrScrollTimeout = 100;
    apl_duration_t mTickHandlerUpdateLimit = 16;
    apl_duration_t mPointerInactivityTimeout = 50;
    apl_duration_t mScrollCommandDuration = 1000;
    apl_duration_t mScrollSnapDuration = 500;
    apl_duration_t mDefaultPagerAnimationDuration = 600;

    std::map<std::string, Color> mDefaultThemeFontColor = {{"light", 0x1e2222ff}, {"dark", 0xfafafaff}};
    std::map<std::string, Color> mDefaultThemeHighlightColor = {{"light", 0x0070ba4d}, {"dark",  0x00caff4d}};

    AnimationQuality mAnimationQuality = kAnimationQualityNormal;
    APLVersion mEnforcedAPLVersion = APLVersion::kAPLVersionIgnore;
    ScreenMode mScreenMode = kScreenModeNormal;

    Color mDefaultFontColor = 0xfafafaff;
    Color mDefaultHighlightColor = 0x00caff4d;

    int mPagerChildCache = 1;
    int mSequenceChildCache = 1;

    float mSwipeAwayTriggerDistanceThreshold = 10;
    float mSwipeAwayFulfillDistancePercentageThreshold = 0.5;
    float mSwipeVelocityThreshold = 200;
    float mSwipeAngleSlope;
    apl_duration_t mMaxSwipeAnimationDuration = 300;
    float mMinimumFlingVelocity = 50;
    float mMaximumFlingVelocity = 500;
    float mFontScale = 1.0;
    float mPointerSlopThreshold = 10;

    bool mScreenReaderEnabled = false;
    bool mAllowOpenUrl = false;
    bool mDisallowVideo = false;
    bool mEnforceTypeField = false;
    bool mTrackProvenance = true;

    // Set of enabled experimental features
    std::set<ExperimentalFeature> mEnabledExperimentalFeatures;
};

}

#endif //_APL_ROOT_CONFIG_H
