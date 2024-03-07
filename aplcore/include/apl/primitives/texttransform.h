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

#ifndef _APL_TEXT_TRANSFORM_H
#define _APL_TEXT_TRANSFORM_H

namespace apl {

// Class to handle various text transformations
class TextTransformer {
public:
    /**
     * Transform the input string based on the provided configuration.
     *
     * This pure virtual function needs to be implemented by concrete classes.
     *
     * @param input The input string to be transformed.
     * @param config An Object representing configuration settings for the transformation.
     * @return The transformed string.
     */
    virtual std::string transform(const std::string& input, const Object& config) const = 0;
    /**
     * Virtual destructor for proper polymorphic destruction.
     */
    virtual ~TextTransformer() = default;
};
} // namespace apl

#endif // _APL_TEXT_TRANSFORM_H
