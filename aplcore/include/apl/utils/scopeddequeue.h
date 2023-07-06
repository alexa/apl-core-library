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

#ifndef _APL_SCOPED_DEQUEUE
#define _APL_SCOPED_DEQUEUE

#include "apl/utils/scopedcollection.h"

#include <deque>

namespace apl {

/**
 * Scoped implementation of the dequeue.
 * @tparam Scope Scoping key type.
 * @tparam Type Value type.
 */
template<class Scope, class Type>
class ScopedDequeue : public ScopedCollection<Scope, Type, std::deque<std::pair<Scope, Type>>> {
public:
    bool empty() const override {
        return mQueue.empty();
    }

    size_t size() const override {
        return mQueue.size();
    }

    const std::deque<std::pair<Scope, Type>>& getAll() const override {
        return mQueue;
    }

    const Type& front() override {
        return mQueue.front().second;
    }

    Type pop() override {
        auto result = mQueue.front();
        mQueue.pop_front();
        return result.second;
    }

    void clear() override {
        mQueue.clear();
    }

    std::vector<Type> getScoped(const Scope& scope) const override {
        auto result = std::vector<Type>();
        for(auto it = mQueue.begin(); it != mQueue.end(); ++it) {
            if (it->first == scope) result.emplace_back(it->second);
        }
        return result;
    }

    std::vector<Type> extractScope(const Scope& scope) override {
        auto erased = std::vector<Type>();
        for (auto it = mQueue.begin() ; it != mQueue.end() ; ) {
            if (it->first == scope) {
                erased.emplace_back(it->second);
                it = mQueue.erase(it);
            } else {
                it++;
            }
        }

        return erased;
    }

    void eraseValue(const Type& value) override {}

    size_t eraseScope(const Scope& scope) override {
        auto eraseIt =
            std::remove_if(mQueue.begin(), mQueue.end(), [scope](const std::pair<Scope, Type>& item) {
                return std::get<0>(item) == scope;
            });

        auto result = std::distance(eraseIt, mQueue.end());
        mQueue.erase(eraseIt, mQueue.end());

        return result;
    }

    void emplace(const Scope& scope, const Type& value) override {
        mQueue.emplace_back(std::make_pair(scope, value));
    }

    void emplace(const Scope& scope, Type&& value) override {
        mQueue.emplace_back(std::make_pair(scope, std::forward<Type>(value)));
    }

private:
    std::deque<std::pair<Scope, Type>> mQueue;
};

} // namespace apl

#endif // _APL_SCOPED_DEQUEUE