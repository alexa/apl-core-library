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

#ifndef _APL_LOCALE_METHODS_H
#define _APL_LOCALE_METHODS_H

#include <string>

namespace apl {

/**
 * Defines the contract for locale-aware string manipulation. Override this class
 * to have platform specific methods and respect provided locale.
 */
class LocaleMethods {
public:
    virtual ~LocaleMethods() = default;

    /**
     * Transform the provided string value to upper case according to the specified locale.
     * Locales are specified in the same format used by the Alexa Skills Kit (see
     * https://developer.amazon.com/en-US/docs/alexa/custom-skills/request-and-response-json-reference.html#request-locale),
     * for example "en-US". An empty locale indicates that the default, device-specific locale
     * should be used.
     *
     * @param value The input string for case transform
     * @param locale The locale to use for the conversion, or the empty string if the default locale should be used instead
     * @return The transformed string to upper case
     */
    virtual std::string toUpperCase( const std::string& value, const std::string& locale ) = 0;

    /**
     * Transform the provided string value to lower case according to the specified locale.
     * Locales are specified in the same format used by the Alexa Skills Kit (see
     * https://developer.amazon.com/en-US/docs/alexa/custom-skills/request-and-response-json-reference.html#request-locale),
     * for example "en-US". An empty locale indicates that the default, device-specific locale
     * should be used.
     *
     * @param value The input string for case transform
     * @param locale The locale to use for the conversion, or the empty string if the default locale should be used instead
     * @return The transformed string to lower case
     */
    virtual std::string toLowerCase( const std::string& value, const std::string& locale ) = 0;

};

} // namespace apl

#endif //_APL_LOCALE_METHODS_H