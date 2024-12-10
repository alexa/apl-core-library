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

#ifndef APL_PENDINGIMPORTPACKAGE_H
#define APL_PENDINGIMPORTPACKAGE_H

#include <functional>
#include <map>
#include <set>
#include <memory>
#include <vector>
#include <string>

#include "apl/common.h"
#include "apl/content/packageresolver.h"
#include "apl/utils/path.h"
#include "apl/utils/session.h"

namespace apl {

/**
 * Encapsulates state needed to resolve the dependencies of a Package (imports).
 */
class PendingImportPackage {
public:
    /**
     * Creates a PendingImportPackage
     * @param context   the evaluation context for imports (may be nullptr)
     * @param session   the session
     * @param root      the root package
     * @param preLoaded any pre-loaded packages
     */
    PendingImportPackage(const ContextPtr& context, const SessionPtr& session, const PackagePtr& root, const std::vector<PackagePtr>& preLoaded) :
          mContext(context),
          mSession(session),
          mRoot(root),
          mPreLoaded(preLoaded)
    {
        addImportList(*mRoot);
        updateStatus();
    }

    /**
     * Creates a PendingImportPackage
     * @param context   the evaluation context for imports (may be nullptr)
     * @param session   the session
     * @param root      the request
     */
    PendingImportPackage(const ContextPtr& context, const SessionPtr& session, const ImportRequest& request) :
          mContext(context),
          mSession(session)
    {
        mRequested.emplace(request);
    }

    /**
     * Adds a package.
     * @param request the import request
     * @param package the package to add
     */
    void addPackage(const ImportRequest& request, const PackagePtr& package);

    /**
     * @return if the package tree is satisfied and well ordered
     */
    bool isReady() const { return mState == State::READY; }

    /**
     * @return if the package tree is not well ordered or added packages are not properly defined.
     */
    bool isError() const { return mState == State::ERROR; }

    /*
     * @return the failing request if there is one.
     */
    const ImportRef& getFailedRequestReference() const { return mFailedRequestReference; }

    /**
     * @return the error string if state is error.
     */
    const std::string& getError() const { return mError; }

    /**
     * @param request the import request
     * @return checks if a package is pending for the import request.
     */
    bool isPackagePending(const ImportRequest& request) { return mPending.count(request) > 0; }

    /**
     * @return the set of requested packages and clears it.
     */
    std::set<ImportRequest> getRequestedPackages();

    /**
     * @return move the ordered dependencies out of this object.
     */
    std::vector<PackagePtr>&& moveOrderedDependencies() { return std::move(mOrderedDependencies); }

    /**
     * @return a reference to the root package.
     */
    PackagePtr getRoot() const { return mRoot; }

    /**
     * Return a pre-loaded package by name.
     * @param packageName the stashed package name
     * @return a pre-loaded package if it exists, or nullptr.
     */
    PackagePtr getPreLoadedPackage(const std::string& packageName) const;

private:
    void addImportList(Package& package);
    bool addImport(Package& package,
                   const rapidjson::Value& value,
                   const std::string& name = "",
                   const std::string& version = "",
                   const std::string& domain = "",
                   const std::set<std::string>& loadAfter = {},
                   const std::string& accept = "");
    bool addToDependencyList(std::vector<PackagePtr>& ordered, std::set<PackagePtr>& inProgress, const PackagePtr& package);
    void updateStatus();
    void setError(const std::string& error);
    bool orderDependencyList();
    ImportRequest createOrGetSuitableRequest(const ImportRequest& request);

private:
    enum class State {
        LOADING,
        READY,
        ERROR
    };

    ContextPtr mContext;
    SessionPtr mSession;
    PackagePtr mRoot;
    std::set<ImportRequest> mRequested;
    std::set<ImportRequest> mPending;
    std::map<ImportRef, PackagePtr> mLoaded;
    std::map<std::string, std::vector<ImportRef>> mDependencies;
    // Map of import name to created imports with that name. When creating an import, we look up
    // this map first and return an existing one if it satisfies the import request.
    std::map<std::string, std::vector<ImportRequest>> mNameImportRequestMap;
    std::vector<PackagePtr> mOrderedDependencies;
    State mState = State::LOADING;
    std::string mError = "";
    ImportRef mFailedRequestReference;
    // list of preloaded packages to reuse
    std::vector<PackagePtr> mPreLoaded;
};

}

#endif // APL_PENDINGIMPORTPACKAGE_H
