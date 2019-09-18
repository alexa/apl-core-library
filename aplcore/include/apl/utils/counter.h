/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef _APL_CORE_COUNTER_H
#define _APL_CORE_COUNTER_H

namespace apl {

/**
 * Mixin class for counting the number of items created and destroyed.  Inspired by the Scott Meyers
 * article in the C/C++ Users Journal (April 1998).
 *
 * There are two ways to use this:
 *
 * 1. Include as a member variable of a class - cost is that the size of the class increases
 *
 *    class MyClass {
 *    #ifdef DEBUG_MEMORY_USE
 *        public:
 *            static Counter<MyClass>::Pair itemsDelta() { return mCounter.itemsDelta(); }
  *        private:
 *            Counter<MyClass> mCounter;
 *    #endif
 *    };
 *
 * 2. Inherit from the class - but if you do this, you don't get separate counters for subclasses
 *
 *    class MyClass : private Counter<MyClass> {
 *    #ifdef DEBUG_MEMORY_USE
 *        public:
 *            using Counter<MyClass>::itemsDelta;
 *    #endif
 *    };
 *
 * @tparam T The class you are instantiated this for.
 * @tparam size_type The counter type.
 */
template<typename T, typename size_type=unsigned int>
class Counter {
public:
    struct Pair {
        size_type created;
        size_type destroyed;

        Pair(size_type created, size_type destroyed) : created(created), destroyed(destroyed) {}
        bool operator==(const Pair& rhs) { return created - destroyed == rhs.created - rhs.destroyed; }
        Pair operator-=(const Pair& rhs) { return Pair(created - rhs.created, destroyed - rhs.destroyed); }

        friend bool operator==(const Pair& lhs, const Pair& rhs) {
            return lhs.created - lhs.destroyed == rhs.created - rhs.destroyed;
        }

        friend Pair operator-(const Pair& lhs, const Pair& rhs) {
            return Pair(lhs.created - rhs.created, lhs.destroyed - rhs.destroyed);
        }
    };
#ifdef DEBUG_MEMORY_USE
public:
    Counter() { sItemsCreated++; }
    Counter(const Counter&) { sItemsCreated++; }
    ~Counter() { sItemsDestroyed++; }

    static size_type itemsCreated() { return sItemsCreated; }
    static size_type itemsDestroyed() { return sItemsDestroyed; }
    static Pair itemsDelta() { return Pair(sItemsCreated, sItemsDestroyed); }
    static void reset() {sItemsDestroyed = 0; sItemsCreated = 0;}
private:
    static size_type sItemsCreated;
    static size_type sItemsDestroyed;
#endif
};

#ifdef DEBUG_MEMORY_USE
template<typename T, typename size_type>
size_type Counter<T, size_type>::sItemsDestroyed = 0;

template<typename T, typename size_type>
size_type Counter<T, size_type>::sItemsCreated = 0;
#endif

} // namespace apl

#endif //_APL_CORE_MEMORY_TRACKER_H
