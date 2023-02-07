/*
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
 *
 */

#include <tao/pegtl.hpp>

#include "apl/component/corecomponent.h"
#include "apl/component/selector.h"
#include "apl/datagrammar/grammarpolyfill.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/utils/make_unique.h"

namespace apl {

// ********************* Grammar definition is isolated ***********************

namespace selectorgrammar {

using MatchFunction = std::function<bool(const CoreComponentPtr&)>;

static CoreComponentPtr
findChildFromOffset(const CoreComponentPtr& parent, const MatchFunction& matcher,
                    int start, int delta)
{
    assert(parent);
    const auto len = parent->getChildCount();
    for (int i = start ; i >= 0 && i < len ; i += delta) {
        auto child = parent->getCoreChildAt(i);
        if (matcher(child))
            return child;
    }

    return nullptr;
}

static CoreComponentPtr
findChild(const CoreComponentPtr& parent, const MatchFunction& matcher)
{
    return findChildFromOffset(parent, matcher, 0, 1);
}

static CoreComponentPtr
findSibling(const CoreComponentPtr& component, const MatchFunction& matcher, int offset)
{
    assert(component);
    auto parent = CoreComponent::cast(component->getParent());
    if (!parent)
        return nullptr;

    int index;
    const auto len = parent->getChildCount();
    for (index = 0 ; index < len ; index++) {
        if (parent->getCoreChildAt(index) == component)
            break;
    }

    return findChildFromOffset(parent, matcher, index + offset, offset);
}

static CoreComponentPtr
findNext(const CoreComponentPtr& component, const MatchFunction& matcher)
{
    return findSibling(component, matcher, 1);
}

static CoreComponentPtr
findPrevious(const CoreComponentPtr& component, const MatchFunction& matcher)
{
    return findSibling(component, matcher, -1);
}

/**
 * Iterate up the parent components
 */
static CoreComponentPtr
findParent(const CoreComponentPtr& start, const MatchFunction& matcher)
{
    assert(start);
    auto parent = CoreComponent::cast(start->getParent());
    while (parent) {
        if (matcher(parent))
            return parent;
        parent = CoreComponent::cast(parent->getParent());
    }
    return nullptr;
}

static CoreComponentPtr
findSearch(const CoreComponentPtr& start, const MatchFunction& matcher)
{
    assert(start);
    std::vector<std::pair<CoreComponentPtr, int>> stack = {{start, 0}};

    while (!stack.empty()) {
        auto& back = stack.back();
        const auto len = back.first->getChildCount();
        auto index = back.second;
        if (index < len) {
            auto result = back.first->getCoreChildAt(index);
            if (matcher(result))
                return result;
            back.second += 1;
            stack.emplace_back(std::pair<CoreComponentPtr, int>{result, 0});
        }
        else {
            stack.pop_back();
        }
    }

    return nullptr;
}

enum ModifierType {
    kModifierParent,
    kModifierChild,
    kModifierFind,
    kModifierNext,
    kModifierPrevious
};

enum MatcherType {
    kMatchTypeNumeric,
    kMatchTypeId,
    kMatchTypeType,
};

using ModifierFunction = CoreComponentPtr (*)(const CoreComponentPtr&, const MatchFunction&);
static const ModifierFunction sDispatchFunctions[] = {
    &findParent,
    &findChild,
    &findSearch,
    &findNext,
    &findPrevious
};

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;

static const bool DEBUG_GRAMMAR = false;

struct ws : star<space> {};  // White space

struct str_child    : string<'c','h','i','l','d'> {};
struct str_find     : string<'f','i','n','d'> {};
struct str_next     : string<'n','e','x','t'> {};
struct str_parent   : string<'p','a','r','e','n','t'> {};
struct str_previous : string<'p','r','e','v','i','o','u','s'> {};

struct str_root     : string<':','r','o','o','t'> {};
struct str_source   : string<':','s','o','u','r','c','e'> {};

struct str_id       : string<'i','d'> {};
struct str_type     : string<'t','y','p','e'> {};

struct key_child    : seq<str_child, not_at<identifier_other>> {};
struct key_find     : seq<str_find, not_at<identifier_other>> {};
struct key_next     : seq<str_next, not_at<identifier_other>> {};
struct key_parent   : seq<str_parent, not_at<identifier_other>> {};
struct key_previous : seq<str_previous, not_at<identifier_other>> {};

struct key_id       : seq<str_id, one<'='>> {};
struct key_type     : seq<str_type, one<'='>> {};

struct key_source   : seq<str_source, not_at<identifier_other>> {};
struct key_root     : seq<str_root, not_at<identifier_other>> {};

struct uid          : seq<one<':'>, plus<digit>> {};
struct id           : plus<sor<alnum, one<'-'>, one<'_'>>> {};

struct zero         : if_must<one<'0'>, not_at<digit>> {};
struct number_int   : sor<zero, seq< range<'1', '9'>, star<digit>>> {};
struct number_sign  : sor<one<'-'>,success> {};
struct number       : seq<number_sign, number_int> {};

struct value        : id {};
struct key          : sor<key_id, key_type> {};
struct key_value    : if_must<key, value> {};
struct arg          : sor<number, key_value, success> {};

struct mod_type     : sor<key_child, key_find, key_next, key_parent, key_previous> {};
struct modifier     : if_must<one<':'>, mod_type, one<'('>, arg, one<')'>> {};
struct modifiers    : list<ws, modifier> {};

struct top_id       : id {};
struct element      : sor<key_source, key_root, uid, top_id, success> {};
struct grammar      : must<ws, element, modifiers, eof>{};

// ***************** Error reporting *******************

template<typename Rule>
struct selector_control : apl_control< Rule > {
    static const std::string error_message;

