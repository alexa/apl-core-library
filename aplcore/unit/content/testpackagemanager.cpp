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

#include "testpackagemanager.h"

#include <algorithm>

using namespace apl;

void
TestPackageManager::loadPackage(const PackageRequestPtr& packageRequest)
{
    requests.push_back(packageRequest);
    auto packageIt = packages.find(packageRequest->request().reference().toString());
    if (packageIt != packages.end()) {
        succeed(packageRequest->request(), SharedJsonData(packageIt->second));
    }
}

void
TestPackageManager::succeed(const ImportRequest& request, SharedJsonData&& jsonData)
{
    PackageRequestPtr packageRequest = nullptr;
    for (auto &it : requests) {
        if (it->request() == request) {
            packageRequest = it;
            break;
        }
    }

    if (packageRequest) {
        resolvedRequests.push_back(request);
        requests.erase(std::remove(requests.begin(), requests.end(), packageRequest), requests.end());
        packageRequest->succeed(std::move(jsonData));
    }
}

void
TestPackageManager::fail(const apl::ImportRequest& request)
{
    PackageRequestPtr packageRequest = nullptr;
    for (auto &it : requests) {
        if (it->request() == request) {
            packageRequest = it;
            break;
        }
    }

    if (packageRequest) {
        resolvedRequests.push_back(request);
        requests.erase(std::remove(requests.begin(), requests.end(), packageRequest), requests.end());
        packageRequest->fail("Package not found.", 404);
    }
}

ImportRequest
TestPackageManager::get(const std::string& packageName) const
{
    for (const auto & request : requests) {
        if (request->request().reference().toString() == packageName) {
            return request->request();
        }
    }

    return {};
}

