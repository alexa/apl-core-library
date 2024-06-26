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

#ifndef APL_PACKAGEMANAGER_H
#define APL_PACKAGEMANAGER_H

#include <functional>
#include <memory>

#include "apl/content/importrequest.h"
#include "apl/content/sharedjsondata.h"

namespace apl {

/**
 * Package Manager responsible for responding to Import Requests.
 *
 * The viewhost should pass in an implementation of this class to resolve import requests for the
 * APL document. Furthermore, it is advisable to maintain a global package cache to reuse
 * SharedJsonData for multiple of the same ImportRequest.
 *
 * The accept property allows for an existing cached package to be used in place of the requested Import.
 * Therefore, when receiving a new request, check if it can match against other cached imports with
 * the same name using the {@code isAcceptableReplacementFor(thisRequest)} check to send an acceptable package.
 *
 * For example:
 *
 * virtual void loadPackage(const PackageRequestPtr& packageRequest) override {
 *   ImportRequest& request = packageRequest->request();
 *
 *   // try to find exact match of name/version first
 *   if (auto packageData = mCache.get(request) {
 *      packageRequest->succeed(packageData);
 *      return;
 *   }
 *
 *   // otherwise find a best match according to accept
 *   std::vector<std::pair<ImportRequest, SharedJsonData>> matchedNames = mCache.getPackagesWithSameName(request.reference().name());
 *   for (const auto& requestPackagePair : matchedNames) {
 *     if (requestPackagePair.first.isAcceptableReplacementFor(request)) {
 *       packageRequest->succeed(requestPackagePair.second);
 *       return;
 *     }
 *   }
 * }
 *
 */
class PackageManager {
public:
    virtual ~PackageManager() = default;
    /**
      * Callback invoked when package is loaded.
      * @param request The original import request.
      * @param packageData The package JSON data.
     */
    using SuccessCallback = std::function<void(const ImportRequest& request, const SharedJsonData& packageData)>;

    /**
      * Callback invoked when package cannot be loaded successfully.
      * @param request The original import request.
      * @param errorMessage The error message.
      * @param errorCode The error code.
     */
    using FailureCallback = std::function<void(const ImportRequest& request, const std::string& errorMessage, int errorCode)>;

    /**
      * A package request.
      *
      * @param request ImportRequest object containing package metadata.
      * @param onSuccess Callback to invoke when package loaded.
      * @param onFailure Callback to invoke when package failed to load.
     */
    class PackageRequest {
    public:
        PackageRequest(const ImportRequest& request, SuccessCallback&& onSuccess, FailureCallback&& onFailure):
            mRequest(request),
            mOnSuccess(onSuccess),
            mOnFailure(onFailure) {}

        /**
         * @return the import request.
         */
        const ImportRequest& request() { return mRequest; }

        /**
         * Succeed the request with the package SharedJsonData.
         * @param sharedJsonData
         */
        void succeed(const SharedJsonData& sharedJsonData) {
            mOnSuccess(mRequest, sharedJsonData);
            clear();
        }

        /**
         * Fail the request.
         */
        void fail(const std::string& errorMessage, int code) {
            mOnFailure(mRequest, errorMessage, code);
            clear();
        }

    private:
        void clear() {
            mOnFailure = [](const ImportRequest&, const std::string& errorMessage, int errorCode){};
            mOnSuccess = [](const ImportRequest&, const SharedJsonData&){};
        }

    private:
        const ImportRequest mRequest;
        SuccessCallback mOnSuccess;
        FailureCallback mOnFailure;
    };

    using PackageRequestPtr = std::shared_ptr<PackageRequest>;

    /**
      * Request a package be imported.
      * @param packageRequest The Package Request.
     */
    virtual void loadPackage(const PackageRequestPtr& packageRequest) = 0;
};

} // namespace apl


#endif // APL_PACKAGEMANAGER_H
