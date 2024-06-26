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

#ifndef APL_TEST_PACKAGE_MANAGER_H
#define APL_TEST_PACKAGE_MANAGER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "apl/content/packagemanager.h"

namespace apl
{

class TestPackageManager : public PackageManager
{
public:
    void loadPackage(const PackageRequestPtr& packageRequest) override;

    void putPackage(const std::string& packageName, const std::string& packageData) {
        packages.emplace(packageName, packageData);
    }

    void succeed(const ImportRequest &request,
                 SharedJsonData&& jsonData);

    void fail(const ImportRequest &request);

    ImportRequest get(const std::string &packageName) const;

    const std::vector<PackageRequestPtr>& getUnresolvedRequests() const { return requests; }

    int getResolvedRequestCount() const { return resolvedRequests.size(); }

private:
    std::map<std::string, std::string> packages;
    std::vector<PackageRequestPtr> requests;
    std::vector<ImportRequest> resolvedRequests;
};

} // namespace apl

#endif // APL_TEST_PACKAGE_MANAGER_H
