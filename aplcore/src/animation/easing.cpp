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

#include "apl/animation/coreeasing.h"
#include "apl/animation/easinggrammar.h"
#include "apl/utils/session.h"
#include "apl/utils/weakcache.h"

namespace apl {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

// These easing curves are predefined and available at all times
static auto sLinear = CoreEasing::linear();
static auto sEase = CoreEasing::bezier(0.25, 0.10, 0.25, 1);
static auto sEaseIn = CoreEasing::bezier(0.42, 0, 1, 1);
static auto sEaseOut = CoreEasing::bezier(0, 0, 0.58, 1);
static auto sEaseInOut = CoreEasing::bezier(0.42, 0, 0.58, 1);

static WeakCache<Easing> sEasingCache = {
    {"linear",      sLinear},
    {"ease",        sEase},
    {"ease-in",     sEaseIn},
    {"ease-out",    sEaseOut},
    {"ease-in-out", sEaseInOut}
};

static bool sEasingCacheDirty = false;  // Mark the cache as dirty when an easing curve is deleted

EasingPtr
Easing::parse(const SessionPtr& session, const std::string& easing)
{
    // First, remove all of the spaces from the string.  This helps with caching and simplifies the grammar
    std::string s(easing);
    auto end = std::remove(s.begin(), s.end(), ' ');
    s.erase(end, s.end());

    // Check the cache
    auto ptr = sEasingCache.find(s);
    if (ptr)
        return ptr;

    try {
        easinggrammar::easing_state state;
        pegtl::string_input<> in(s, "");
        pegtl::parse<easinggrammar::grammar, easinggrammar::action>(in, state);

        auto easingCurve = CoreEasing::create(std::move(state.segments),
                                              std::move(state.args),
                                              s);
        if (!easingCurve) {
            CONSOLE_S(session) << "Unable to create easing curve " << easing;
        } else {
            if (sEasingCacheDirty) {
                sEasingCache.clean();
                sEasingCacheDirty = false;
            }

            sEasingCache.insert(s, easingCurve);
            return easingCurve;
        }
    }
    catch (pegtl::parse_error& e) {
        CONSOLE_S(session) << "Parse error in " << easing << " - " << e.what();
    }

    return Easing::linear();
}

EasingPtr
Easing::linear() {
    return sLinear;
}

bool
Easing::has(const char *easing)
{
    std::string s(easing);
    auto end = std::remove(s.begin(), s.end(), ' ');
    s.erase(end, s.end());

    return sEasingCache.find(s) != nullptr;
}

Easing::~Easing() noexcept {
    sEasingCacheDirty = true;

}

Object
Easing::call(const ObjectArray& args) const
{
    if (args.size() != 1)
        return 0;

    return const_cast<Easing&>(*this).calc(static_cast<float>(args.at(0).asNumber()));
}

bool
Easing::operator==(const ObjectData& other) const {
    return *this == dynamic_cast<const Easing&>(other);
}

} // namespace apl
