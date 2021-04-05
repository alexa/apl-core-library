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

#ifndef _APL_RUNTIME_STATE_H
#define _APL_RUNTIME_STATE_H

#include <string>

namespace apl {

/**
 * This structure holds runtime information for RootContextData. These are document-provided values
 * and other calculated values that are used in the initial inflation and execution.
 */
class RuntimeState {
public:
    RuntimeState(std::string theme, std::string requestedAPLVersion, bool reinflation)
        : mTheme(std::move(theme)),
          mRequestedAPLVersion(std::move(requestedAPLVersion)),
          mReinflation(reinflation)
    {}

    std::string getTheme() const { return mTheme; }
    std::string getRequestedAPLVersion() const { return mRequestedAPLVersion; }
    bool getReinflation() const { return mReinflation; }

private:
    std::string mTheme;
    std::string mRequestedAPLVersion;
    bool mReinflation;   // True if this is a re-inflation of an existing layout
};

} // namespace apl

#endif // _APL_RUNTIME_STATE_H
