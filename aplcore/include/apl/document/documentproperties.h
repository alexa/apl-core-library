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

#ifndef _APL_DOCUMENT_PROPERTIES_H
#define _APL_DOCUMENT_PROPERTIES_H

#include <string>

#include "apl/utils/bimap.h"

namespace apl {

/**
 * List of properties that apply to the APL document
 *
 * TODO: Migrate other existing properties here
 */
enum DocumentPropertyKey {
    // Handler for configuration changes
    kDocumentPropertyOnConfigChange,
    // Handler for display state changes
    kDocumentPropertyOnDisplayStateChange
};

extern Bimap<int, std::string> sDocumentPropertyBimap;

}  // namespace apl

#endif //_APL_DOCUMENT_PROPERTIES_H
