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

#include "apl/animation/easing.h"
#include "apl/animation/easinggrammar.h"
#include "apl/animation/coreeasing.h"
#include "apl/utils/session.h"
#include "apl/utils/streamer.h"

namespace apl {

const int MAX_EASING_CACHE_SIZE = 1000;

static Easing sDefaultEasingCurve(new LinearEasing());

std::map<std::string, EasingCurve *> sEasingCurves = {
    { "linear", new LinearEasing() },
    { "ease", new CubicBezierEasing(0.25, 0.10, 0.25, 1) },
    { "ease-in", new CubicBezierEasing(0.42, 0, 1, 1) },
    { "ease-out", new CubicBezierEasing(0, 0, 0.58, 1) },
    { "ease-in-out", new CubicBezierEasing(0.42, 0, 0.58, 1) }
};

static bool checkPathOrder(const std::vector<float>& vector)
{
    float t = vector.at(0);
    for (int i = 2 ; i < vector.size() ; i+=2) {
        if (vector.at(i) <= t)
            return false;
        t = vector.at(i);
    }
    return true;
}

Easing
Easing::parse(const SessionPtr& session, const std::string& easing) {
    // First, remove all of the spaces from the string.  This helps with caching and simplifies the grammar
    std::string s(easing);
    auto end = std::remove(s.begin(), s.end(), ' ');
    s.erase(end, s.end());

    auto it = sEasingCurves.find(s);
    if (it != sEasingCurves.end())
        return it->second;

    try {
        pegtl::data_parser parser(s, "Easing");
        easinggrammar::easing_state state;
        parser.parse<easinggrammar::grammar, easinggrammar::action>(state);

        switch (state.type) {
            case easinggrammar::easing_state::PATH:
                if (state.args.size() == 0)
                    return sDefaultEasingCurve;

                if (state.args.size() % 2 != 0) {
                    CONSOLE_S(session) << "Path easing function needs an even number of arguments";
                } else {
                    state.args.insert(state.args.begin(), 2, 0);
                    state.args.push_back(1);
                    state.args.push_back(1);

                    if (!checkPathOrder(state.args)) {
                        CONSOLE_S(session) << "Path easing function needs ordered array of arguments";
                    }
                    else {
                        auto easingCurve = new PathEasing(std::move(state.args));
                        if (sEasingCurves.size() < MAX_EASING_CACHE_SIZE)
                            sEasingCurves.emplace(s, easingCurve);
                        return easingCurve;
                    }
                }
                break;
            case easinggrammar::easing_state::CUBIC_BEZIER:
                if (state.args.size() != 4) {
                    CONSOLE_S(session) << "Cubic bezier easing function needs four arguments";
                } else {
                    auto easingCurve = new CubicBezierEasing(state.args.at(0), state.args.at(1), state.args.at(2), state.args.at(3));
                    if (sEasingCurves.size() < MAX_EASING_CACHE_SIZE)
                        sEasingCurves.emplace(s, easingCurve);
                    return easingCurve;
                }
                break;
        }
    }
    catch (pegtl::parse_error e) {
        CONSOLE_S(session) << "Parse error in " << easing << " - " << e.what();
    }

    return sDefaultEasingCurve;
}

bool
Easing::has(const char *easing)
{
    std::string s(easing);
    auto end = std::remove(s.begin(), s.end(), ' ');
    s.erase(end, s.end());

    auto it = sEasingCurves.find(s);
    return it != sEasingCurves.end();
}

Easing
Easing::linear() {
    return sDefaultEasingCurve;
}

float
Easing::operator()(float time) {
    if (time <= 0) return 0;
    if (time >= 1) return 1;
    if (time == mLastTime) return mLastValue;
    mLastTime = time;
    mLastValue = mCurve->calc(time);
    return mLastValue;
}

bool
Easing::operator==(const Easing& rhs)
{
    return mCurve->equal(rhs.mCurve);
}

bool
Easing::operator!=(const Easing& rhs)
{
    return !mCurve->equal(rhs.mCurve);
}

streamer& operator<<(streamer& s, const Easing& easing) {
    s << easing.mCurve->toDebugString();
    return s;
}

} // namespace apl