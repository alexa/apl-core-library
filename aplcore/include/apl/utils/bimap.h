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

#ifndef _APL_BIMAP_H
#define _APL_BIMAP_H

#include <vector>
#include <map>
#include <algorithm>

namespace apl {

/**
 * A Bimap is a two-direction mapping between items of type "A" and items
 * of type "B".  The types need to be distinct.
 *
 * There is no guarantee that a Bimap is a one-to-one mapping.  A Bimap may
 * be initialized with repeating values, in which case the first value listed
 * will be deemed the canonical one.  For example,
 * \code{.cpp}
 *   Bimap<int, std::string> test = {
 *     { 10, "dog" },
 *     { 20, "dog" },
 *     { 20, "cat" }
 *   };
 * \endcode
 *
 * In this example:
 *
 *     test.at("dog")   -> 10
 *     test.at("cat")   -> 20
 *     test.at(10)      -> "dog"
 *     test.at(20)      -> "dog"
 *
 * @tparam A The first type.
 * @tparam B The second type.
 */
template<class A, class B>
class Bimap {
public:
    Bimap(std::initializer_list<std::pair<A, B>> list)
        : mOriginal(std::move(list))
    {
        for (auto& m : mOriginal) {
            mAtoB.emplace(m.first, m.second);
            mBtoA.emplace(m.second, m.first);
        }
    }

    const A& at(B x) const { return mBtoA.at(x); }
    const B& at(A x) const { return mAtoB.at(x); }

    bool has(B x) const { return mBtoA.count(x); }
    bool has(A x) const { return mAtoB.count(x); }

    std::vector<A> all(B x) const {
        std::vector<A> result;
        for (const auto& m : mOriginal) {
            if (m.second == x)
                result.push_back(m.first);
        }
        return result;
    }

    std::vector<B> all(A x) const {
        std::vector<B> result;
        for (const auto& m : mOriginal) {
            if (m.first == x)
                result.push_back(m.second);
        }
        return result;
    }

    std::size_t size() const { return mAtoB.size(); }

    A get(B x, A defvalue) const {
        auto it = mBtoA.find(x);
        return (it != mBtoA.end() ? it->second : defvalue);
    }

    B get(A x, B defvalue) const {
        auto it = mAtoB.find(x);
        return (it != mAtoB.end() ? it->second : defvalue);
    }

    typename std::map<A,B>::const_iterator find(A x) const {
        return mAtoB.find(x);
    }

    typename std::map<B,A>::const_iterator find(B x) const {
        return mBtoA.find(x);
    }

    typename std::map<A,B>::const_iterator begin() const { return mAtoB.begin(); }
    typename std::map<A,B>::const_iterator end() const { return mAtoB.end(); }

    typename std::map<B,A>::const_iterator beginBtoA() const { return mBtoA.begin(); }
    typename std::map<B,A>::const_iterator endBtoA() const { return mBtoA.end(); }

    A append(B b) {
        auto it = mBtoA.find(b);
        if (it != mBtoA.end())
            return it->second;

        int a = maxA() + 1;
        mAtoB.emplace(a, b);
        mBtoA.emplace(b, a);
        return a;
    }

    A maxA() {
        return std::max_element(mAtoB.begin(), mAtoB.end(), [](const std::pair<A,B>& a, const std::pair<A,B>& b) {
            return a.first < b.first;
        })->first;
    }

private:
    std::vector<std::pair<A, B>> mOriginal;
    std::map<A, B> mAtoB;
    std::map<B, A> mBtoA;
};

} // namespace apl

#endif //_APL_BIMAP_H