    template<typename Input>
    static void raise( const Input& in, fail_state& state) {
        state.fail(error_message, in);
    }

    template< typename Input>
    static void fail( const Input& in, fail_state& state ) {
        state.fail(error_message, in);
    }
};

template<> const std::string selector_control<one<'('>>::error_message = "Expected a left parenthesis after the modifier name";
template<> const std::string selector_control<one<')'>>::error_message = "Invalid modifier arguments";
template<> const std::string selector_control<mod_type>::error_message = "Invalid modifier name";

// This template will catch any other parsing errors
template<typename T> const std::string selector_control<T>::error_message = "selector error matching " + pegtl::internal::demangle<T>();

// **************** Actions ***********/

template<typename Rule>
struct action : pegtl::nothing< Rule > {
};


struct selector_state : fail_state {
    selector_state(ContextPtr context, CoreComponentPtr target)
        : context(std::move(context)), target(std::move(target)) {}

    ContextPtr context;
    CoreComponentPtr target;

    ModifierType modifierType = kModifierChild;
    MatcherType matcherType = kMatchTypeNumeric;
    std::string value;
    int number = 0;
    bool somethingMatched = false;
};


template<> struct action< number >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        std::string s(in.string());
        auto value = sutil::stoi(s);
        LOGF_IF(DEBUG_GRAMMAR, "Number: '%s' -> %d", in.string().c_str(), value);
        state.number = value;
    }
};

template<> struct action< key_root >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        state.target = CoreComponent::cast(state.context->topComponent());
        state.somethingMatched = true;
        LOG_IF(DEBUG_GRAMMAR) << "Root element";
    }
};

template<> struct action< key_source >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        state.somethingMatched = true;
        LOG_IF(DEBUG_GRAMMAR) << "Source element";
    }
};

template<> struct action< uid >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        state.target = CoreComponent::cast(state.context->findComponentById(in.string()));
        state.somethingMatched = true;
        LOGF_IF(DEBUG_GRAMMAR, "Find UID: '%s'", in.string().c_str());
    }
};

template<> struct action< top_id >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        state.target = CoreComponent::cast(state.context->findComponentById(in.string()));
        state.somethingMatched = true;
        LOGF_IF(DEBUG_GRAMMAR, "Find ID: '%s'", in.string().c_str());
    }
};

template<> struct action< value >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        state.value = in.string();
        LOG_IF(DEBUG_GRAMMAR) << "Value '" << state.value << "'";
    }
};

template<> struct action< key_id >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        state.matcherType = kMatchTypeId;
        LOG_IF(DEBUG_GRAMMAR) << "Match id";
    }
};

