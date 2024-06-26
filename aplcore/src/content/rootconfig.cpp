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

#include "apl/content/rootconfig.h"

#include <cmath>
#include <set>
#include <vector>

#include "apl/animation/coreeasing.h"
#include "apl/component/textmeasurement.h"
#include "apl/content/configurationchange.h"
#include "apl/content/rootpropdef.h"
#include "apl/document/displaystate.h"
#include "apl/engine/context.h"
#include "apl/media/coremediamanager.h"
#include "apl/media/mediaplayerfactory.h"
#include "apl/time/coretimemanager.h"
#include "apl/utils/corelocalemethods.h"
#include "apl/utils/session.h"

namespace {

using namespace apl;

const std::vector<RootProperty> sCopyableConfigProperties = {
    RootProperty::kAgentName,
    RootProperty::kAgentVersion,
    RootProperty::kAllowOpenUrl,
    RootProperty::kFontScale,
    RootProperty::kScreenMode,
    RootProperty::kScreenReader,
    RootProperty::kUTCTime,
    RootProperty::kInitialDisplayState,
    RootProperty::kLocalTimeAdjustment,
    RootProperty::kAnimationQuality,
    RootProperty::kReportedVersion,
    RootProperty::kDoublePressTimeout,
    RootProperty::kLongPressTimeout,
    RootProperty::kMinimumFlingVelocity,
    RootProperty::kPressedDuration,
    RootProperty::kTapOrScrollTimeout,
    RootProperty::kMaximumTapVelocity
};

} // unnamed namespace

