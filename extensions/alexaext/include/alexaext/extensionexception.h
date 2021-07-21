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

#ifndef _ALEXAEXT_EXTENSIONEXCEPTION_H
#define _ALEXAEXT_EXTENSIONEXCEPTION_H

#include <cstdint>
#include <cstdarg>
#include <exception>
#include <string>
#include <vector>

namespace alexaext {

/**
 * Error Codes
 */
enum ExtensionError :  std::uint32_t {
    kErrorNone = 0,
    kErrorUnknownURI = 100,
    kErrorInvalidMessage = 200,
    kErrorException = 300,
    kErrorExtensionException = 400,
    kErrorFailedCommand = 500,
    kErrorInvalidExtensionSchema = 600,
};

static std::map<ExtensionError, std::string> sErrorMessage = {
        {kErrorUnknownURI,             "Unknown extension - uri: "},
        {kErrorInvalidMessage,         "Invalid or malformed message."},
        {kErrorException,              "Unknown Exception."},
        {kErrorExtensionException,     "Extension Exception - uri:%s msg:%s"},
        {kErrorFailedCommand,          "Failed Command - id: "},
        {kErrorInvalidExtensionSchema, "Invalid or malformed extension schema. uri: "}
};


/**
 * Exception class for extensions with simple message builder.
 */
class ExtensionException : public std::exception {
public:

    /**
     * Create an exception with formatted message.
     * @param format The base message string representing the format
     * @param ... arguments to be formatted
     * @return A string object holding the formatted result.
     */
    static ExtensionException create(const char *format, ...) {
        std::va_list argptr;
        va_start(argptr, format);

        std::va_list args2;
        va_copy(args2, argptr);

        std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, format, argptr));
        va_end(argptr);

        std::vsnprintf(buf.data(), buf.size(), format, args2);
        va_end(args2);

        ExtensionException exception;
        exception.msg = std::string(buf.begin(), buf.end());
        return exception;
    }

    virtual const char *what() const throw() {
        return msg.c_str();
    }

private:
    explicit ExtensionException() = default;

    std::string msg;
};

} // namespace alexaext

#endif //_ALEXAEXT_EXTENSIONEXCEPTION_H
