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

#ifndef _APL_TRANSFORM_GRAMMAR_H
#define _APL_TRANSFORM_GRAMMAR_H

#include "apl/primitives/transform.h"

namespace apl {

/**
 * Simple transformation accumulator interface. Should be implemented by the user of the parse
 * method below to extract result of the processing.
 */
class TransformationAccumulator {
public:
    /**
     * Called each time when single transformation is successfully parsed.
     * @param transform Extracted transform.
     */
    virtual void add(const TransformPtr& transform) = 0;
};

namespace t2grammar {

/**
 * Parse an AVG style transformation definition and extract transformations into provided accumulator.
 * @param session Logging session.
 * @param transform String representing a transform definition.
 * @param out TransformationAccumulator for the output.
 * @return true if successful, false if string definition failed to parse.
 */
extern bool parse(const SessionPtr& session, const std::string& transform, TransformationAccumulator& out);

} // t2grammar

} // namespace apl

#endif //_APL_TRANSFORM_GRAMMAR_H
