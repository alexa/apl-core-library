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

#ifndef _APL_GRAMMAR_POLYFILL_H
#define _APL_GRAMMAR_POLYFILL_H

#include <string>
#include <vector>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>
#include <tao/pegtl/contrib/unescape.hpp>

namespace apl {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;

/// Pretty much structure of pegtl::parse_error, without exceptions involved
/// see: https://github.com/taocpp/PEGTL/blob/main/include/tao/pegtl/parse_error.hpp
struct parse_fail
{
    parse_fail() = default;

    parse_fail( const std::string& msg, std::vector< position >&& in_positions )
        : demangled( msg ),
          positions( std::move( in_positions ) )
    {
    }

    template< typename Input >
    parse_fail( const std::string& msg, const Input& in )
        : parse_fail( msg, in.position() )
    {
    }

    parse_fail( const std::string& msg, const position& pos )
        : demangled( to_string( pos ) + ": " + msg ),
          positions( 1, pos )
    {
    }

    parse_fail( const std::string& msg, position&& pos )
        : demangled( to_string( pos ) + ": " + msg )
    {
        positions.emplace_back( std::move( pos ) );
    }

    std::string what() const { return demangled; }

    std::string demangled;
    std::vector< position > positions;
};

/**
 * Base state to be used in case of any "must" rule.
 */
struct fail_state {
    bool failed = false;
    parse_fail internal;

    template<typename Input>
    void fail(const std::string& msg, const Input& in) {
        if (!failed) {
            failed = true;
            internal = {msg, in};
        }
    }

    std::string what() const { return internal.demangled; }
    std::vector<position> positions() const { return internal.positions; }
};

template<typename Rule>
struct apl_control : pegtl::normal< Rule > {
    static const std::string error_message;

    template< typename Input>
    static void fail( const Input& in, fail_state& state ) {
        state.fail(error_message, in);
    }

    template<typename Input, typename...States >
    static void raise( const Input& in, States&&... st) {
        fail(in, st...);
    }
};

template<typename T> const std::string apl_control<T>::error_message = "parse error matching " + pegtl::internal::demangle<T>();

/// "must" polyfills. Based on following:
/// https://github.com/taocpp/PEGTL/blob/main/include/tao/pegtl/internal/raise.hpp
/// https://github.com/taocpp/PEGTL/blob/main/include/tao/pegtl/internal/must.hpp
/// https://github.com/taocpp/PEGTL/blob/main/include/tao/pegtl/internal/if_must.hpp
/// https://github.com/taocpp/PEGTL/blob/main/include/tao/pegtl/internal/list_must.hpp
namespace internal {
    template< typename T >
    struct raise
    {
        template< apply_mode,
                  rewind_mode,
                  template< typename... >
                  class Action,
                  template< typename... >
                  class Control,
                  typename Input,
                  typename... States >
        static bool match( Input& in, States&&... st )
        {
            Control< T >::raise( static_cast< const Input& >( in ), st... );
            return false;
        }
    };

    template< typename... Rules >
    struct must : seq< must< Rules >... >
    {};

    template <typename Rule>
    struct must<Rule> {
        template <apply_mode A, rewind_mode, template <typename...> class Action,
                  template <typename...> class Control, typename Input, typename... States>
        static bool match(Input& in, States&&... st) {
            if (!Control<Rule>::template match<A, rewind_mode::dontcare, Action, Control>(in, st...)) {
                raise< Rule >::template match< A, rewind_mode::dontcare, Action, Control >( in, st... );
                return false;
            }
            return true;
        }
    };

    template< bool Default, typename Cond, typename... Rules >
    struct if_must
    {
        template< apply_mode A,
                  rewind_mode M,
                  template< typename... >
                  class Action,
                  template< typename... >
                  class Control,
                  typename Input,
                  typename... States >
        static bool match( Input& in, States&&... st )
        {
            if( Control< Cond >::template match< A, M, Action, Control >( in, st... ) ) {
                if (Control< must< Rules... > >::template match< A, M, Action, Control >( in, st... )) {
                    return true;
                }
            }
            return Default;
        }
    };

    template< typename Rule, typename Sep >
    using list_must = seq< Rule, star< Sep, must< Rule > > >;
}

template< typename... Rules > struct must : internal::must< Rules... > {};
template< typename Cond, typename... Thens > struct if_must : internal::if_must< false, Cond, Thens... > {};
template< typename Cond, typename... Rules > struct opt_must : internal::if_must< true, Cond, Rules... > {};
template< typename Rule, typename Sep, typename Pad = void > struct list_must : internal::list_must< Rule, tao::pegtl::internal::pad< Sep, Pad > > {};
template< typename Rule, typename Sep > struct list_must< Rule, Sep, void > : internal::list_must< Rule, Sep > {};

} // namespace apl

#endif // _APL_GRAMMAR_POLYFILL_H
