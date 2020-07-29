#!/usr/bin/env bash
#
# Find the use of properties that have "kPropIn" set but are being
# used in a view host. Properties marked as kPropIn are provided by,
# and set by the document.  kPropIn properties are intended for
# APLCoreEngine use only.  These properties are used to derive kPropOut
# properties which can be safely consumed by the view host.
# 
# This scripts expects that you start in a view host git repository
# and the APLCoreEngine repository is next to it.

INPUT_ONLY_PROPS=$(cd ../APLCoreEngine && \
         git grep -E '\{.*kPropIn[^O]' | \
         cut -d ',' -f 1 | \
         grep -o -E '(k\w+)' | \
         sort | \
         uniq | \
         paste -d '|' -s - -)

git grep -E "${INPUT_ONLY_PROPS}"

