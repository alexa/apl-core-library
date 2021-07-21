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
 *            CounterPair itemsDelta() { return mCounter.itemsDelta(); }
 *        private:
 *            Counter<MyClass> mCounter;
 *    #endif
 *    };
 *
 * 2. Inherit from the class - but if you do this, you don't get separate counters for subclasses
 *
 *    class MyClass : public Counter<MyClass> {};
 *
 */

struct CounterPair {
    using size_type = unsigned int;

    size_type created;
    size_type destroyed;

    CounterPair(size_type created, size_type destroyed) : created(created), destroyed(destroyed) {}
    bool operator==(const CounterPair& rhs) {
        return created - destroyed == rhs.created - rhs.destroyed;
    }

    CounterPair operator-=(const CounterPair& rhs) {
        return {created - rhs.created, destroyed - rhs.destroyed};
    }

    friend bool operator==(const CounterPair& lhs, const CounterPair& rhs) {
        return lhs.created - lhs.destroyed == rhs.created - rhs.destroyed;
    }

    friend CounterPair operator-(const CounterPair& lhs, const CounterPair& rhs) {
        return {lhs.created - rhs.created, lhs.destroyed - rhs.destroyed};
    }
};

template<typename T>
class Counter {
#ifdef DEBUG_MEMORY_USE
    using size_type = CounterPair::size_type;

public:
    Counter() { sItemsCreated++; }
    Counter(const Counter&) { sItemsCreated++; }
    ~Counter() { sItemsDestroyed++; }

    static CounterPair itemsDelta() { return {sItemsCreated, sItemsDestroyed}; }

private:
    static size_type sItemsCreated;
    static size_type sItemsDestroyed;
#endif
};

#ifdef DEBUG_MEMORY_USE
template<typename T>
CounterPair::size_type Counter<T>::sItemsDestroyed = 0;

template<typename T>
CounterPair::size_type Counter<T>::sItemsCreated = 0;
#endif

} // namespace apl

#endif //_APL_CORE_MEMORY_TRACKER_H
