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

#ifndef _APL_SCOPED_COLLECTION
#define _APL_SCOPED_COLLECTION

#include "apl/utils/noncopyable.h"

namespace apl {

/**
 * Simple implementation of collection that can be grouped by a particular key (scope)
 * @tparam Scope Type of the scope
 * @tparam Type Type of the value
 * @tparam Collection Enclosed collection type
 * @tparam ReturnType Collection returns
 */
template<
    class Scope,
    class Type,
    class Collection,
    class ReturnType = std::vector<Type>>
class ScopedCollection : public NonCopyable {
public:
    virtual ~ScopedCollection() = default;

    /**
     * @return true if collection is empty, false otherwise.
     */
    virtual bool empty() const = 0;

    /**
     * @return Number of elements in the collection.
     */
    virtual size_t size() const = 0;

    /**
     * @return Reference to enclosed collection
     */
    virtual const Collection& getAll() const = 0;

    /**
     * Get set of values related to a requested scope
     * @param scope grouping key
     * @return Collection of values behind requested scope.
     */
    virtual ReturnType getScoped(const Scope& scope) const = 0;

    /**
     * @return Reference to first element in the collection.
     */
    virtual const Type& front() = 0;

    /**
     * Remove first element from the collection and return it.
     * @return First element in the collection.
     */
    virtual Type pop() = 0;

    /**
     * Clear the collection.
     */
    virtual void clear() = 0;

    /**
     * Extract and remove all values related to provided key.
     * @param scope grouping key.
     * @return Collection of values that were extracted.
     */
    virtual ReturnType extractScope(const Scope& scope) = 0;

    /**
     * Remove all values related to provided key.
     * @param scope grouping key.
     * @return number of elements removed.
     */
    virtual size_t eraseScope(const Scope& scope) = 0;

    /**
     * Remove particular value from the calculation. Will remove only first occurence of it.
     * @param value value to remove.
     */
    virtual void eraseValue(const Type& value) = 0;

    /**
     * Add value into the collection.
     * @param scope grouping key.
     * @param value value to add.
     */
    virtual void emplace(const Scope& scope, const Type& value) = 0;

    /**
     * Add value into the collection.
     * @param scope grouping key.
     * @param value value to add.
     */
    virtual void emplace(const Scope& scope, Type&& value) = 0;
};

} // namespace apl

#endif // _APL_SCOPED_COLLECTION