namespace apl {

static float angleToSlope(float degrees) {
    return std::tan(degrees * M_PI / 180);
}

inline Object asSlope(const Context& context, const Object& object) {
    assert(object.isNumber());
    return angleToSlope(object.getDouble());
}

Bimap<int, std::string> sScreenModeBimap = {
    { RootConfig::kScreenModeNormal, "normal" },
    { RootConfig::kScreenModeHighContrast, "high-contrast" }
};

Bimap<int, std::string> sAnimationQualityBimap = {
    { RootConfig::kAnimationQualityNone,   "none" },
    { RootConfig::kAnimationQualityNormal, "normal" },
    { RootConfig::kAnimationQualitySlow,   "slow" },
};

/**
 * Default media player factory installed in RootConfig.
 * This media player factory does nothing.
 */
class DefaultMediaPlayerFactory : public MediaPlayerFactory {
public:
    MediaPlayerPtr createPlayer(MediaPlayerCallback callback) override {
        return nullptr;
    }
};

RootConfig::RootConfig()
    : mTextMeasurement( TextMeasurement::instance() ),
      mMediaManager(std::static_pointer_cast<MediaManager>(std::make_shared<CoreMediaManager>())),
      mMediaPlayerFactory(std::static_pointer_cast<MediaPlayerFactory>(std::make_shared<DefaultMediaPlayerFactory>())),
      mTimeManager(std::static_pointer_cast<TimeManager>(std::make_shared<CoreTimeManager>(0))),
      mLocaleMethods(std::static_pointer_cast<LocaleMethods>(std::make_shared<CoreLocaleMethods>())),
      mDefaultComponentSize({
          // Set default sizes for components that aren't "auto" width and "auto" height.
        {{kComponentTypeHost, true}, {Dimension(100), Dimension(100)}},
        {{kComponentTypeImage, true}, {Dimension(100), Dimension(100)}},
        {{kComponentTypePager, true}, {Dimension(100), Dimension(100)}},
        {{kComponentTypeScrollView, true}, {Dimension(), Dimension(100)}},
        {{kComponentTypeSequence, true}, {Dimension(), Dimension(100)}},  // Vertical scrolling, height=100dp width=auto
        {{kComponentTypeSequence, false}, {Dimension(100), Dimension()}}, // Horizontal scrolling, height=auto width=100dp
        {{kComponentTypeGridSequence, true}, {Dimension(), Dimension(100)}},  // Vertical scrolling, height=100dp width=auto
        {{kComponentTypeGridSequence, false}, {Dimension(100), Dimension()}}, // Horizontal scrolling, height=auto width=100dp
        {{kComponentTypeVideo, true}, {Dimension(100), Dimension(100)}},
      })
{
    const auto& defSet = propDefSet();
    for (const auto& cpd : defSet) {
        const auto& pd = cpd.second;
        mProperties.set(pd.key, pd.defvalue);
    }
    // Separate session as RootConfig owned by viewhost and errors in it should not be exposed to
    // the skill.
    mConfigSession = makeDefaultSession();
    mContext = Context::createTypeEvaluationContext(*this,
                                                    APLVersion::getDefaultReportedVersionString(),
                                                    mConfigSession);
}

const Context&
RootConfig::evaluationContext()
{
    return *mContext;
}

const RootPropDefSet&
RootConfig::propDefSet() const
{
    static RootPropDefSet sRootProperties = RootPropDefSet()
        .add({
            {RootProperty::kAgentName,                                   "Default agent",                               asString},
            {RootProperty::kAgentVersion,                                "1.0",                                         asString},
            {RootProperty::kAllowOpenUrl,                                false,                                         asBoolean},
            {RootProperty::kDisallowDialog,                              false,                                         asBoolean},
            {RootProperty::kDisallowEditText,                            false,                                         asBoolean},
            {RootProperty::kDisallowVideo,                               false,                                         asBoolean},
            {RootProperty::kAnimationQuality,                            kAnimationQualityNormal,                       sAnimationQualityBimap},
            {RootProperty::kDefaultIdleTimeout,                          30000,                                         asNumber},
            {RootProperty::kReportedVersion,                             APLVersion::getDefaultReportedVersionString(), asString},
            {RootProperty::kEnforceTypeField,                            false,                                         asBoolean},
            {RootProperty::kDefaultFontColor,                            Color(0xfafafaff),                             asColor},
            {RootProperty::kDefaultHighlightColor,                       Color(0x00caff4d),                             asColor},
            {RootProperty::kDefaultFontFamily,                           "sans-serif",                                  asString},
            {RootProperty::kTrackProvenance,                             true,                                          asBoolean},
            {RootProperty::kPagerChildCache,                             1,                                             asInteger},
            {RootProperty::kSequenceChildCache,                          1,                                             asInteger},
            {RootProperty::kUTCTime,                                     0,                                             asNumber},
            {RootProperty::kLang,                                        "",                                            asString},
            {RootProperty::kLayoutDirection,                             kLayoutDirectionLTR,                           sLayoutDirectionMap},
            {RootProperty::kLocalTimeAdjustment,                         0,                                             asNumber},
            {RootProperty::kDoublePressTimeout,                          500,                                           asNumber},
            {RootProperty::kLongPressTimeout,                            1000,                                          asNumber},
            {RootProperty::kPressedDuration,                             64,                                            asNumber},
            {RootProperty::kTapOrScrollTimeout,                          100,                                           asNumber},
            {RootProperty::kSwipeAwayFulfillDistancePercentageThreshold, 0.5,                                           asNumber},
            {RootProperty::kSwipeAwayAnimationEasing,                    CoreEasing::bezier(0,0,0.58,1),                asEasing},
            {RootProperty::kSwipeVelocityThreshold,                      500,                                           asNumber},
            {RootProperty::kSwipeMaxVelocity,                            2000,                                          asNumber},
            {RootProperty::kSwipeAngleTolerance,                         angleToSlope(40),                              asSlope},
            {RootProperty::kDefaultSwipeAnimationDuration,               200,                                           asNumber},
            {RootProperty::kMaxSwipeAnimationDuration,                   400,                                           asNumber},
            {RootProperty::kMinimumFlingVelocity,                        50,                                            asNumber},
            {RootProperty::kMaximumFlingVelocity,                        1200,                                          asNumber},
            {RootProperty::kMaximumTapVelocity,                          50,                                            asNumber},
            {RootProperty::kTickHandlerUpdateLimit,                      16,                                            asPositiveInteger},
            {RootProperty::kFontScale,                                   1.0,                                           asNumber},
            {RootProperty::kScreenMode,                                  kScreenModeNormal,                             sScreenModeBimap},
            {RootProperty::kScreenReader,                                false,                                         asBoolean},
            {RootProperty::kPointerInactivityTimeout,                    200,                                           asNumber},
            {RootProperty::kPointerSlopThreshold,                        40,                                            asNumber},
            {RootProperty::kScrollCommandDuration,                       1000,                                          asNumber},
            {RootProperty::kScrollOnFocusDuration,                       200,                                           asNumber},
            {RootProperty::kScrollSnapDuration,                          500,                                           asNumber},
            {RootProperty::kDefaultPagerAnimationDuration,               600,                                           asNumber},
            {RootProperty::kDefaultPagerAnimationEasing,                 CoreEasing::bezier(.42,0,.58,1),               asEasing},
            {RootProperty::kScrollAngleSlopeVertical,                    angleToSlope(56),                              asSlope},
            {RootProperty::kScrollAngleSlopeHorizontal,                  angleToSlope(33),                              asSlope},
            {RootProperty::kScrollFlingVelocityLimitEasingVertical,      CoreEasing::bezier(.6,.4,.35,.6),              asEasing},
            {RootProperty::kScrollFlingVelocityLimitEasingHorizontal,    CoreEasing::bezier(.42,.66,.5,1),              asEasing},
            {RootProperty::kUEScrollerVelocityEasing,                    CoreEasing::bezier(.25,1,.5,1),                asEasing},
            {RootProperty::kUEScrollerDurationEasing,                    CoreEasing::bezier(.65,0,.35,1),               asEasing},
            {RootProperty::kUEScrollerMaxDuration,                       3000,                                          asNumber},
            {RootProperty::kUEScrollerDeceleration,                      0.175,                                         asNumber},
            {RootProperty::kSendEventAdditionalFlags,                    Object::EMPTY_MAP(),                           asAny},
            {RootProperty::kTextMeasurementCacheLimit,                   500,                                           asInteger},
            {RootProperty::kInitialDisplayState,                         DEFAULT_DISPLAY_STATE,                         sDisplayStateMap},
        });
    return sRootProperties;
}

bool
RootConfig::isAllowedEnvironmentName(const std::string &name) const
{
    static std::set<std::string> sReserved;
    if (sReserved.empty()) {
        // Don't allow custom env properties to shadow synthesized configuration change event props
        for (const auto &synthesizedName : ConfigurationChange::getSynthesizedPropertyNames()) {
            sReserved.emplace(synthesizedName);
        }

        // Check the name against a clean evaluation context
        auto context = Context::createTypeEvaluationContext(RootConfig(),
                                                            APLVersion::getDefaultReportedVersionString(),
                                                            mConfigSession);
        assert(context);

        // Don't allow custom env properties to shadow top-level names (e.g. "environment")
        for (const auto &entry : *context) {
            sReserved.emplace(entry.first);
        }

        // Don't allow custom env properties to shadow built-in environment properties
        auto env = context->opt("environment");
        assert(env.isMap());
        for (const auto &entry : env.getMap()) {
            sReserved.emplace(entry.first);
        }

        // Don't allow custom env properties to shadow built-in viewport properties
        auto viewport = context->opt("viewport");
        assert(viewport.isMap());
        for (const auto &entry : viewport.getMap()) {
            sReserved.emplace(entry.first);
        }
    }

    return sReserved.find(name) == sReserved.end();
}

RootConfig&
RootConfig::session(const SessionPtr& session) {
    return *this;
}

RootConfig&
RootConfig::set(const std::string& name, const Object& object)
{
    auto it = sRootPropertyBimap.find(name);
    if (it != sRootPropertyBimap.endBtoA()) {
        auto propertyKey = static_cast<RootProperty>(it->second);
        return set(propertyKey, object);
    } else {
        LOG(LogLevel::kInfo).session(mConfigSession) << "Unable to find property " << name;
    }

    return *this;
}

RootConfig&
RootConfig::set(RootProperty key, const Object& object)
{
    assert(mContext);
    const auto& pds = propDefSet();
    auto it = pds.find(key);
    if (it != pds.end()) {
        auto pd = it->second;
        if (pd.map && object.isNumber() && pd.map->has(object.getInteger())) {
            mProperties.set(it->first, object.getInteger());
        } else {
            auto val = pd.calculate(*mContext, object);
            mProperties.set(it->first, val);
        }
    }
    return *this;
}

RootConfig&
RootConfig::set(const std::map<RootProperty, Object>& values)
{
    for (auto& kv : values) {
        set(kv.first, kv.second);
    }
    return *this;
}

Object
RootConfig::getProperty(RootProperty key) const
{
    const auto& pds = propDefSet();
    auto it = pds.find(key);
    if (it != pds.end()) {
        return mProperties.get(key);
    }
    return Object::NULL_OBJECT();
}

const char*
RootConfig::getAnimationQualityString() const {
    auto animationQuality = static_cast<AnimationQuality>(getProperty(RootProperty::kAnimationQuality).getInteger());
    auto it = sAnimationQualityBimap.find(animationQuality);
    if (it != sAnimationQualityBimap.end()) {
        return it->second.c_str();
    }
    return "none";
}

RootConfig&
RootConfig::setEnvironmentValue(const std::string& name, const Object& value) {
    if (isAllowedEnvironmentName(name)) {
        mEnvironmentValues[name] = value;
    } else {
        LOG(LogLevel::kWarn).session(mConfigSession) << "Ignoring attempt to set environment value: " << name;
    }
    return *this;
}

/**
 * @return Animation easing for SwipeAway gesture.
 */
EasingPtr
RootConfig::getSwipeAwayAnimationEasing() const {
    return getProperty(RootProperty::kSwipeAwayAnimationEasing).get<Easing>();
}

RootConfigPtr
RootConfig::copy() const
{
    auto copy = std::make_shared<RootConfig>();
    copy->timeManager(getTimeManager());
    copy->audioPlayerFactory(getAudioPlayerFactory());
    copy->documentManager(getDocumentManager());
    copy->mediaPlayerFactory(getMediaPlayerFactory());
    copy->measure(getMeasure());
    copy->experimentalFeatures(getExperimentalFeatures());
    copy->packageManager(getPackageManager());

    for (auto key : sCopyableConfigProperties) {
        copy->set(key, getProperty(key));
    }

    return copy;
}

} // namespace apl
