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

#ifndef _APL_CONTEXT_DATA_H
#define _APL_CONTEXT_DATA_H

#include "apl/common.h"
#include "apl/component/componentproperties.h"
#include "apl/content/metrics.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/runtimestate.h"
#include "apl/primitives/size.h"
#include "apl/utils/counter.h"

namespace apl {

class ContextData : public Counter<ContextData> {
public:
    explicit ContextData(const RootConfig& config,
                         RuntimeState&& runtimeState,
                         const SettingsPtr& settings,
                         const std::string& lang,
                         LayoutDirection layoutDirection)
        : mConfig(config),
          mRuntimeState(std::move(runtimeState)),
          mSettings(settings),
          mLang(lang),
          mLayoutDirection(layoutDirection)
    {}

    std::string getRequestedAPLVersion() const { return mRuntimeState.getRequestedAPLVersion(); }

    const RootConfig& rootConfig() const { return mConfig; }

    ContextData& lang(const std::string& lang) {
        mLang = lang;
        return *this;
    }

    ContextData& layoutDirection(LayoutDirection layoutDirection) {
        mLayoutDirection = layoutDirection;
        return *this;
    }

    std::string getLang() const { return mLang; }
    LayoutDirection getLayoutDirection() const { return mLayoutDirection; }
    bool getReinflationFlag() const { return mRuntimeState.getReinflation(); }
    virtual std::string getTheme() const { return mRuntimeState.getTheme(); }

    /**
     * @return true if represents full data binding context, false otherwise.
     */
    virtual bool fullContext() const { return false; }

    /**
     * @return Console log session owned by this context.
     */
    virtual const SessionPtr& session() const = 0;

    /**
     * @return true if context is in embedded document, false otherwise.
     */
    virtual bool embedded() const = 0;

    virtual ~ContextData() = default;

protected:
    const RootConfig mConfig;
    RuntimeState mRuntimeState;
    SettingsPtr mSettings;
    std::string mLang;
    LayoutDirection mLayoutDirection = LayoutDirection::kLayoutDirectionInherit;
};


} // namespace apl

#endif //_APL_CONTEXT_DATA_H
