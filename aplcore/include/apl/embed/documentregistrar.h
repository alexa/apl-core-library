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

#ifndef APL_DOCUMENT_REGISTRY_H
#define APL_DOCUMENT_REGISTRY_H

#include <algorithm>
#include <map>
#include <string>

#include "apl/common.h"
#include "apl/utils/noncopyable.h"

namespace apl {

/**
 * Allows registration and de-registration of documents.
 */
class DocumentRegistrar final : public NonCopyable
{
public:
    ~DocumentRegistrar() = default;

    /**
     * Retrieve the document associated with id in the registry.
     *
     * @param id identifies the document to retrieve
     * @return the document if registered, and nullptr otherwise
     */
    CoreDocumentContextPtr get(int id) const;

    /**
     * @return All documents in the registrar.
     */
    const std::map<int, CoreDocumentContextPtr>& list() const;

    /**
     * Apply a function to every registered document.
     * @param func unary function object
     */
    template<typename Function>
    Function forEach(Function func) {
        std::for_each(
            mDocumentMap.begin(),
            mDocumentMap.end(),
            [func](const std::pair<int, const CoreDocumentContextPtr>& document) {
                return func(document.second);
            });
        return func;
    }

    /**
     * Add document to this registry.
     *
     * @param document the document to register
     * @return unique document ID
     */
    int registerDocument(const CoreDocumentContextPtr& document);

    /**
     * Removes the document identified by id from this registry.
     *
     * @param id identifies the document to deregister
     */
    void deregisterDocument(int id);

private:
    std::map<int, CoreDocumentContextPtr> mDocumentMap;

    int mIdGenerator = 1000;
};

} // namespace apl

#endif // APL_DOCUMENT_REGISTRY_H
