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

#ifndef APL_DATAURL_GRAMMAR_H
#define APL_DATAURL_GRAMMAR_H

#include <tao/pegtl.hpp>

#include <string>

#include "apl/datagrammar/grammarpolyfill.h"
#include "apl/utils/log.h"

namespace apl {

namespace dataurlgrammar {

/**
* Grammar for verifying data url specified images.
*
* Data url spec: https://datatracker.ietf.org/doc/html/rfc2397
* Base64 syntax: https://datatracker.ietf.org/doc/html/rfc4648#section-4
*/
namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;

struct alnum_character : ranges<'a', 'z', 'A', 'Z', '0', '9'> {};
struct dataprefix : string<'d', 'a', 't', 'a', ':'> {};
struct base64extension : string<';', 'b', 'a', 's', 'e', '6', '4'> {};

// We only allow data urls containing base64, no need to go for full urlchar spec definition.
struct base64char : sor<alnum, one<'/'>, one<'+'>>{};
struct base64data : plus<base64char> {};
struct base64padding : seq<opt<rep_max<2, one<'='>>>>{};
struct base64 : seq<base64data, base64padding> {};

// Also only image types with any subtype. Optional.
struct imagetype : string<'i','m','a','g','e'>{};
struct subtype : plus<alnum_character>{};

// Parameters are optional
struct tspecials : one<'(', ')', '<', '>', '@', ',', ';', ':', '\\', '"',
                               '/', '[', ']', '?', '='>{};
struct token : plus<seq<not_at<tspecials>, not_at<space>, any>>{};
struct parameter_attribute : token{};
struct parameter_value : token{};
struct parameter : seq<one<';'>, parameter_attribute, one<'='>, parameter_value>{};
struct parameters : plus<parameter>{};

struct mediatype : seq<opt<imagetype, one<'/'>, subtype>>{};
struct mediatype_props : seq<mediatype, opt<parameters>>{};
struct dataurl : must<dataprefix, opt<mediatype_props>, base64extension, one<','>, base64, eof>{};

// ******************** Data Structures *********************

struct dataurl_state : fail_state {
    template<typename Input>
    explicit dataurl_state(const Input& in) {}

    std::string data;
    std::string type;
    std::string subtype;
};

// ============== Data URL Rules ===================

template<typename Rule>
struct dataurl_action : pegtl::nothing<Rule> {};

template<>
struct dataurl_action<base64> {
    template<typename Input>
    static void apply(const Input& in, dataurl_state& state)
    {
        if (state.failed) return;

        state.data = in.string();
    }
};

template<>
struct dataurl_action<imagetype> {
    template<typename Input>
    static void apply(const Input& in, dataurl_state& state)
    {
        if (state.failed) return;

        state.type = in.string();
    }
};

template<>
struct dataurl_action<subtype> {
    template<typename Input>
    static void apply(const Input& in, dataurl_state& state)
    {
        if (state.failed) return;

        state.subtype = in.string();
    }
};

// ****************** Error messages *******************

template<typename Rule>
struct dataurl_control : public apl_control<Rule> {
   static const std::string error_message;

   template<typename Input, typename ... States>
   static void raise(const Input& in, States&& ... st)
   {
       apl_control<Rule>::fail(in, st...);
   }
};

} // namespace dataurlgrammar

} // namespace apl

#endif //APL_DATAURL_GRAMMAR_H
