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

#include "apl/document/documentproperties.h"

namespace apl {

/**
 * Map between property names and property constants.
 *
 * The naming convention is camel-case, starting with a lower-case
 * letter.  The naming convention is:
 *
 * 1. User-settable properties start with a lower-case letter (e.g., "onConfigChange")
 * 2. System-calculated properties start with an underscore (e.g.  "_someCalculatedProp").
 *    A system-calculated property may not be set by the user, but may (and should) be read.
 * 3. Experimental properties start with a hyphen (e.g., "-someExperimentalProp").
 *    Experimental properties are not part of the public documentation and may
 *    change at any time.
 */
Bimap<int, std::string> sDocumentPropertyBimap = {
    {kDocumentPropertyOnConfigChange,       "onConfigChange"},
    {kDocumentPropertyOnDisplayStateChange, "onDisplayStateChange"}
};

}  // namespace apl
