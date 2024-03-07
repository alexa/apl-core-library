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

#include "apl/primitives/pseudolocalizer.h"
#include "apl/utils/stringfunctions.h"

namespace apl {

const double DEFAULT_EXPANSION_PERCENTAGE = 30;

// Map of characters to their French Accented versions
static const std::map<char, std::string> latinEquivalentMap = {
    {'A', "Ȧ"}, {'B', "Ɓ"}, {'C', "Ƈ"}, {'D', "Ḓ"}, {'E', "Ḗ"}, {'F', "Ƒ"}, {'G', "Ɠ"}, {'H', "Ħ"},
    {'I', "Ī"}, {'J', "Ĵ"}, {'K', "Ķ"}, {'L', "Ŀ"}, {'M', "Ḿ"}, {'N', "Ƞ"}, {'O', "Ǿ"}, {'P', "Ƥ"},
    {'Q', "Ɋ"}, {'R', "Ř"}, {'S', "Ş"}, {'T', "Ŧ"}, {'U', "Ŭ"}, {'V', "Ṽ"}, {'W', "Ẇ"}, {'X', "Ẋ"},
    {'Y', "Ẏ"}, {'Z', "Ẑ"}, {'a', "ȧ"}, {'b', "ƀ"}, {'c', "ƈ"}, {'d', "ḓ"}, {'e', "ḗ"}, {'f', "ƒ"},
    {'g', "ɠ"}, {'h', "ħ"}, {'i', "ī"}, {'j', "ĵ"}, {'k', "ķ"}, {'l', "ŀ"}, {'m', "ḿ"}, {'n', "ƞ"},
    {'o', "ǿ"}, {'p', "ƥ"}, {'q', "ɋ"}, {'r', "ř"}, {'s', "ş"}, {'t', "ŧ"}, {'u', "ŭ"}, {'v', "ṽ"},
    {'w', "ẇ"}, {'x', "ẋ"}, {'y', "ẏ"}, {'z', "ẑ"}};

std::string
PseudoLocalizationTextTransformer::expandByDuplicatingVowels(const std::string& input,
                                                             int maxVowelsToBeAdded)
{
    if (maxVowelsToBeAdded <= 0) {
        return input;
    }
    else {
        auto duplicatedVowels = std::string("");
        for (char c : input) {
            char lower_c = sutil::tolower(c);
            if ((lower_c == 'a' || lower_c == 'e' || lower_c == 'i' || lower_c == 'o' ||
                 lower_c == 'u') &&
                maxVowelsToBeAdded > 0) {
                duplicatedVowels += c;
                maxVowelsToBeAdded--;
            }
            duplicatedVowels += c;
        }
        return duplicatedVowels;
    }
}

std::string
PseudoLocalizationTextTransformer::addPaddingToMeetExpansion(const std::string& input,
                                                             int paddingToBeAdded)
{
    if (paddingToBeAdded <= 0) {
        return input;
    }
    auto desiredTotalLength = input.length() + paddingToBeAdded;

    auto paddedText = std::string("");
    paddedText.reserve(desiredTotalLength);

    // Add padding at the start
    for (auto i = 0; i < paddingToBeAdded / 2; ++i) {
        paddedText += '-';
    }

    paddedText += input;

    // Add padding at the end
    for (auto i = 0; i < paddingToBeAdded - paddingToBeAdded / 2; ++i) {
        paddedText += '-';
    }

    return paddedText;
}

std::string
PseudoLocalizationTextTransformer::expandString(const std::string& input,
                                                double expansionPercentage)
{
    if (input.empty()) {
        return input;
    }
    double expansionFactor;
    if (expansionPercentage >= 0 && expansionPercentage <= 100) {
        expansionFactor = 1 + expansionPercentage / 100.0;
    }
    else {
        expansionFactor = 1 + DEFAULT_EXPANSION_PERCENTAGE / 100.0;
    }

    // Calculate total characters to be added
    auto expandedLength = static_cast<size_t>(input.length() * expansionFactor);
    auto charactersToAdd = expandedLength - input.length();

    // Add characters by duplicating vowels
    auto duplicatedVowels = expandByDuplicatingVowels(input, charactersToAdd);

    // In case there is still scope for expansion, add padding
    auto paddingToBeAdded = expandedLength - duplicatedVowels.length();
    auto expanded = addPaddingToMeetExpansion(duplicatedVowels, paddingToBeAdded);

    return expanded;
}

std::string
PseudoLocalizationTextTransformer::addStartAndEndMarkers(const std::string& input)
{
    return "[" + input + "]";
}

std::string
PseudoLocalizationTextTransformer::replaceCharsWithAccentedVersions(const std::string& input)
{
    auto diacriticized = std::string("");
    for (char c : input) {
        auto it = latinEquivalentMap.find(c);
        if (it != latinEquivalentMap.end())
            diacriticized += it->second;
        else
            diacriticized += c;
    }
    return diacriticized;
}

std::string
PseudoLocalizationTextTransformer::transform(const std::string& input, const Object& config) const
{
    if (config.isMap() && config.get("enabled").asBoolean()) {
        // Horizontally expand the string.
        auto expanded = expandString(input, config.get("expansionPercentage").asNumber());
        // Adding start and end markers.
        auto bracketed = addStartAndEndMarkers(expanded);

        // Replace characters with accented latin versions
        auto replaced = replaceCharsWithAccentedVersions(bracketed);
        return replaced;
    }
    return input;
}

} // namespace apl
