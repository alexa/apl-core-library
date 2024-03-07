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

#ifndef _APL_PSEUDO_LOCALIZATION_H
#define _APL_PSEUDO_LOCALIZATION_H

#include "apl/primitives/object.h"
#include "apl/primitives/texttransform.h"

namespace apl {

// Concrete implementation for pseudo-localization
class PseudoLocalizationTextTransformer : public TextTransformer {
public:
    /**
     * Transform the input string using pseudo-localization techniques.
     *
     * This method applies pseudo-localization transformations to the input string,
     * such as expanding by duplicating vowels, adding padding, and replacing
     * characters with accented equivalents. The specific transformations are
     * configured using the provided Object object.
     *
     * @param input The input string to be transformed.
     * @param config An Object representing configuration settings for pseudo-localization.
     * @return The pseudo-localized string.
     */
    std::string transform(const std::string& input, const Object& config) const override;

private:
    /**
     * Expand the string by duplicating vowels.
     * @param input The input string.
     * @param maxVowelsToBeAdded Maximum no. of vowels that can be added.
     * @return The expanded string.
     */
    static std::string expandByDuplicatingVowels(const std::string& input, int maxVowelsToBeAdded);
    /**
     * Add padding to the input string.
     * Padding is added at the start and end of the string.
     * @param input The input string.
     * @param paddingToBeAdded The desired padding.
     * @return The string with added padding.
     */
    static std::string addPaddingToMeetExpansion(const std::string& input, int paddingToBeAdded);

    /**
     * Function to expand the input string by duplicating vowels to meet the specified expansion
     * factor.
     *
     * @param input The input string to be expanded.
     * @param expansionPercentage The desired expansion percentage. In case its less than
     * 0 or greater than 100, default expansion percentage of 30 will be used.
     * @return The expanded string.
     */
    static std::string expandString(const std::string& input, double expansionPercentage);
    /**
     * Add start and end markers around the input string.This would help identify truncations in
     * case of missing markers.
     * @param input The input string to which markers will be added.
     * @return The string with added markers.
     */
    static std::string addStartAndEndMarkers(const std::string& input);
    /**
     * Replace characters with their Latin accented equivalents.
     * If a character is a vowel, it's retained to expand the length.
     * @param input The input string to be transformed.
     * @return The transformed string.
     */
    static std::string replaceCharsWithAccentedVersions(const std::string& input);
};
} // namespace apl

#endif // _APL_PSEUDO_LOCALIZATION_H
