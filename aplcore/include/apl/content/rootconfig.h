/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include "apl/primitives/color.h"
#include "apl/primitives/dimension.h"
#include "apl/component/componentproperties.h"

namespace apl {

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
    RootConfig& reportedAPLVersion(const std::string &version) {
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
    RootConfig& defaultComponentSize( ComponentType type, Dimension width, Dimension height) {
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
    RootConfig& defaultComponentSize( ComponentType type, bool isVertical, Dimension width, Dimension height) {
        mDefaultComponentSize[std::pair<ComponentType, bool>{type, isVertical}] = std::pair<Dimension, Dimension>{width, height};
        return *this;
    }

    /**
     * Set the session
     * @param session The session
     * @return This object for chaining.
     */
    RootConfig& session( const SessionPtr& session ) {
        mSession = session;
        return *this;
    }

    /*
     * Set the current local time in milliseconds since the epoch.
     * @param time Milliseconds.
     * @return This object for chaining.
     */
    RootConfig& localTime(apl_time_t time) {
        mLocalTime = time;
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
    const AnimationQuality getAnimationQuality() const { return mAnimationQuality; }

    /**
     * @return The string name of the current animation quality
     */
    const char* getAnimationQualityString() const {
        switch (mAnimationQuality) {
            case kAnimationQualityNone: return "none";
            case kAnimationQualityNormal: return "normal";
            case kAnimationQualitySlow: return "slow";
        }
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
    Dimension getDefaultComponentWidth(ComponentType type, bool isVertical=true) const {
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
    Dimension getDefaultComponentHeight(ComponentType type, bool isVertical=true) const {
        auto it = mDefaultComponentSize.find({type, isVertical});
        if (it != mDefaultComponentSize.end())
            return it->second.second;

        return {};   // Return an auto dimension
    }

    /**
     * @return The current session pointer
     */
    SessionPtr getSession() const {
        return mSession;
    }

    /*
     * @return The starting local time in milliseconds past the epoch.
     */
    apl_time_t getLocalTime() const {
        return mLocalTime;
    }

    /**
     * @return The local time zone adjustment. This is the duration in milliseconds, which added
     *         to the current time in UTC gives the local time.  This includes any daylight saving
     *         adjustment.
     */
    apl_duration_t getLocalTimeAdjustment() const {
        return mLocalTimeAdjustment;
    }

private:
    TextMeasurementPtr mTextMeasurement;
    std::shared_ptr<TimeManager> mTimeManager;
    apl_time_t mLocalTime;
    apl_duration_t mLocalTimeAdjustment;
    std::string mAgentName;
    std::string mAgentVersion;
    AnimationQuality mAnimationQuality;
    bool mAllowOpenUrl;
    bool mDisallowVideo;
    int mDefaultIdleTimeout;
    APLVersion mEnforcedAPLVersion;
    std::string mReportedAPLVersion;
    bool mEnforceTypeField;
    Color mDefaultFontColor;
    std::map<std::string, Color> mDefaultThemeFontColor;
    std::string mDefaultFontFamily;
    bool mTrackProvenance;
    std::map<std::pair<ComponentType, bool>, std::pair<Dimension, Dimension>> mDefaultComponentSize;
    SessionPtr mSession;
};

}

#endif //_APL_ROOT_CONFIG_H
