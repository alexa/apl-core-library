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

#ifndef APL_THROW_H
#define APL_THROW_H

namespace apl {

#if defined( _WIN32 )
__declspec(noreturn) void aplThrow(const char* message) noexcept;
#else
void aplThrow(const char* message) noexcept __attribute__((__noreturn__));
#endif

}

#endif //APL_THROW_H
