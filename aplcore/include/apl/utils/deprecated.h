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

#ifndef _APL_DEPRECATED_H
#define _APL_DEPRECATED_H

#if defined(__GNUC__) || defined(__clang__)
#define APL_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define APL_DEPRECATED __declspec(deprecated)
#else
#pragma message("WARNING: Missing compiler implementation for DEPRECATED")
#define APL_DEPRECATED
#endif

#endif //_APL_DEPRECATED_H