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
#include "apl/component/componentproperties.h"
#include "apl/content/aplversion.h"
#include "apl/content/extensioncommanddefinition.h"
#include "apl/content/extensioncomponentdefinition.h"
#include "apl/content/extensioneventhandler.h"
#include "apl/content/extensionfilterdefinition.h"
#include "apl/content/rootproperties.h"
#include "apl/engine/propertymap.h"
#include "apl/livedata/liveobject.h"
#include "apl/primitives/color.h"
#include "apl/primitives/dimension.h"

#ifdef ALEXAEXTENSIONS
#include <alexaext/alexaext.h>
#endif

namespace apl {

class LiveDataObject;
class CoreEasing;
class TimeManager;
class LiveDataObjectWatcher;
class LocaleMethods;
class RootPropDefSet;

using LiveDataObjectWatcherPtr = std::shared_ptr<LiveDataObjectWatcher>;
using LiveDataObjectWatcherWPtr = std::weak_ptr<LiveDataObjectWatcher>;
using RootPropertyMap = PropertyMap<RootProperty, sRootPropertyBimap>;

extern Bimap<int, std::string> sScreenModeBimap;
extern Bimap<int, std::string> sAnimationQualityBimap;

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
        /// Mark EditText component dirty if text updated
        kExperimentalFeatureMarkEditTextDirtyOnUpdate,
        /// Manage media request status in core and update dirty properties
        kExperimentalFeatureManageMediaRequests,
        /// Enable the experimental API for Alexa Extensions, in addition to existing extension api.
        /// The viewhost is responsible for configuring a ExtensionProvider. All extensions registered through
        /// the ExtensionProvider will have messages mediated by the framework, reducing the burden on the runtime
        /// to be a go-between.
        kExperimentalFeatureExtensionProvider,
        /// Focus EditText component on tap
        kExperimentalFeatureFocusEditTextOnTap,
        /// Send even when core assumes keyboard input is required
        kExperimentalFeatureRequestKeyboard,
    };

    /**
     * @return RootConfig instance.
     */
    static RootConfigPtr create() { return std::make_shared<RootConfig>(); }

    /**
     * Default constructor. Use create() instead.
     */
    RootConfig();

    /**
     * Set RootConfig property
     * @param name Property name,
     * @param object property value
     * @return This object for chaining
     */
    RootConfig& set(const std::string& name, const Object& object);

    /**
     * Set RootConfigProperty.
     * @param key property key
     * @param object property value
     * @return This object for chaining
     */
    RootConfig& set(RootProperty key, const Object& object);

    /**
     * Set a dictionary of values.
     * @param values dictionary from RootProperty to setting value
     * @return This object for chaining
     */
    RootConfig& set(const std::map<RootProperty, Object>& values);

    /**
     * @return Reference to evaluation context that can be used outside of data-binding context.
     */
    const Context& evaluationContext();

    /**
     * Configure the agent name and version
     * @deprecated Use  set({
            {RootProperty::kAgentName, agentName},
            {RootProperty::kAgentVersion, agentVersion}
        }) instead
     * @param agentName
     * @param agentVersion
     * @return This object for chaining
     */
    RootConfig& agent(const std::string& agentName, const std::string& agentVersion) {
        return set({
            {RootProperty::kAgentName, agentName},
            {RootProperty::kAgentVersion, agentVersion}
        });
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
     * Specify the media manager used for loading images, videos, and vector graphics.
     * @param mediaManager The media manager object.
     * @return This object for chaining.
     */
    RootConfig& mediaManager(const MediaManagerPtr& mediaManager) {
        mMediaManager = mediaManager;
        return *this;
    }

    /**
     * Specify the media player factory used for creating media players for video
     * @param mediaPlayerFactory The media player factory object.
     * @return This object for chaining
     */
    RootConfig& mediaPlayerFactory(const MediaPlayerFactoryPtr& mediaPlayerFactory) {
        mMediaPlayerFactory = mediaPlayerFactory;
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
     * Specify the runtime specific locale methods.
     * @param localeMethods The runtime implemented localeMethods
     * @return This object for chaining.
     */
    RootConfig& localeMethods(const std::shared_ptr<LocaleMethods>& localeMethods) {
        mLocaleMethods = localeMethods;
        return *this;
    }

    /**
     * Set if the OpenURL command is supported
     * @deprecated Use set(RootProperty::kAllowOpenUrl, allowed) instead
     * @param allowed If true, the OpenURL command is supported.
     * @return This object for chaining
     */
    RootConfig& allowOpenUrl(bool allowed) {
        return set(RootProperty::kAllowOpenUrl, allowed);
    }

    /**
     * Set if video is supported
     * @deprecated Use set(RootProperty::kDisallowVideo, disallowed) instead
     * @param disallowed If true, the Video component is disabled
     * @return This object for chaining
     */
    RootConfig& disallowVideo(bool disallowed) {
        return set(RootProperty::kDisallowVideo, disallowed);
    }

    /**
     * Set the quality of animation expected.  If set to kAnimationQualityNone,
     * all animation commands are disabled (include onMount).
     * @deprecated Use set(RootProperty::kAnimationQuality, quality) instead
     * @param quality The expected quality of animation playback.
     * @return This object for chaining
     */
    RootConfig& animationQuality(AnimationQuality quality) {
        return set(RootProperty::kAnimationQuality, quality);
    }

    /**
     * Set the default idle timeout.
     * @deprecated Use set(RootProperty::kDefaultIdleTimeout, idleTimeout) instead
     * @param idleTimeout Device wide idle timeout.
     * @return This object for chaining
     */
    RootConfig& defaultIdleTimeout(int idleTimeout) {
        return set(RootProperty::kDefaultIdleTimeout, idleTimeout);
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
     * @deprecated Use set(RootProperty::kReportedVersion, version) instead
     * @param version The version string to report.
     * @return This object for chaining.
     */
    RootConfig& reportedAPLVersion(const std::string& version) {
        return set(RootProperty::kReportedVersion, version);
    }

    /**
     * Sets whether the "type" field of an APL document should be enforced.
     * Type should always be "APL", but for backwards compatibility, this is
     * optionally ignored.
     * @deprecated Use set(RootProperty::kEnforceTypeField, enforce) instead
     * @param enforce `true` to enforce that the "type" field is set to "APL"
     * @return This object for chaining
     */
    RootConfig& enforceTypeField(bool enforce) {
        return set(RootProperty::kEnforceTypeField, enforce);
    }

    /**
     * Set the default font color. This is the fallback color for all themes.
     * This color will only be applied if there is not a theme-defined default color.
     * @deprecated Use set(RootProperty::kDefaultFontColor, color) instead
     * @param color The font color
     * @return This object for chaining
     */
    RootConfig& defaultFontColor(Color color) {
        return set(RootProperty::kDefaultFontColor, color);
    }

    /**
     * Set the default font color for a particular theme.
     * @deprecated Use set(RootProperty::kAllowOpenUrl, allowed) instead
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
     * @deprecated Use set(RootProperty::kDefaultFontFamily, fontFamily) instead
     * @param fontFamily The font family.
     * @return This object for chaining.
     */
    RootConfig& defaultFontFamily(const std::string& fontFamily) {
        return set(RootProperty::kDefaultFontFamily, fontFamily);
    }

    /**
     * Enable or disable tracking of resource, style, and component provenance
     * @deprecated Use set(RootProperty::kTrackProvenance, trackProvenance) instead
     * @param trackProvenance True if provenance should be tracked.
     * @return This object for chaining.
     */
    RootConfig& trackProvenance(bool trackProvenance) {
        return set(RootProperty::kTrackProvenance, trackProvenance);
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
     * @deprecated Use set(RootProperty::kPagerChildCache, cache) instead
     * @param cache Number of pages to ensure before and after current one.
     * @return This object for chaining.
     */
    RootConfig& pagerChildCache(int cache) {
        return set(RootProperty::kPagerChildCache, cache);
    }

    /**
     * Set sequence layout cache in both directions. 1 is default and results in 1 page of children ensured before and
     * one after current one.
     * @deprecated Use set(RootProperty::kSequenceChildCache, cache) instead
     * @param cache Number of pages to ensure before and after current one.
     * @return This object for chaining.
     */
    RootConfig& sequenceChildCache(int cache) {
        return set(RootProperty::kSequenceChildCache, cache);
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
    RootConfig& session(const SessionPtr& session);

    /**
     * Set the current UTC time in milliseconds since the epoch.
     * @deprecated Use set(RootProperty::kUTCTime, time) instead
     * @param time Milliseconds.
     * @return This object for chaining.
     */
    RootConfig& utcTime(apl_time_t time) {
        return set(RootProperty::kUTCTime, time);
    }

    /**
     * Set the local time zone adjustment in milliseconds.  When added to the current UTC time,
     * this will give the local time. This includes any daylight saving time adjustment.
     * @deprecated Use set(RootProperty::kLocalTimeAdjustment, adjustment) instead
     * @param adjustment Milliseconds
     * @return This object for chaining
     */
    RootConfig& localTimeAdjustment(apl_duration_t adjustment) {
        return set(RootProperty::kLocalTimeAdjustment, adjustment);
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
     * @return This object for chaining
     */
    RootConfig& liveDataWatcher(const std::string& name, const std::shared_ptr<LiveDataObjectWatcher>& watcher) {
        mLiveDataObjectWatchersMap.emplace(name, watcher);
        return *this;
    }

#ifdef ALEXAEXTENSIONS
    /**
     * Assign a Alexa Extension provider.
     * Requires kExperimentalExtensionProvider feature be enabled.
     *
     * @param extensionProvider Enables access to the system available extensions.
     * @return This object for chaining
     */
    RootConfig& extensionProvider(const alexaext::ExtensionProviderPtr& extensionProvider) {
        if (experimentalFeatureEnabled(kExperimentalFeatureExtensionProvider)) {
            mExtensionProvider = extensionProvider;
        }
        return *this;
    }

    /**
     * Assign a Alexa Extension mediator.
     * Requires kExperimentalFeatureExtensionProvider feature be enabled.
     *
     * IMPORTANT: ExtensionMediator is a class that is expected to be eliminated.  It
     * can only be used with a single document/RootContext.  It is expected the viewhost call ExtensionMediator.loadExtensions()
     * prior to calling RootContext::create(). RootContext will bind to the mediator obtained from this assignment.
     *
     * @param extensionMediator Message mediator manages messages between Extension and APL engine.
     * and the APL engine.
     * @return This object for chaining
     */
    RootConfig& extensionMediator(const ExtensionMediatorPtr &extensionMediator) {
        if (experimentalFeatureEnabled(kExperimentalFeatureExtensionProvider)) {
            mExtensionMediator = extensionMediator;
        }
        return *this;
    }
#endif

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
     * Register an extension component that can be inflated in the document.  The name should be something
     * like 'DomainCanvas'.
     * This method will also register the extension as a supported extension.
     * @param componentDef The definition of the custom component (includes the name, URI, etc).
     * @return This object for chaining
     */
    RootConfig& registerExtensionComponent(ExtensionComponentDefinition componentDef) {
        auto uri = componentDef.getURI();
        if (mSupportedExtensions.find(uri) == mSupportedExtensions.end()) {
            registerExtension(uri);
        }
        mExtensionComponentDefinitions.emplace_back(std::move(componentDef));
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
     * Register a supported extension. Any previously registered configuration is overwritten.
     * @param uri The URI of the extension
     * @param optional configuration value(s) supported by this extension.
     * @return This object for chaining
     */
    RootConfig& registerExtension(const std::string& uri, const Object& config = Object::TRUE_OBJECT()) {
        if (!config.truthy()) {
            mSupportedExtensions[uri] = Object::TRUE_OBJECT();
        } else {
            mSupportedExtensions[uri] = config;
        }
        return *this;
    }

    /**
     * Register a flags for an extension.  Flags are opaque data provided by the execution environment
     * (not the document) and passed to the extension at the beginning of the document session.
     * Flags may be a single non-null value, array, or key-value bag.
     * @param uri The URI of the extension
     * @param flags The extension flags
     * @return This object for chaining
     */
    RootConfig& registerExtensionFlags(const std::string& uri, const Object& flags) {
        if (mSupportedExtensions.find(uri) == mSupportedExtensions.end()) {
            registerExtension(uri);
        }
        mExtensionFlags[uri] = flags;
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
     * @deprecated Use set(RootProperty::kDoublePressTimeout, timeout) instead
     * @param timeout new double press timeout. Default is 500 ms.
     * @return This object for chaining
     */
    RootConfig& doublePressTimeout(apl_duration_t timeout) {
        return set(RootProperty::kDoublePressTimeout, timeout);
    }

    /**
     * Set long press timeout.
     * @deprecated Use set(RootProperty::kLongPressTimeout, timeout) instead
     * @param timeout new long press timeout. Default is 1000 ms.
     * @return This object for chaining
     */
    RootConfig& longPressTimeout(apl_duration_t timeout) {
        return set(RootProperty::kLongPressTimeout, timeout);
    }

    /**
     * Set pressed duration timeout.  This is the duration to show the "pressed" state of a component
     * when programmatically invoked.
     * @deprecated Use set(RootProperty::kPressedDuration, timeout) instead
     * @param timeout Duration in milliseconds.  Default is 64 ms.
     * @return This object for chaining
     */
    RootConfig& pressedDuration(apl_duration_t timeout) {
        return set(RootProperty::kPressedDuration, timeout);
    }

    /**
     * Set the tap or scroll timeout.  This is the maximum amount of time that can pass before the
     * system has to commit to this being a touch event instead of a scroll event.
     * @deprecated Use set(RootProperty::kTapOrScrollTimeout, timeout) instead
     * @param timeout Duration in milliseconds.  Default is 100 ms.
     * @return This object for chaining
     */
    RootConfig& tapOrScrollTimeout(apl_duration_t timeout) {
        return set(RootProperty::kTapOrScrollTimeout, timeout);
    }

    /**
     * Set SwipeAway gesture trigger distance threshold in dp. Initial movement below this threshold does not trigger the
     * gesture.
     * @deprecated see RootConfig::pointerSlopThreshold. Swiping is not that different from scrolling.
     * @param distance threshold distance.
     * @return This object for chaining
     */
    RootConfig& swipeAwayTriggerDistanceThreshold(float distance) {
        return set(RootProperty::kPointerSlopThreshold, distance);
    }

    /**
     * Set SwipeAway gesture fulfill distance threshold in percents. Gesture requires swipe to be performed above this
     * threshold for it to be considered complete.
     * @deprecated Use set(RootProperty::kSwipeAwayFulfillDistancePercentageThreshold, distance) instead
     * @param distance threshold distance.
     * @return This object for chaining
     */
    RootConfig& swipeAwayFulfillDistancePercentageThreshold(float distance) {
        return set(RootProperty::kSwipeAwayFulfillDistancePercentageThreshold, distance);
    }

    /**
     * Set SwipeAway gesture animation easing.
     * @deprecated Use set(RootProperty::kSwipeAwayAnimationEasing, easing) instead
     * @param easing Easing string to use for gesture animation. Should be according to current APL spec.
     * @return This object for chaining
     */
    RootConfig& swipeAwayAnimationEasing(const std::string& easing) {
        return set(RootProperty::kSwipeAwayAnimationEasing, easing);
    }

    /**
     * Set SwipeAway (and any related gesture) gesture swipe speed threshold.
     * @deprecated Use set(RootProperty::kSwipeVelocityThreshold, velocity) instead
     * @param velocity swipe velocity threshold in dp per second.
     * @return This object for chaining
     */
    RootConfig& swipeVelocityThreshold(float velocity) {
        return set(RootProperty::kSwipeVelocityThreshold, velocity);
    }

    /**
     * Set maximum SwipeAway (and any related gesture) gesture swipe speed.
     * @deprecated Use set(RootProperty::kSwipeMaxVelocity, velocity) instead
     * @param velocity max swipe velocity in dp per second.
     * @return This object for chaining
     */
    RootConfig& swipeMaxVelocity(float velocity) {
        return set(RootProperty::kSwipeMaxVelocity, velocity);
    }

    /**
     * Set SwipeAway gesture tolerance in degrees when determining whether a swipe was triggered.
     * @deprecated Use set(RootProperty::kSwipeAngleTolerance, degrees) instead
     * @param degrees swipe direction tolerance, in degrees.
     * @return This object for chaining
     */
    RootConfig& swipeAngleTolerance(float degrees) {
        return set(RootProperty::kSwipeAngleTolerance, degrees);
    }

    /**
     * Set default animation duration, in ms, for SwipeAway animations.
     * @deprecated Use set(RootProperty::kDefaultSwipeAnimationDuration, duration) instead
     * @param duration the default duration for animations, in ms.
     * @return This object for chaining
     */
    RootConfig& defaultSwipeAnimationDuration(apl_duration_t duration) {
        return set(RootProperty::kDefaultSwipeAnimationDuration, duration);
    }

    /**
     * Set max animation duration, in ms, for SwipeAway animations.
     * @deprecated Use set(RootProperty::kMaxSwipeAnimationDuration, duration) instead
     * @param duration the maximum duration for animations, in ms.
     * @return This object for chaining
     */
    RootConfig& maxSwipeAnimationDuration(apl_duration_t duration) {
        return set(RootProperty::kMaxSwipeAnimationDuration, duration);
    }

    /**
     * Set the fling velocity threshold.  The user must fling at least this fast to start a fling action.
     * @deprecated Use set(RootProperty::kMinimumFlingVelocity, velocity) instead
     * @param velocity Fling velocity in dp per second.
     * @return This object for chaining
     */
    RootConfig& minimumFlingVelocity(float velocity) {
        return set(RootProperty::kMinimumFlingVelocity, velocity);
    }

    /**
     * Set a tick handler update limit in ms. Default is 16ms (60 FPS).
     * @deprecated Use set(RootProperty::kTickHandlerUpdateLimit, updateLimit) instead
     * @param updateLimit update limit in ms. Should be > 0.
     * @return  This object for chaining
     */
    RootConfig& tickHandlerUpdateLimit(apl_duration_t updateLimit) {
        return set(RootProperty::kTickHandlerUpdateLimit, updateLimit);
    }

    /**
     * Set the requested font scaling factor for the document.
     * @deprecated Use set(RootProperty::kFontScale, scale) instead
     * @param scale The scaling factor.  Default is 1.0
     * @return This object for chaining
     */
    RootConfig& fontScale(float scale) {
        return set(RootProperty::kFontScale, scale);
    }

    /**
     * Set the screen display mode for accessibility (normal or high-contrast)
     * @deprecated Use set(RootProperty::kScreenMode, screenMode) instead
     * @param screenMode The screen display mode
     * @return This object for chaining
     */
    RootConfig& screenMode(ScreenMode screenMode) {
        return set(RootProperty::kScreenMode, screenMode);
    }

    /**
     * Inform that a screen reader is turned on.
     * @deprecated Use set(RootProperty::kScreenReader, enabled) instead
     * @param enabled True if the screen reader is enabled
     * @return This object for chaining
     */
    RootConfig& screenReader(bool enabled) {
        return set(RootProperty::kScreenReader, enabled);
    }

    /**
     * Set pointer inactivity timeout. Pointer considered stale after pointer was not updated for this time.
     * @deprecated Use set(RootProperty::kPointerInactivityTimeout, timeout) instead
     * @param timeout inactivity timeout in ms.
     * @return This object for chaining
     */
    RootConfig& pointerInactivityTimeout(apl_duration_t timeout) {
        return set(RootProperty::kPointerInactivityTimeout, timeout);
    }

    /**
     * Set fling gestures velocity limit.
     * @deprecated Use set(RootProperty::kMaximumFlingVelocity, velocity) instead
     * @param velocity fling gesture velocity in dp per second.
     * @return This object for chaining
     */
    RootConfig& maximumFlingVelocity(float velocity) {
        return set(RootProperty::kMaximumFlingVelocity, velocity);
    }

    /**
     * Set the gesture distance threshold in dp. Initial movement below this threshold does not trigger
     * gestures.
     * @deprecated Use set(RootProperty::kPointerSlopThreshold, distance) instead
     * @param distance threshold distance.
     * @return This object for chaining
     */
    RootConfig& pointerSlopThreshold(float distance) {
        return set(RootProperty::kPointerSlopThreshold, distance);
    }

    /**
     * Set scroll commands duration.
     * @deprecated Use set(RootProperty::kScrollCommandDuration, duration) instead
     * @param duration duration in ms.
     * @return This object for chaining
     */
    RootConfig& scrollCommandDuration(apl_duration_t duration) {
        return set(RootProperty::kScrollCommandDuration, duration);
    }

    /**
     * Set scroll snap duration.
     * @deprecated Use set(RootProperty::kScrollSnapDuration, duration) instead
     * @param duration duration in ms.
     * @return This object for chaining
     */
    RootConfig& scrollSnapDuration(apl_duration_t duration) {
        return set(RootProperty::kScrollSnapDuration, duration);
    }

    /**
     * Set default pager page switch animation duration.
     * @deprecated Use set(RootProperty::kDefaultPagerAnimationDuration, duration) instead
     * @param duration duration in ms.
     * @return This object for chaining
     */
    RootConfig& defaultPagerAnimationDuration(apl_duration_t duration) {
        return set(RootProperty::kDefaultPagerAnimationDuration, duration);
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
     * Get RootConfig property
     * @param key property key
     * @return property value
     */
    Object getProperty(RootProperty key) const;

    /**
     * @return The configured text measurement object.
     */
    TextMeasurementPtr getMeasure() const { return mTextMeasurement; }

    /**
     * @return The configured media manager object
     */
     MediaManagerPtr getMediaManager() const { return mMediaManager; }

     /**
      * @return The configured media player factory
      */
     MediaPlayerFactoryPtr getMediaPlayerFactory() const { return mMediaPlayerFactory; }

    /**
     * @return The time manager object
     */
    std::shared_ptr<TimeManager> getTimeManager() const { return mTimeManager; }

    /**
     * @return The locale methods object
     */
    std::shared_ptr<LocaleMethods> getLocaleMethods() const { return mLocaleMethods; }

    /**
     * @return The agent name string
     */
    std::string getAgentName() const { return getProperty(RootProperty::kAgentName).getString(); }

    /**
     * @return The agent version string
     */
    std::string getAgentVersion() const { return getProperty(RootProperty::kAgentVersion).getString(); }

    /**
     * @return The expected animation quality
     */
    AnimationQuality getAnimationQuality() const {
        return static_cast<AnimationQuality>(getProperty(RootProperty::kAnimationQuality).getInteger());
    }

    /**
     * @return The string name of the current animation quality
     */
    const char* getAnimationQualityString() const;

    /**
     * @return True if the OpenURL command is supported
     */
    bool getAllowOpenUrl() const { return getProperty(RootProperty::kAllowOpenUrl).getBoolean(); }

    /**
     * @return True if the video component is not supported.
     */
    bool getDisallowVideo() const { return getProperty(RootProperty::kDisallowVideo).getBoolean(); }

    /**
     * @return Time in ms for default IdleTimeout value.
     */
    int getDefaultIdleTimeout() const { return getProperty(RootProperty::kDefaultIdleTimeout).getInteger(); };

    /**
     * @return The version or versions of the specification that should be enforced.
     */
    APLVersion getEnforcedAPLVersion() const { return mEnforcedAPLVersion; }

    /**
     * @return The reported version of APL used during document inflation
     */
    std::string getReportedAPLVersion() const { return getProperty(RootProperty::kReportedVersion).getString(); }

    /**
     * @return true if the type field value of an APL doc should be enforced
     */
    bool getEnforceTypeField() const { return getProperty(RootProperty::kEnforceTypeField).getBoolean(); }

    /**
     * @return The default font color
     */
    Color getDefaultFontColor(const std::string& theme) const {
        auto it = mDefaultThemeFontColor.find(theme);
        if (it != mDefaultThemeFontColor.end())
            return it->second;
        return getProperty(RootProperty::kDefaultFontColor).getColor();
    }

    /**
     * @return The default text highlight color
     */
    Color getDefaultHighlightColor(const std::string& theme) const {
        auto it = mDefaultThemeHighlightColor.find(theme);
        if (it != mDefaultThemeHighlightColor.end())
            return it->second;
        return getProperty(RootProperty::kDefaultHighlightColor).getColor();
    }

    /**
     * @return The default font family
     */
    std::string getDefaultFontFamily() const { return getProperty(RootProperty::kDefaultFontFamily).getString(); }

    /**
     * @return True if provenance of resources, styles, and components will be calculated.
     */
    bool getTrackProvenance() const { return getProperty(RootProperty::kTrackProvenance).getBoolean(); }

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
    int getPagerChildCache() const { return getProperty(RootProperty::kPagerChildCache).getInteger(); }

    /**
     * Return number of pages to ensure around current one.
     * @return number of pages.
     */
    int getSequenceChildCache() const { return getProperty(RootProperty::kSequenceChildCache).getInteger(); }

    /**
     * @return The current session pointer
     */
    SessionPtr getSession() const { return mSession; }

    /**
     * @return The starting UTC time in milliseconds past the epoch.
     */
    apl_time_t getUTCTime() const { return getProperty(RootProperty::kUTCTime).getDouble(); }

    /**
     * @return The local time zone adjustment. This is the duration in milliseconds, which added
     *         to the current time in UTC gives the local time.  This includes any daylight saving
     *         adjustment.
     */
    apl_duration_t getLocalTimeAdjustment() const { return getProperty(RootProperty::kLocalTimeAdjustment).getDouble(); }

    /**
     * @return A reference to the map of live data sources
     */
    const std::map<std::string, LiveObjectPtr>& getLiveObjectMap() const { return mLiveObjectMap; }

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

#ifdef ALEXAEXTENSIONS
    /**
     * Requires kExperimentalExtensionProvider.
     * @return The extension provider.
     */
    const alexaext::ExtensionProviderPtr getExtensionProvider() const {
        return mExtensionProvider;
    }

    /**
     * Requires kExperimentalExtensionProvider.
     * @return The extension mediator.
     */
    const ExtensionMediatorPtr getExtensionMediator() const {
        return mExtensionMediator;
    }
#endif

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
     * @return The registered extension components
     */
    const std::vector<ExtensionComponentDefinition>& getExtensionComponentDefinitions() const {
        return mExtensionComponentDefinitions;
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
    const Object& getExtensionEnvironment(const std::string& uri) const {
        auto it = mSupportedExtensions.find(uri);
        if (it != mSupportedExtensions.end())
            return it->second;
        return Object::NULL_OBJECT();
    }

    /**
     * @param uri The URI of the extension.
     * @return The registered extension flags, NULL_OBJECT if no flags are registered.
     */
    const Object& getExtensionFlags(const std::string& uri) const {
        auto it = mExtensionFlags.find(uri);
        if (it != mExtensionFlags.end())
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
        return getProperty(RootProperty::kDoublePressTimeout).getDouble();
    }

    /**
     * @return Long press timeout in milliseconds.
     */
    apl_duration_t getLongPressTimeout() const {
        return getProperty(RootProperty::kLongPressTimeout).getDouble();
    }

    /**
     * @return Duration to show the "pressed" state of a component when programmatically invoked
     */
    apl_duration_t getPressedDuration() const {
        return getProperty(RootProperty::kPressedDuration).getDouble();
    }

    /**
     * @return Maximum time to wait before deciding that a touch event starts a scroll or paging gesture.
     */
    apl_duration_t getTapOrScrollTimeout() const {
        return getProperty(RootProperty::kTapOrScrollTimeout).getDouble();
    }

    /**
     * @return SwipeAway trigger distance threshold.
     */
    float getSwipeAwayTriggerDistanceThreshold() const {
        return getProperty(RootProperty::kPointerSlopThreshold).getDouble();
    }

    /**
     * @return SwipeAway fulfill distance threshold in percents.
     */
    float getSwipeAwayFulfillDistancePercentageThreshold() const {
        return getProperty(RootProperty::kSwipeAwayFulfillDistancePercentageThreshold).getDouble();
    }

    /**
     * @return Animation easing for SwipeAway gesture.
     */
    EasingPtr getSwipeAwayAnimationEasing() const {
        return getProperty(RootProperty::kSwipeAwayAnimationEasing).getEasing();
    }

    /**
     * @return Swipe velocity threshold.
     */
    float getSwipeVelocityThreshold() const {
        return getProperty(RootProperty::kSwipeVelocityThreshold).getDouble();
    }

    /**
     * @return Maximum swipe velocity.
     */
    float getSwipeMaxVelocity() const {
        return getProperty(RootProperty::kSwipeMaxVelocity).getDouble();
    }

    /**
     * @return Max allowed pointer movement angle during swipe gestures, as a slope.
     */
    float getSwipeAngleSlope() const {
        return getProperty(RootProperty::kSwipeAngleTolerance).getDouble();
    }

    /**
     * @return Default animation duration for SwipeAway gestures, in ms.
     */
    apl_duration_t getDefaultSwipeAnimationDuration() const {
        return getProperty(RootProperty::kDefaultSwipeAnimationDuration).getDouble();
    }

    /**
     * @return Max animation duration for SwipeAway gestures, in ms.
     */
    apl_duration_t getMaxSwipeAnimationDuration() const {
        return getProperty(RootProperty::kMaxSwipeAnimationDuration).getDouble();
    }

    /**
     * @return Fling velocity threshold
     */
    float getMinimumFlingVelocity() const {
        return getProperty(RootProperty::kMinimumFlingVelocity).getDouble();
    }

    /**
     * @return Tick handler update limit.
     */
    apl_duration_t getTickHandlerUpdateLimit() const {
        return getProperty(RootProperty::kTickHandlerUpdateLimit).getDouble();
    }

    /**
     * @return The requested scaling factor for fonts
     */
    float getFontScale() const {
        return getProperty(RootProperty::kFontScale).getDouble();
    }

    /**
     * @return The current screen mode (high-contrast or normal)
     */
    const char* getScreenMode() const {
        return sScreenModeBimap.at(getScreenModeEnumerated()).c_str();
    }

    /**
     * @return Screen mode as an enumerated value
     */
    ScreenMode getScreenModeEnumerated() const {
        return static_cast<ScreenMode>(getProperty(RootProperty::kScreenMode).getInteger());
    }

    /**
     * @return True if an accessibility screen reader is enabled
     */
    bool getScreenReaderEnabled() const {
        return getProperty(RootProperty::kScreenReader).getBoolean();
    }

    /**
     * @return Pointer inactivity timeout.
     */
    apl_duration_t getPointerInactivityTimeout() const {
        return getProperty(RootProperty::kPointerInactivityTimeout).getDouble();
    }

    /**
     * @return Maximum fling speed.
     */
    float getMaximumFlingVelocity() const {
        return getProperty(RootProperty::kMaximumFlingVelocity).getDouble();
    }

    /**
     * @return Pointer slop threshold.
     */
    float getPointerSlopThreshold() const {
        return getProperty(RootProperty::kPointerSlopThreshold).getDouble();
    }

    /**
     * @return Scroll command duration.
     */
    apl_duration_t getScrollCommandDuration() const {
        return getProperty(RootProperty::kScrollCommandDuration).getDouble();
    }

    /**
     * @return Scroll snap duration.
     */
    apl_duration_t getScrollSnapDuration() const {
        return getProperty(RootProperty::kScrollSnapDuration).getDouble();
    }

    /**
     * @return Default pager animation duration.
     */
    apl_duration_t getDefaultPagerAnimationDuration() const {
        return getProperty(RootProperty::kDefaultPagerAnimationDuration).getDouble();
    }

    /**
     * @param feature feature to check.
     * @return true if feature enabled, false otherwise.
     */
    bool experimentalFeatureEnabled(ExperimentalFeature feature) const {
        return mEnabledExperimentalFeatures.count(feature) > 0;
    }

private:
    const RootPropDefSet& propDefSet() const;

    ContextPtr mContext;

    TextMeasurementPtr mTextMeasurement;
    MediaManagerPtr mMediaManager;
    MediaPlayerFactoryPtr mMediaPlayerFactory;
    std::shared_ptr<TimeManager> mTimeManager;
    std::shared_ptr<LocaleMethods> mLocaleMethods;
    std::map<std::pair<ComponentType, bool>, std::pair<Dimension, Dimension>> mDefaultComponentSize;

    SessionPtr mSession;
    ObjectMap mEnvironmentValues;

    // Data sources and live maps
    std::map<std::string, DataSourceProviderPtr> mDataSources;
    std::map<std::string, LiveObjectPtr> mLiveObjectMap;
    std::multimap<std::string, LiveDataObjectWatcherWPtr> mLiveDataObjectWatchersMap;

    // Extensions
#ifdef ALEXAEXTENSIONS
    alexaext::ExtensionProviderPtr mExtensionProvider;
    ExtensionMediatorPtr mExtensionMediator;
#endif
    ObjectMap mSupportedExtensions; // URI -> config
    ObjectMap mExtensionFlags; // URI -> opaque flags
    std::vector<ExtensionEventHandler> mExtensionHandlers;
    std::vector<ExtensionCommandDefinition> mExtensionCommands;
    std::vector<ExtensionFilterDefinition> mExtensionFilters;
    std::vector<ExtensionComponentDefinition> mExtensionComponentDefinitions;

    std::map<std::string, Color> mDefaultThemeFontColor = {{"light", 0x1e2222ff}, {"dark", 0xfafafaff}};
    std::map<std::string, Color> mDefaultThemeHighlightColor = {{"light", 0x0070ba4d}, {"dark",  0x00caff4d}};

    APLVersion mEnforcedAPLVersion = APLVersion::kAPLVersionIgnore;

    // Set of enabled experimental features
    std::set<ExperimentalFeature> mEnabledExperimentalFeatures;
    RootPropertyMap mProperties;
};

}

#endif //_APL_ROOT_CONFIG_H
