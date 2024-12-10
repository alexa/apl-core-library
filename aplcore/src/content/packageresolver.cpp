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

#include "apl/content/packageresolver.h"

#include "apl/utils/make_unique.h"

namespace apl {

void
PackageResolver::load(const PendingImportPackagePtr& pendingImportPackage,
                      SuccessCallback&& onSuccess,
                      FailureCallback&& onFailure,
                      PackageAddedCallback&& onPackageAdded)
{
    mPending = {pendingImportPackage, std::move(onSuccess), std::move(onFailure), std::move(onPackageAdded)};
    loadRequested(*pendingImportPackage);
}

void
PackageResolver::load(const ContextPtr& evaluationContext,
                      const apl::SessionPtr& session,
                      const ImportRequest& request,
                      SuccessCallback&& onSuccess,
                      FailureCallback&& onFailure)
{
    auto pending = std::make_shared<PendingImportPackage>(evaluationContext, session, request);
    mPending = {pending, std::move(onSuccess), std::move(onFailure), [](const Package& package){}};
    loadRequested(*pending);
}

void
PackageResolver::onPackageLoaded(const ImportRequest& request, JsonData&& jsonData)
{
    auto ptr = createPackage(request.reference(), mSession, std::move(jsonData));
    if (!ptr) {
        onPackageFailure(request, "Package unable to be parsed.", 400);
        return;
    }

    addPackage(request, ptr);
}

void
PackageResolver::addPackage(const ImportRequest& request, const PackagePtr& package)
{
    if (mPending.pendingImport == nullptr) return;
    if (!mPending.pendingImport->isPackagePending(request)) return;

    auto pendingImport = mPending.pendingImport;
    pendingImport->addPackage(request, package);
    mPending.onPackageAddedListener(*package);

    if (pendingImport->isReady()) {
        mPending.onSuccess(pendingImport->moveOrderedDependencies());
        mPending = {};
    } else if (pendingImport->isError()) {
        mPending.onFailure(pendingImport->getFailedRequestReference(), pendingImport->getError(), 400);
        mPending = {};
    } else {
        loadRequested(*pendingImport);
    }
}

void
PackageResolver::onPackageFailure(const ImportRequest& request, const std::string& errorMessage, int errorCode)
{
    if (mPending.pendingImport == nullptr) return;
    if (!mPending.pendingImport->isPackagePending(request)) return;

    mPending.onFailure(request.reference(), errorMessage, errorCode);
    mPending = {};
}

PackagePtr
PackageResolver::createPackage(const ImportRef& ref, const SessionPtr& session, JsonData&& jsonData)
{
    // If the package data is invalid, set the error state
    if (!jsonData) {
        CONSOLE(session).log("Package %s (%s) parse error offset=%u: %s",
                             ref.name().c_str(),
                             ref.version().c_str(),
                             jsonData.offset(), jsonData.error());
        return nullptr;
    }

    auto ptr = Package::create(session, ref.toString(), std::move(jsonData));
    if (!ptr) {
        LOG(LogLevel::kError).session(session)
            << "Package " << ref.name() << " (" << ref.version()
            << ") is invalid.";
        return nullptr;
    }

    return ptr;
}

void
PackageResolver::loadRequested(PendingImportPackage& pending)
{
    auto weakSelf = std::weak_ptr<PackageResolver>(shared_from_this());
    for (const auto& request : pending.getRequestedPackages()) {
        if (auto stashed = pending.getPreLoadedPackage(request.reference().toString())) {
            addPackage(request, stashed);
            continue ;
        }

        auto packageRequest = std::make_shared<PackageManager::PackageRequest>(
            request,
            [weakSelf](const ImportRequest& request, const SharedJsonData& jsonData) {
                if (auto self = weakSelf.lock())
                    self->onPackageLoaded(request, jsonData);
            },
            [weakSelf](const ImportRequest& request, const std::string& errorMessage, int code) {
                if (auto self = weakSelf.lock())
                    self->onPackageFailure(request, errorMessage, code);
            }
        );

        mPackageManager->loadPackage(packageRequest);
    }
}

} // namespace apl