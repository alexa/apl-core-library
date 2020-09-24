#!/usr/bin/env bash
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
#
#     http://aws.amazon.com/apache2.0/
#
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.

## This script validates include used by developer must be from valid header
## inclusion list.

include_dirs=("aplcore/include")

whitelisted_apl_headers=(
    "apl/action/action.h"
    "apl/apl.h"
    "apl/buildTimeConstants.h"
    "apl/command/commandproperties.h"
    "apl/common.h"
    "apl/component/component.h"
    "apl/component/componentproperties.h"
    "apl/component/textmeasurement.h"
    "apl/content/aplversion.h"
    "apl/content/content.h"
    "apl/content/extensioncommanddefinition.h"
    "apl/content/extensioneventhandler.h"
    "apl/content/extensionproperty.h"
    "apl/content/importref.h"
    "apl/content/importrequest.h"
    "apl/content/jsondata.h"
    "apl/content/metrics.h"
    "apl/content/package.h"
    "apl/content/rootconfig.h"
    "apl/content/settings.h"
    "apl/datasource/datasourceconnection.h"
    "apl/datasource/datasourceprovider.h"
    "apl/datasource/dynamicindexlistdatasourceprovider.h"
    "apl/datasource/offsetindexdatasourceconnection.h"
    "apl/dynamicdata.h"
    "apl/engine/binding.h"
    "apl/engine/dependant.h"
    "apl/engine/event.h"
    "apl/engine/info.h"
    "apl/engine/jsonresource.h"
    "apl/engine/parameterarray.h"
    "apl/engine/properties.h"
    "apl/engine/propertymap.h"
    "apl/engine/recalculatetarget.h"
    "apl/engine/rootcontext.h"
    "apl/engine/state.h"
    "apl/engine/styledefinition.h"
    "apl/engine/styleinstance.h"
    "apl/engine/styles.h"
    "apl/graphic/graphic.h"
    "apl/graphic/graphiccontent.h"
    "apl/graphic/graphicelement.h"
    "apl/graphic/graphicpattern.h"
    "apl/graphic/graphicproperties.h"
    "apl/livedata/livearray.h"
    "apl/livedata/livemap.h"
    "apl/livedata/liveobject.h"
    "apl/primitives/color.h"
    "apl/primitives/dimension.h"
    "apl/primitives/filter.h"
    "apl/primitives/gradient.h"
    "apl/primitives/keyboard.h"
    "apl/primitives/mediasource.h"
    "apl/primitives/mediastate.h"
    "apl/primitives/object.h"
    "apl/primitives/objectbag.h"
    "apl/primitives/objectdata.h"
    "apl/primitives/point.h"
    "apl/primitives/radii.h"
    "apl/primitives/rect.h"
    "apl/primitives/styledtext.h"
    "apl/primitives/transform2d.h"
    "apl/scaling/metricstransform.h"
    "apl/time/timers.h"
    "apl/touch/pointerevent.h"
    "apl/utils/bimap.h"
    "apl/utils/counter.h"
    "apl/utils/log.h"
    "apl/utils/path.h"
    "apl/utils/session.h"
    "apl/utils/streamer.h"
    "apl/utils/telemetry.h"
    "apl/utils/userdata.h"
    "apl/utils/visitor.h"
)

whitelisted_external_headers=(
    "rapidjson/document.h"
    "rapidjson/error/en.h"
    "rapidjson/stringbuffer.h"
    "rapidjson/writer.h"
)

blacklisted_headers=(
    "yoga/Yoga.h"
    "yoga/YGConfig.h"
    "yoga/YGEnums.h"
    "yoga/YGLayout.h"
    "yoga/YGNode.h"
    "yoga/YGNodePrint.h"
    "yoga/YGStyle.h"
    "yoga/YGValue.h"
)

worklist=(
    "apl/apl.h"
    "apl/dynamicdata.h"
)

