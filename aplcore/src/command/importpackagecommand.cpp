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

#include "apl/command/importpackagecommand.h"

#include "apl/action/importpackageaction.h"
#include "apl/content/packageresolver.h"
#include "apl/content/importrequest.h"
#include "apl/utils/session.h"
#include "apl/document/coredocumentcontext.h"

namespace apl {

const CommandPropDefSet&
ImportPackageCommand::propDefSet() const {
    static CommandPropDefSet sImportPackageCommandProperties(CoreCommand::propDefSet(), {
        { kCommandPropertyAccept,   "",                    asString},
        { kCommandPropertyName,     "",                    asString, kPropRequired},
        { kCommandPropertyOnFail,   Object::EMPTY_ARRAY(), asArray},
        { kCommandPropertyOnLoad,   Object::EMPTY_ARRAY(), asArray},
        { kCommandPropertySource,   "",                    asString},
        { kCommandPropertyVersion,  "",                    asString, kPropRequired},
    });

    return sImportPackageCommandProperties;
}

ActionPtr
ImportPackageCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!calculateProperties())
        return nullptr;

    if (fastMode) {
        CONSOLE(mContext) << "Ignoring ImportPackage command in fast mode";
        return nullptr;
    }

    ActionRef actionRef{nullptr};
    auto action = Action::make(timers, [&actionRef](ActionRef ref) { actionRef = ref; });
    auto importPackageAction = ImportPackageAction::make(
        timers, std::static_pointer_cast<CoreCommand>(shared_from_this()), std::move(action));

    auto session = mContext->session();
    auto version = getValue(kCommandPropertyVersion).getString();
    auto accept = getValue(kCommandPropertyAccept).asString();
    auto acceptPattern =
        accept.empty() ? nullptr : SemanticPattern::create(session, accept);
    auto semanticVersion = SemanticVersion::create(session, version);

    ImportRequest request =
        ImportRequest(getValue(kCommandPropertyName).getString(), version,
                      getValue(kCommandPropertySource).getString(), std::set<std::string>(),
                      semanticVersion, acceptPattern);

    auto coreDocumentContext = CoreDocumentContext::cast(mContext->documentContext());
    if (coreDocumentContext->isPackageProcessed(request.reference().toString())) {
        importPackageAction->onLoad(request.reference().version());
        return nullptr;
    }

    auto evaluationContext = coreDocumentContext->contextPtr();
    auto packageManager = mContext->getRootConfig().getPackageManager();
    if (!packageManager) {
        importPackageAction->onFail(getValue(kCommandPropertyName).getString(), "ImportPackage command is unsupported by this runtime.", 400);
        return nullptr;
    }

    mPackageResolver = PackageResolver::create(packageManager, mContext->session());
    mPackageResolver->load(
        evaluationContext, session, request,
        [coreDocumentContext, importPackageAction, version](std::vector<PackagePtr>&& ordered) {
            coreDocumentContext->processPackagesIntoContext(ordered);
            importPackageAction->onLoad(version);
        },
        [importPackageAction](const ImportRef& ref, const std::string& errorMessage, int errorCode) {
            auto nameVersionSource = ref.toString() + ":" + ref.source();
            importPackageAction->onFail(nameVersionSource, errorMessage, errorCode);
    });

    return importPackageAction;
}

} // namespace apl