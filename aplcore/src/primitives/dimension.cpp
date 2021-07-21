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

#include <assert.h>
#include <tao/pegtl.hpp>

#include "apl/primitives/dimension.h"
#include "apl/engine/context.h"

namespace apl {

    namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
    using namespace pegtl;

    // TODO:  Currently if the grammar doesn't match, we set the result to 0.

    /**
     * \cond ShowDimensionGrammar
     */
    struct d_ws      : star< space > {};
    struct d_num     : sor< seq< opt<one<'-'>>, plus< digit >, opt< one<'.'>, star< digit > > >,
                            seq< opt<one<'-'>>, one<'.'>, plus< digit > > > {};
    struct d_vh      : string<'v', 'h'> {};
    struct d_vw      : string<'v', 'w'> {};
    struct d_px      : string<'p', 'x'> {};
    struct d_dp      : string<'d', 'p'> {};
    struct d_auto    : string<'a', 'u', 't', 'o'> {};
    struct d_percent : one<'%'> {};
    struct d_op      : sor< d_px, d_dp, d_vh, d_vw, d_percent > {};
    struct d_value   : seq< d_num, opt< d_ws, d_op > > {};
    struct d_expr    : sor< d_auto, d_value >{};
    struct d_grammar : seq< d_ws, d_expr, d_ws, eof > {};

    template<typename Rule>
    struct d_action : nothing< Rule >
    {
    };

    template<> struct d_action< d_num >
    {
        template< typename Input >
        static void apply(const Input& in, std::string& unit, bool& isAuto, double& value) {
            value = std::stod(in.string());
        }
    };

    template<> struct d_action< d_op >
    {
        template< typename Input >
        static void apply(const Input& in, std::string& unit, bool& isAuto, double& value) {
            unit = in.string();
        }
    };

    template<> struct d_action< d_auto >
    {
        template< typename Input >
        static void apply(const Input& in, std::string& unit, bool& isAuto, double& value) {
            isAuto = true;
        }
    };
    /**
     * \endcond
     */

    /**
     * Construct a dimension from a string.
     * @param context The defining context. Used to retrieve screen metrics.
     * @param value The dimension definition.
     * @param preferRelative If true, pure numeric values should be interpreted as relative values
     */
    Dimension::Dimension(const Context& context, const std::string& value, bool preferRelative)
        : mType(DimensionType::Absolute),
          mValue(0)
    {
        std::string unit;
        bool isAuto = false;
        pegtl::string_input<> in(value, "");

        // If you fail to parse it, return 0
        if (!pegtl::parse<d_grammar, d_action>(in, unit, isAuto, mValue)) {
            mValue = 0;
            return;
        }

        // We were set to auto
        if (isAuto) {
            mType = DimensionType::Auto;
        }
        else if (unit.compare("vh") == 0) {
            mValue = context.vhToDp(mValue);
        }
        else if (unit.compare("vw") == 0) {
            mValue = context.vwToDp(mValue);
        }
        else if (unit.compare("px") == 0) {
            mValue = context.pxToDp(mValue);
        }
        else if (unit.compare("%") == 0) {
            mType = DimensionType::Relative;
        }
        else if (unit.compare("") == 0) {
            if (preferRelative) {
                mValue *= 100;
                mType = DimensionType::Relative;
            }
        }

        // Anything else is absolute
    };

    /**
     * Write a dimension to the output stream.
     * @param os The output stream.
     * @param dimension The dimension to show.
     * @return The output stream, for chaining.
     */
    streamer& operator<<(streamer& os, const Dimension& dimension)
    {
        switch(dimension.mType) {
            case DimensionType::Auto:
                os << "auto";
                break;
            case DimensionType::Relative:
                os << dimension.mValue << "%";
                break;
            case DimensionType::Absolute:
                os << dimension.mValue << "dp";
                break;
        }
        return os;
    }


} // namespace apl