function is_whitelisted_apl_header() {
    local canonical_header="$1"

    for h in "${whitelisted_apl_headers[@]}"; do
        if [ "${h}" == "${canonical_header}" ] ; then
            return 0
        fi
    done

    return 1
}

function is_whitelisted_external_header() {
    local canonical_header="$1"

    for h in "${whitelisted_external_headers[@]}"; do
        if [ "${h}" == "${canonical_header}" ] ; then
            return 0
        fi
    done

    return 1
}

function is_blacklisted_header() {
    local canonical_header="$1"

    for h in "${blacklisted_headers[@]}"; do
        if [ "${h}" == "${canonical_header}" ] ; then
            return 0
        fi
    done

    return 1
}

function is_whitelisted() {
    if is_whitelisted_apl_header "$1" ; then
        return 0
    fi

    if is_whitelisted_external_header "$1" ; then
        return 0
    fi

    return 1
}

function resolve_header_path() {
    local included_file="$1"
    local parent_file="$2"

    # #include "..." checks the current directory first, so mirror this logic if a parent file is provided
    if [[ ! -z "$2" ]]; then
        local parent_header_dir=$(dirname "$2")

        if [ -f "${parent_header_dir}/${included_file}" ]; then
            echo "${parent_header_dir}/${included_file}"
        fi
    fi

    # Look for the included file in the include path
    for d in "${include_dirs[@]}"; do
        if [ -f "${d}/${included_file}" ]; then
            echo "${d}/${included_file}"
            break
        fi
    done
}

function resolve_canonical_header() {
    resolved_path=$(resolve_header_path "$1" "$2")
    for d in "${include_dirs[@]}"; do
        if [[ "${resolved_path}" == ${d}* ]]; then
            echo "${resolved_path#$d/}"
            return
        fi
    done

    echo $1
}

function enqueue() {
    local canonical_header=$1

    for ((n=0; n<${#worklist[@]}; n++)); do
        if [[ "${worklist[n]}" == "${canonical_header}" ]]; then
            return
        fi
    done

    worklist+=( "$canonical_header" )
}

function validate_canonical_header() {
    local canonical_parent_header="$1"
    local resolved_header_path=$(resolve_header_path "${canonical_parent_header}")

    if is_whitelisted_external_header "${canonical_parent_header}" ; then
        # external header found, stop here without looking at its list of includes
        :
    elif is_whitelisted_apl_header "${canonical_parent_header}" ; then

        # check all included headers for whitelisting
        included_raw_headers=( `egrep '^#include[[:space:]]"[[:print:]]+"' "${resolved_header_path}" | sed 's/^[[:space:]]*#include[[:space:]]*"\([^"]*\)".*/\1/'` )

        for raw_header in "${included_raw_headers[@]}"; do
            canonical_header=$(resolve_canonical_header "${raw_header}" "${resolved_header_path}")
            if ! is_whitelisted "${canonical_header}" ; then
                echo "ERROR: file ${canonical_parent_header} includes invalid header ${raw_header}"
                exit 1
            fi

            enqueue "${canonical_header}"
        done

        # check all included headers for blacklisting
        included_lib_headers=( `egrep '^#include[[:space:]]<[[:print:]]+>' "${resolved_header_path}" | sed 's/^[[:space:]]*#include[[:space:]]*<\([^<]*\)>.*/\1/'` )
        for lib_header in "${included_lib_headers[@]}"; do
            canonical_header=$(resolve_canonical_header "${lib_header}" "${resolved_header_path}")
            if is_blacklisted_header "${lib_header}" ||  is_blacklisted_header "${canonical_header}"; then
                echo "ERROR: file ${canonical_parent_header} includes blacklisted header ${lib_header}"
                exit 1
            fi
        done

    fi
}

for ((i=0; i<${#worklist[@]}; i++)); do
    validate_canonical_header "${worklist[i]}"
done

echo "** No invalid headers included."
