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

#ifndef _APL_SCOPED_SET
#define _APL_SCOPED_SET

#include "apl/utils/scopedcollection.h"

namespace apl {

/**
 * Scoped implementation of the set.
 * @tparam Scope Scoping key type.
 * @tparam Type Value type.
 */
template<class Scope, class Type>
class ScopedSet : public ScopedCollection<Scope, Type, std::set<Type>, std::set<Type>> {
public:
    bool empty() const override {
        return mSet.empty();
    }

    size_t size() const override {
        return mSet.size();
    }

    const std::set<Type>& getAll() const override {
        return mSet;
    }

    const Type& front() override {
        return *mSet.begin();
    }

    Type pop() override {
        auto result = *mSet.begin();
        mSet.erase(mSet.begin());
        eraseFromScope(result);
        return result;
    }

    void clear() override {
        mSet.clear();
        mScopeToValue.clear();
    }

    std::set<Type> getScoped(const Scope& scope) const override {
        auto result = std::set<Type>();
        auto range = mScopeToValue.equal_range(scope);
        for(auto it = range.first; it != range.second; ++it) {
            result.emplace(it->second);
        }
        return result;
    }

    std::set<Type> extractScope(const Scope& scope) override {
        auto erased = getScoped(scope);
        for (auto it = mSet.begin() ; it != mSet.end() ; ) {
            if (erased.count(*it)) {
                it = mSet.erase(it);
            } else {
                it++;
            }
        }
        mScopeToValue.erase(scope);

        return erased;
    }

    size_t eraseScope(const Scope& scope) override {
        return extractScope(scope).size();
    }

    void eraseValue(const Type& value) override {
        mSet.erase(value);
        eraseFromScope(value);
    }

    void emplace(const Scope& scope, const Type& value) override {
        if (mSet.emplace(value).second) {
            mScopeToValue.emplace(scope, value);
        }
    }

    void emplace(const Scope& scope, Type&& value) override {
        if (mSet.emplace(std::forward<Type>(value)).second) {
            mScopeToValue.emplace(scope, std::forward<Type>(value));
        }
    }

private:
    void eraseFromScope(const Type& value) {
        for (auto it = mScopeToValue.begin(); it != mScopeToValue.end(); it++) {
            if (it->second == value) {
                mScopeToValue.erase(it);
                break;
            }
        }
    }

    std::multimap<Scope, Type> mScopeToValue{};
    std::set<Type> mSet;
};

} // namespace apl

#endif // _APL_SCOPED_SET