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

#include "apl/content/directive.h"
#include "apl/engine/builder.h"
#include "apl/content/content.h"
#include "apl/utils/session.h"
#include "apl/content/rootconfig.h"

namespace apl {

std::shared_ptr<Directive>
Directive::create(JsonData&& directive)
{
    return create(std::move(directive), makeDefaultSession());
}

std::shared_ptr<Directive>
Directive::create(JsonData&& directive, const SessionPtr& session)
{
    if (!directive) {
        CONSOLE_S(session).log("Directive parse error offset=%u: %s", directive.offset(), directive.error());
        return nullptr;
    }

    auto ptr = std::make_shared<Directive>(session, std::move(directive));
    if (!ptr->mContent)
        return nullptr;

    return ptr;
}

Directive::Directive(const SessionPtr& session, JsonData &&directive)
    : mSession(session),
      mDirective(std::move(directive))
{
    // Sometimes we get a directive that includes a header and payload.
    // Sometimes we just get the contents of the payload.
    // Check for the document first; if we don't find it, try looking for a document under the payload
    const rapidjson::Value *payload = &mDirective.get();
    auto payloadIter = payload->FindMember("payload");
    if (payloadIter != payload->MemberEnd())
        payload = &payloadIter->value;

    const auto& doc = payload->FindMember("document");
        if (doc == payload->MemberEnd()) {
            CONSOLE_S(session).log("Directive payload does not contain a document");
            return;
        }

    // Find the main template and count the parameters
    auto mainTemplate = doc->value.FindMember("mainTemplate");
    if (mainTemplate == doc->value.MemberEnd()) {
        CONSOLE_S(session).log("Directive document does not contain a mainTemplate");
        return;
    }

    int paramCount = 0;
    auto parameters = mainTemplate->value.FindMember("parameters");
    if (parameters != mainTemplate->value.MemberEnd()) {
        if (!parameters->value.IsArray()) {
            CONSOLE_S(session).log("Main template parameters is not an array");
            return;
        }

        paramCount = parameters->value.Size();
        if (paramCount > 1) {
            CONSOLE_S(session).log("Main template can have at most one parameter");
            return;
        }
    }

    // Now check that we have a single data source if we have a single parameters
    auto datasources = payload->FindMember("datasources");
    auto hasDataSource = datasources != payload->MemberEnd();

    if (paramCount == 1 && !hasDataSource) {
        CONSOLE_S(session).log("Document missing datasources");
        return;
    }

    mContent = Content::create(doc->value, session);
    if (paramCount == 1)
        mContent->addData(mContent->getParameterAt(0), datasources->value);
}

RootContextPtr
Directive::build(const Metrics& metrics) const
{
    return build(metrics, RootConfig());
}

RootContextPtr
Directive::build(const Metrics& metrics, const RootConfig& config) const
{
    return RootContext::create(metrics, mContent, config);
}

}  // namespace apl