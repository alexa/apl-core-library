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
#include "apl/livedata/liveobject.h"
#include "apl/primitives/color.h"
#include "apl/primitives/dimension.h"
#include "apl/component/componentproperties.h"

namespace apl {

class LiveDataObject;
class CoreEasing;
class TimeManager;

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
     * @param timeout new double press timeout. Default is 500ms.
     * @return This object for chaining
     */
    RootConfig& doublePressTimeout(apl_duration_t timeout) {
        mDoublePressTimeout = timeout;
        return *this;
    }

    /**
     * Set long press timeout.
     * @param timeout new long press timeout. Default is 1000ms.
     * @return This object for chaining
     */
    RootConfig& longPressTimeout(apl_duration_t timeout) {
        mLongPressTimeout = timeout;
        return *this;
    }

    /**
     * Set SwipeAway gesture trigger distance threshold in pixels. Initial movement below this threshold does not trigger the
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
     * @param velocity swipe velocity threshold in pixels per second.
     * @return This object for chaining
     */
    RootConfig& swipeVelocityThreshold(float velocity) {
        mSwipeVelocityThreshold = velocity;
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
     * @return The map of URI, custom
     */
    const std::vector<ExtensionEventHandler>& getExtensionEventHandlers() const {
        return mExtensionHandlers;
    }

    /**
     * @return The collection of supported extensions and their config values. These are the extensions that have
     *         been marked in the root config as "supported".
     */
    const ObjectMap& getSupportedExtensions() const {
        return mSupportedExtensions;
    }

    /**
     * @param uri The for the extension.
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
     * @return Double press timeout in millisecons.
     */
    apl_duration_t getDoublePressTimeout() const {
        return mDoublePressTimeout;
    }

    /**
     * @return Long press timeout in millisecons.
     */
    apl_duration_t getLongPressTimeout() const {
        return mLongPressTimeout;
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
    std::shared_ptr<Easing> getSwipeAwayAnimationEasing() const {
        return mSwipeAwayAnimationEasing;
    }

    /**
     * @return Swipe velocity threshold.
     */
    float getSwipeVelocityThreshold() const {
        return mSwipeVelocityThreshold;
    }

    /**
     * @return Tick handler update limit.
     */
    apl_duration_t getTickHandlerUpdateLimit() const {
        return mTickHandlerUpdateLimit;
    }

private:
    std::string mAgentName;
    std::string mAgentVersion;
    TextMeasurementPtr mTextMeasurement;
    std::shared_ptr<TimeManager> mTimeManager;
    apl_time_t mUTCTime;
    apl_duration_t mLocalTimeAdjustment;
    std::map<std::string, DataSourceProviderPtr> mDataSources;
    AnimationQuality mAnimationQuality;
    bool mAllowOpenUrl;
    bool mDisallowVideo;
    int mDefaultIdleTimeout;
    APLVersion mEnforcedAPLVersion;
    std::string mReportedAPLVersion;
    bool mEnforceTypeField;
    Color mDefaultFontColor;
    std::string mDefaultFontFamily;
    Color mDefaultHighlightColor;
    std::map<std::string, Color> mDefaultThemeFontColor;
    std::map<std::string, Color> mDefaultThemeHighlightColor;
    bool mTrackProvenance;
    std::map<std::pair<ComponentType, bool>, std::pair<Dimension, Dimension>> mDefaultComponentSize;
    int mPagerChildCache;
    int mSequenceChildCache;
    SessionPtr mSession;
    std::map<std::string, LiveObjectPtr> mLiveObjectMap;
    ObjectMap mSupportedExtensions; // URI -> config
    std::vector<ExtensionEventHandler> mExtensionHandlers;
    std::vector<ExtensionCommandDefinition> mExtensionCommands;
    ObjectMap mEnvironmentValues;
    apl_duration_t mDoublePressTimeout;
    apl_duration_t mLongPressTimeout;
    float mSwipeAwayTriggerDistanceThreshold;
    float mSwipeAwayFulfillDistancePercentageThreshold;
    std::shared_ptr<Easing> mSwipeAwayAnimationEasing;
    float mSwipeVelocityThreshold;
    apl_duration_t mTickHandlerUpdateLimit;
};

}

#endif //_APL_ROOT_CONFIG_H
