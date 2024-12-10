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

#ifndef APL_PACKAGERESOLVER_H
#define APL_PACKAGERESOLVER_H

#include <functional>
#include <map>
#include <memory>
#include <set>

#include "apl/common.h"
#include "apl/content/package.h"
#include "apl/content/packagemanager.h"
#include "apl/content/pendingimportpackage.h"

namespace apl {

/**
 * Manages resolving the requested imports for a package.
 */
class PackageResolver : public std::enable_shared_from_this<PackageResolver> {
public:
    /// Callback with the ordered list of packages.
    using SuccessCallback = std::function<void(std::vector<PackagePtr>&& ordered)>;
    /// Callback for if one or more packages were unable to be downloaded or they could not be ordered.
    using FailureCallback = std::function<void(const ImportRef& ref, const std::string& errorMessage, int errorCode)>;
    /// Callback for when packages are added.
    using PackageAddedCallback = std::function<void(const Package& package)>;

    /**
     * Creates a PackageResolver for resolving all the imports from a root Package.
     * @param packageManager the package manager for retrieving requested imports.
     * @param session  the session for error logging.
     * @return a pointer to a PackageResolver
     */
    static PackageResolverPtr create(const PackageManagerPtr& packageManager,
                                     const SessionPtr& session)
    {
        return std::make_shared<PackageResolver>(packageManager, session);
    }

    /**
     * Do not call this directly. Instead use create(const PackageManagerPtr& packageManager, const SessionPtr& session).
     * @param packageManager the package manager for retrieving requested imports.
     * @param session  the session for error logging.
     */
    PackageResolver(const PackageManagerPtr& packageManager, const SessionPtr& session) : mPackageManager(packageManager), mSession(session) {}

    /**
     * Loads the packages that are requested from a pending import package.
     *
     * @param pendingImportPackage the requested packages to import
     * @param onSuccess            the success callback for when all imports have been successfully
     *                                 resolved and ordered
     * @param onFailure            the failure callback for if one or more imports have not resolved
     *                                 or they cannot be ordered
     * @param onPackageAdded       a callback for handling package added updates.
     */
    void load(const PendingImportPackagePtr& pendingImportPackage,
                                 SuccessCallback&& onSuccess,
                                 FailureCallback&& onFailure,
                                 PackageAddedCallback&& onPackageAdded = [](const Package& package){});

    /**
     * Loads the packages that are requested from the import request.
     *
     * @param evaluationContext the context for evaluating imports
     * @param session           the session
     * @param request           the import request
     * @param onSuccess         the success callback for when all imports have been successfully
     *                              resolved and ordered
     * @param onFailure         the failure callback for if one or more imports have not resolved
     *                              or they cannot be ordered
     */
    void load(const ContextPtr& evaluationContext,
              const SessionPtr& session,
              const ImportRequest& request,
              SuccessCallback&& onSuccess,
              FailureCallback&& onFailure);

private:
    friend class Content;

    struct PendingLoad {
        PendingImportPackagePtr pendingImport;
        SuccessCallback onSuccess;
        FailureCallback onFailure;
        PackageAddedCallback onPackageAddedListener;
    };

    void setPackageManager(const PackageManagerPtr& packageManager) { mPackageManager = packageManager; }
    void onPackageFailure(const ImportRequest& request, const std::string& errorMessage, int errorCode);
    void onPackageLoaded(const ImportRequest& request, const SharedJsonData& jsonData) {
        onPackageLoaded(request, JsonData(jsonData));
    }
    PackagePtr createPackage(const ImportRef& ref, const SessionPtr& session, SharedJsonData&& jsonData) {
        return createPackage(ref, session, JsonData(jsonData));
    }
    void addPackage(const ImportRequest& request, const PackagePtr& package);
    /// TODO delete these when Content no longer uses addPackage(ImportRequest&, JsonData&&)
    void onPackageLoaded(const ImportRequest& request, JsonData&& jsonData);
    PackagePtr createPackage(const ImportRef& ref, const SessionPtr& session, JsonData&& jsonData);
    void loadRequested(PendingImportPackage& pending);

private:
    PackageManagerPtr mPackageManager;
    SessionPtr mSession;
    PendingLoad mPending;
};

} // namespace apl

#endif // APL_PACKAGERESOLVER_H
