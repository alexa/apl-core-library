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

#ifndef APL_HOST_COMPONENT_H
#define APL_HOST_COMPONENT_H

#include <string>

#include "apl/common.h"
#include "apl/component/actionablecomponent.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/componentproperties.h"
#include "apl/embed/documentmanager.h"
#include "apl/engine/properties.h"
#include "apl/utils/path.h"

namespace apl {

class HostComponent : public ActionableComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);

    HostComponent(const ContextPtr& context,
                  Properties&& properties,
                  const Path& path);

    ComponentType getType() const override { return kComponentTypeHost; };

    ComponentPtr findComponentById(const std::string& id, bool traverseHost) const override;

    bool singleChild() const override { return true; }

    void processLayoutChanges(bool useDirtyFlag, bool first) override;

    void postProcessLayoutChanges() override;

    /**
     * Reinflate contained document.
     */
    void reinflate();

protected:
    const ComponentPropDefSet& propDefSet() const override;

    void preRelease() override;

    void releaseSelf() override;

    void attachYogaNodeIfRequired(const CoreComponentPtr& coreChild, int index) override {}

    bool includeChildrenInVisualContext() const override;

    std::string getVisualContextType() const override;

    bool executeKeyHandlers(KeyHandlerType type, const Keyboard &keyboard) override;

private:
    DocumentContextPtr onLoad(const EmbeddedRequestSuccessResponse&& response);

    void onLoadHandler();

    void onFail(const EmbeddedRequestFailureResponse&& response);

    void onFailHandler(const URLRequest& url, const std::string& failure);

    DocumentContextPtr initializeEmbedded(const EmbeddedRequestSuccessResponse&& response);

    void detachEmbedded();

    void releaseEmbedded();

    void requestEmbedded();

    void resolvePendingParameters(const ContentPtr& content);

    /**
     * @return Owned document ID, 0 if none.
     */
    int getDocumentId() const;

    void setDocument(int id, bool connectedVC);

private:
    EmbedRequestPtr mRequest;
    bool mOnLoadOnFailReported = false;
    bool mNeedToRequestDocument = true;
};

} // namespace apl

#endif // APL_HOST_COMPONENT_H
