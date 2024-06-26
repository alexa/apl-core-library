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

#include "apl/content/pendingimportpackage.h"

#include <queue>

#include "apl/engine/evaluate.h"

namespace apl {

const bool DEBUG_IMPORT_PACKAGE = false;
const char* DOCUMENT_IMPORT = "import";
const char* PACKAGE_TYPE = "type";
const char* PACKAGE_TYPE_PACKAGE = "package";
const char* PACKAGE_TYPE_ONEOF = "oneOf";
const char* PACKAGE_TYPE_ALLOF = "allOf";
const char* PACKAGE_OTHERWISE = "otherwise";
const char* PACKAGE_ITEMS = "items";
const char* PACKAGE_WHEN = "when";

std::set<ImportRequest>
PendingImportPackage::getRequestedPackages()
{
    mPending.insert(mRequested.begin(), mRequested.end());
    auto result = mRequested;
    mRequested.clear();
    return result;
}

void
PendingImportPackage::addPackage(const ImportRequest& request, const PackagePtr& package)
{
    if (mRoot == nullptr) {
        mRoot = package;
    }
    // Erase from the pending set
    mPending.erase(request);
    mLoaded.emplace(request.reference(), package);
    addImportList(*package);
    if (isError()) {
        mFailedRequestReference = request.reference();
    }
    updateStatus();
}

void
PendingImportPackage::addImportList(Package& package)
{
    const rapidjson::Value& value = package.json();

    auto it = value.FindMember(DOCUMENT_IMPORT);
    if (it != value.MemberEnd()) {
        if (!it->value.IsArray()) {
            setError("Document import property should be an array");
            return;
        }
        for (const auto& v : it->value.GetArray())
            addImport(package, v);
    }
}

bool
PendingImportPackage::addImport(Package& package,
                                const rapidjson::Value& value,
                                const std::string& name,
                                const std::string& version,
                                const std::set<std::string>& loadAfter,
                                const std::string& accept)
{
    LOG_IF(DEBUG_IMPORT_PACKAGE).session(mSession) << "addImport " << &package;

    if (mState == State::ERROR) return false;

    if (!value.IsObject()) {
        setError("Invalid import record in document");
        return false;
    }

    // Check for conditionality, only if context available.
    if (mContext) {
        auto it_when = value.FindMember(PACKAGE_WHEN);
        if (it_when != value.MemberEnd()) {
            auto evaluatedWhen = evaluate(*mContext, it_when->value.GetString());
            if (!evaluatedWhen.asBoolean()) return false;
        }
    }

    auto typeIt = value.FindMember(PACKAGE_TYPE);
    std::string type = PACKAGE_TYPE_PACKAGE;
    if (typeIt != value.MemberEnd() && typeIt->value.IsString()) {
        type = typeIt->value.GetString();
    }

    if (type == PACKAGE_TYPE_ONEOF) {
        auto sIt = value.FindMember(PACKAGE_ITEMS);
        if (sIt != value.MemberEnd() && sIt->value.IsArray()) {
            // Expansion. Can use common name/version/loadAfter/accept.
            auto commonNameAndVersion = ImportRequest::extractNameAndVersion(value, mContext);
            auto commonLoadAfter = ImportRequest::extractLoadAfter(value, mContext);
            auto commonAccept = ImportRequest::extractAccept(value, mContext);
            for (const auto& s : sIt->value.GetArray()) {
                if (addImport(package, s,
                              commonNameAndVersion.first.empty() ? name : commonNameAndVersion.first,
                              commonNameAndVersion.second.empty() ? version : commonNameAndVersion.second,
                              commonLoadAfter.empty() ? loadAfter : commonLoadAfter,
                              commonAccept.empty() ? accept : commonAccept))
                    return true;
            }
        } else {
            setError("Missing items field for the oneOf import");
            return false;
        }

        // If no imports were matched - use otherwise
        auto otherwiseIt = value.FindMember(PACKAGE_OTHERWISE);
        if (otherwiseIt != value.MemberEnd() && otherwiseIt->value.IsArray()) {
            // Expansion. Can use common name/version/loadAfter/accept.
            auto commonNameAndVersion = ImportRequest::extractNameAndVersion(value, mContext);
            auto commonLoadAfter = ImportRequest::extractLoadAfter(value, mContext);
            auto commonAccept = ImportRequest::extractAccept(value, mContext);
            for (const auto& s : otherwiseIt->value.GetArray()) {
                if (!addImport(package, s, commonNameAndVersion.first, commonNameAndVersion.second, commonLoadAfter, commonAccept)) {
                    setError("Otherwise imports failed");
                    return false;
                }
            }
        }

        // Nothing was done, which is kinda fine
        return true;
    } else if (type == PACKAGE_TYPE_ALLOF) {
        auto sIt = value.FindMember(PACKAGE_ITEMS);
        if (sIt != value.MemberEnd() && sIt->value.IsArray()) {
            auto commonNameAndVersion = ImportRequest::extractNameAndVersion(value, mContext);
            auto commonLoadAfter = ImportRequest::extractLoadAfter(value, mContext);
            auto commonAccept = ImportRequest::extractAccept(value, mContext);
            for (const auto& s : sIt->value.GetArray()) {
                addImport(package, s,
                          commonNameAndVersion.first.empty() ? name : commonNameAndVersion.first,
                          commonNameAndVersion.second.empty() ? version : commonNameAndVersion.second,
                          commonLoadAfter.empty() ? loadAfter : commonLoadAfter,
                          commonAccept.empty() ? accept : commonAccept);
            }
        } else {
            setError("Missing items field for the allOf import");
            return false;
        }

        return true;
    }

    ImportRequest request = ImportRequest::create(value, mContext, mSession, name, version, loadAfter, accept);
    if (!request.isValid()) {
        setError("Malformed package import record");
        return false;
    }

    // Create a suitable request.
    request = createOrGetSuitableRequest(request);
    auto dependenciesIt = mDependencies.find(package.name());
    if (dependenciesIt == mDependencies.end()) {
        dependenciesIt = mDependencies.emplace(package.name(), std::vector<ImportRef>()).first;
    }
    dependenciesIt->second.push_back(request.reference());

    if (mRequested.find(request) == mRequested.end() &&
        mPending.find(request) == mPending.end() &&
        mLoaded.find(request.reference()) == mLoaded.end()) {

        mRequested.insert(std::move(request));
    }

    return true;
}

bool
PendingImportPackage::addToDependencyList(std::vector<PackagePtr>& ordered,
                                          std::set<PackagePtr>& inProgress,
                                          const PackagePtr& package)
{
    std::vector<ImportRef> pds;
    auto dependenciesIt = mDependencies.find(package->name());
    if (dependenciesIt != mDependencies.end()) {
        pds = dependenciesIt->second;
    }

    LOG_IF(DEBUG_IMPORT_PACKAGE).session(mSession) << "addToDependencyList " << package
                                            << " dependency count=" << pds.size();
    inProgress.insert(package);  // For dependency loop detection

    auto depQueue = std::queue<ImportRef>();
    for (const auto& pd : pds) depQueue.emplace(pd);

    std::set<std::string> available;
    std::set<std::pair<std::string, std::string>> delayed;
    size_t circularCounter = 0;
    while (!depQueue.empty()) {
        auto ref = depQueue.front();
        depQueue.pop();

        auto needDeps = false;

        // Check to see if package has load dependency and it was already included
        for (const auto& dep : ref.loadAfter()) {
            if (!ref.loadAfter().empty() && !available.count(dep)) {
                // Check if we have anything else to load
                if (depQueue.empty()) {
                    CONSOLE(mSession).log("Required loadAfter package not available %s for %s",
                                          dep.c_str(), ref.name().c_str());
                    mFailedRequestReference = ref;
                    return false;
                }

                // Check if we have reverse dep
                if (delayed.count(std::make_pair(dep, ref.name()))) {
                    CONSOLE(mSession).log("Circular package loadAfter dependency between %s and %s",
                                          ref.name().c_str(), dep.c_str());
                    mFailedRequestReference = ref;
                    return false;
                }

                delayed.emplace(ref.name(), dep);
                depQueue.emplace(ref);
                needDeps = true;
            }
        }

        if (circularCounter > depQueue.size()) {
            CONSOLE(mSession).log("Circular package loadAfter dependency chain");
            mFailedRequestReference = ref;
            return false;
        }

        circularCounter++;

        if (needDeps) continue;

        // Reset counter
        circularCounter = 0;

        LOG_IF(DEBUG_IMPORT_PACKAGE).session(mSession) << "checking child " << ref.toString();

        // Convert the reference into a loaded PackagePtr
        const auto& pkg = mLoaded.find(ref);
        if (pkg == mLoaded.end()) {
            assert(false);
            LOG(LogLevel::kError).session(mSession) << "Missing package '" << ref.name()
                                                    << "' in the loaded set";
            return false;
        }

        const PackagePtr& child = pkg->second;

        // Check if it is already in the dependency list (someone else included it first)
        auto it = std::find(ordered.begin(), ordered.end(), child);
        if (it != ordered.end()) {
            LOG_IF(DEBUG_IMPORT_PACKAGE).session(mSession) << "child package " << ref.toString()
                                                    << " already in dependency list";
            continue;
        }

        // Check for a circular dependency
        if (inProgress.count(child)) {
            CONSOLE(mSession).log("Circular package dependency '%s'", ref.name().c_str());
            mFailedRequestReference = ref;
            return false;
        }

        if (!addToDependencyList(ordered, inProgress, child)) {
            LOG_IF(DEBUG_IMPORT_PACKAGE).session(mSession) << "returning false with child package " << child->name();
            return false;
        }
        available.emplace(ref.name());
    }

    LOG_IF(DEBUG_IMPORT_PACKAGE).session(mSession) << "Pushing package " << package
                                            << " onto ordered list";
    ordered.push_back(package);
    inProgress.erase(package);
    return true;
}

void
PendingImportPackage::updateStatus()
{
    if (mState == State::LOADING && mRequested.empty() && mPending.empty()) {
        // Content is ready if the dependency list is successfully ordered, otherwise there is an error.
        if (orderDependencyList()) {
            mState = State::READY;
        } else {
            setError("Failure to order packages");
        }
    }
}

bool 
PendingImportPackage::orderDependencyList()
{
    std::set<PackagePtr> inProgress;
    bool isOrdered = addToDependencyList(mOrderedDependencies, inProgress, mRoot);
    return isOrdered;
}

void PendingImportPackage::setError(const std::string& error)
{
    CONSOLE(mSession) << error;
    mError = error;
    mState = State::ERROR;
    mPending.clear();
    mRequested.clear();
    mLoaded.clear();
}

ImportRequest
PendingImportPackage::createOrGetSuitableRequest(const ImportRequest& request)
{
    auto existingRequests = mNameImportRequestMap.find(request.reference().name());
    if (existingRequests == mNameImportRequestMap.end()) {
        existingRequests = mNameImportRequestMap.emplace(request.reference().name(), std::vector<ImportRequest>()).first;
    }
    // Look for a request that has the same name and whose version matches the accept pattern.
    for (const auto& requestsWithSameName : existingRequests->second) {
        if (requestsWithSameName.isAcceptableReplacementFor(request)) {
            return requestsWithSameName;
        }
    }

    // No existing requests satisfy it, so stash the request and return it
    existingRequests->second.push_back(request);
    return request;
}

PackagePtr
PendingImportPackage::getPreLoadedPackage(const std::string& packageName) const
{
    for (const auto& package : mPreLoaded) {
        if (packageName == package->name()) {
            return package;
        }
    }

    return nullptr;
}

} // namespace apl