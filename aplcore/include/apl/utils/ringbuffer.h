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

#ifndef _APL_RINGBUFFER_H
#define _APL_RINGBUFFER_H

#include <memory>
#include <cassert>

namespace apl {

/**
 * Simple implementation of ring (cycle) buffer. In case of addition of new item happens when buffer
 * already full oldest item will be discarded.
 * @tparam T Class to contain.
 */
template<class T>
class RingBuffer {
public:
    /// STL style typedefs
    typedef T         value_type;
    typedef T*        pointer;
    typedef T&        reference;
    typedef const T&  const_reference;
    typedef size_t    size_type;

    explicit RingBuffer(size_t size) : mBuf(std::unique_ptr<T[]>(new T[size])), mHead(0), mTail(0),
        mMaxSize(size), mFull(false) {}

    /// Simple control functionality
    void clear() { mHead = mTail; mFull = false;}
    bool empty() const { return !mFull && (mHead == mTail); }
    bool full() const { return mFull; }
    size_type capacity() const { return mMaxSize; }

    size_type size() const {
        if (mFull) return mMaxSize;
        if (mHead >= mTail) return mHead - mTail;
        return mMaxSize - (mTail - mHead);
    }

    void enqueue(const value_type& item) {
        mBuf[mHead] = item;
        incrementHead();
    }

    void enqueue(value_type&& item) {
        mBuf[mHead] = std::move(item);
        incrementHead();
    }

    value_type dequeue() {
        assert(!empty());

        mFull = false;

        auto item = mBuf[mTail];
        mTail = (mTail + 1) % mMaxSize;

        return item;
    }

    /// Random access
    reference operator[](size_t idx) { return mBuf[(mTail + idx) % mMaxSize]; }
    const_reference operator[](size_t idx) const { return mBuf[(mHead + idx) % mMaxSize]; }
    reference front() { return operator[](0); }
    reference back() { return operator[](size() - 1); }

    /// Iterator definitions
    template<typename K, typename elem_type = typename K::value_type>
    class RingBufferIterator : public std::iterator<std::bidirectional_iterator_tag, elem_type> {
    public:
        typedef RingBufferIterator self_type;
        typedef size_t    size_type;

        RingBufferIterator(K* b, size_type pos) : mRingBuf(b), mPos(pos) {}

        elem_type& operator*() const { return (*mRingBuf)[mPos]; }
        elem_type* operator->() const { return &(operator*()); }

        self_type& operator++() { ++mPos; return *this; }
        self_type operator++(int) { return self_type(mRingBuf, mPos++); }
        self_type& operator--() { --mPos; return *this; }
        self_type operator--(int) { return self_type(mRingBuf, mPos--); }

        bool operator==(const RingBufferIterator& other) const { return mRingBuf == other.mRingBuf && mPos == other.mPos; }
        bool operator!=(const RingBufferIterator& other) const { return mRingBuf != other.mRingBuf || mPos != other.mPos; }

    private:
        K* mRingBuf;
        size_type mPos;
    };

    typedef RingBufferIterator<RingBuffer, RingBuffer::value_type> iterator;
    typedef RingBufferIterator<const RingBuffer, const RingBuffer::value_type> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    /// Iterator access
    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, size()); }
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_iterator cbegin() { return const_iterator(this, 0); }
    const_iterator cend() { return const_iterator(this, size()); }
    const_reverse_iterator crbegin() { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() { return const_reverse_iterator(cbegin()); }


private:
    std::unique_ptr<T[]> mBuf;
    size_t mHead;
    size_t mTail;
    size_t mMaxSize;
    bool mFull;

    void incrementHead() {
        if (full()) {
            mTail = (mTail + 1) % mMaxSize;
        }

        mHead = (mHead + 1) % mMaxSize;
        mFull = mHead == mTail;
    }
};

} // namespace apl

#endif //_APL_RINGBUFFER_H