template<> struct action< key_type >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        state.matcherType = kMatchTypeType;
        LOG_IF(DEBUG_GRAMMAR) << "Match type";
    }
};

template<> struct action< key_parent >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        state.modifierType = kModifierParent;
        LOG_IF(DEBUG_GRAMMAR) << "Modifier parent";
    }
};

template<> struct action< key_child >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        state.modifierType = kModifierChild;
        LOG_IF(DEBUG_GRAMMAR) << "Modifier child";
    }
};

template<> struct action< key_find >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        state.modifierType = kModifierFind;
        LOG_IF(DEBUG_GRAMMAR) << "Modifier find";
    }
};

template<> struct action< key_next >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        state.modifierType = kModifierNext;
        LOG_IF(DEBUG_GRAMMAR) << "Modifier next";
    }
};

template<> struct action< key_previous >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        state.modifierType = kModifierPrevious;
        LOG_IF(DEBUG_GRAMMAR) << "Modifier previous";
    }
};

/**
 * The component type can match a primitive type (like "Text", "VectorGraphic") or it can match
 * the name of a layout that was inflated and created this component (like "RadioButton").  The
 * data-binding context associated with the component may have "__name" and "__source" properties
 * assigned.  When __source=component, the __name property is the component name (like "Text").
 * When __source=layout, the __name property is the name of the layout.
 *
 * @param component The component whose type is being checked.
 * @param type The desired type string.
 * @return True if this component matches the type or if the layout wrapping this component matches
 *         the type.
 */
static bool matchComponentType(const CoreComponentPtr& component, const std::string& type)
{
    // The first context should have __source=component
    auto context = component->getContext();
    assert(context->opt("__source").asString() == "component");

    // Check the __name property for a quick match
    auto name = context->opt("__name");
    if (name.isString() && name.getString() == type)
        return true;

    // Search upwards through the context for a layout that matches.
    while ((context = context->parent()) != nullptr) {
        auto source = context->opt("__source");
        if (!source.isString())
            return false;

        // This context is connected to a different component.  We are done searching
        if (source.getString() == "component")
            return false;

        // We match if a containing layout has this name
        if (source.getString() == "layout" && context->opt("__name").asString() == type)
            return true;
    }

    return false;
}

template<> struct action< modifier >
{
    template< typename Input >
    static void apply( const Input& in, selector_state& state) {
        if (!state.target)
            return;

        // The :child(NUMBER) match is a special pattern
        if (state.modifierType == kModifierChild &&
            state.matcherType == kMatchTypeNumeric) {
            auto len = static_cast<int>(state.target->getChildCount());
            auto index = state.number < 0 ? state.number + len : state.number;
            state.target = (index < 0 || index >= len) ? nullptr : state.target->getCoreChildAt(index);
        }
        else {
            auto f = sDispatchFunctions[state.modifierType];
            switch (state.matcherType) {
                case kMatchTypeNumeric:
                    state.target = f(state.target, [&](const CoreComponentPtr& component) {
                         return --state.number <= 0;
                    });
                    break;
                case kMatchTypeId:
                    state.target = f(state.target, [&](const CoreComponentPtr& component) {
                        return component->getId() == state.value;
                    });
                    break;
                case kMatchTypeType:
                    state.target = f(state.target, [&](const CoreComponentPtr& component) {
                        return matchComponentType(component, state.value);
                    });
                    break;
            }
        }

        // Clean patterns for the next modifier
        state.matcherType = kMatchTypeNumeric;
        state.value.clear();
        state.number = 0;
        state.somethingMatched = true;
    }
};

} // namespace selectorgrammar

CoreComponentPtr
Selector::resolve(const std::string& string,
                  const ContextPtr& context,
                  const CoreComponentPtr& source)
{
    selectorgrammar::selector_state state(context, source);
    pegtl::string_input<> in(string, "");

    if (!pegtl::parse<selectorgrammar::grammar, selectorgrammar::action, selectorgrammar::selector_control>(in, state) ||
         state.failed ||
        !state.somethingMatched) {
        CONSOLE(context) << "Error parsing selector '" << string << "' " << state.what();
        return nullptr;
    }

    return state.target;
}

} // namespace apl