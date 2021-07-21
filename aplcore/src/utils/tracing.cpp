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

#include "apl/utils/tracing.h"
#include "apl/utils/log.h"

#ifdef ANDROID
#include <dlfcn.h>
static void *(*ATrace_beginSection)(const char *sectionName);
static void *(*ATrace_endSection)();
typedef void *(*fp_ATrace_beginSection)(const char *sectionName);
typedef void *(*fp_ATrace_endSection)();
#endif

namespace apl {

bool Tracing::mSupported = false;
bool Tracing::mInitialized = false;

void
Tracing::beginSection(const char *sectionName)
{
    if (!mInitialized) initialize();
    if (mSupported) {
#ifdef ANDROID
        ATrace_beginSection(sectionName);
#endif
    }
}

void
Tracing::endSection(const char *sectionName)
{
    if (mSupported) {
#ifdef ANDROID
        ATrace_endSection();
#endif
    }
}

void
Tracing::initialize()
{
    if (mInitialized) return;
#ifdef ANDROID
    // Using dlsym to allow tracing on 21+ as android/trace.h appears only in 23+
    void *lib = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
    if (lib == nullptr) {
        LOG(LogLevel::kError) << "Could not open libandroid.so";
    } else {
        ATrace_beginSection = reinterpret_cast<fp_ATrace_beginSection>(dlsym(lib, "ATrace_beginSection"));
        ATrace_endSection = reinterpret_cast<fp_ATrace_endSection>(dlsym(lib, "ATrace_endSection"));
        if (ATrace_beginSection != nullptr && ATrace_endSection != nullptr){
            mSupported = true;
        }
    }
#endif
    if (!mSupported) {
        LOG(LogLevel::kError) << "Platform tracing is not supported.";
    }
    mInitialized = true;
}

} // namespace apl
