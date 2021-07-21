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

#ifndef _APL_LIVE_ARRAY_H
#define _APL_LIVE_ARRAY_H

#include "apl/livedata/liveobject.h"
#include "apl/utils/counter.h"

namespace apl {

class LiveArrayChange;

/**
 * A LiveArray is a public class that holds an array of Objects that changes
 * over time.  LiveArrays are created and modified by ViewHosts.  For example:
 *
 *     // Before creating the root context:
 *     LiveArray myArray({"Object A", "Object B", "Object C"});
 *     rootConfig.addLive("MyLiveArray", myArray);
 *
 *     // After the root context has been created:
 *     myArray.push_back("Object D");
 *     myArray.remove(0);  // Delete the first item
 *
 * Inside of the APL document the LiveArray may be used normally in the
 * data-binding context.  If used as the "data" target for a multi-child component,
 * a LayoutRebuilder will be assigned to add and remove components from the component
 * as the LiveArray changes.
 *
 * For example:
 *
 *     {
 *       "type": "Sequence",
 *       "data": "${MyLiveArray}",
 *       "item": {
 *         "type": "Text",
 *         "text": "${index+1}. ${data}"
 *       }
 *     }
 *
 * As per the above example, when this sequence is first inflated there
 * will be three text components:  "1. Object A", "2. Object B", "3. Object C".
 * After the push_back and remove commands, the text components will now
 * be "1. Object B", "2. Object C", "3. Object D".
 *
 * The same LiveArray may be used by multiple RootContext objects. This allows
 * an application to create a single LiveArray to track a source of changing data
 * and display it in multiple view hosts simultaneously.
 *
 * Please note that changing LiveArray values has a limited effect on the
 * component hierarchy. A component with children bound to a live array will
 * have new children inserted and removed, but changing the value stored in
 * an existing LiveArray index will not cause that child to be re-inflated.
 */
class LiveArray : public LiveObject, public Counter<LiveArray> {
public:
    using size_type = ObjectArray::size_type;
    using ChangeCallback = std::function<void(const LiveArrayChange&)>;

    /**
     * Create an empty LiveArray
     * @return The LiveArray
     */
    static LiveArrayPtr create() {
        return std::make_shared<LiveArray>(ObjectArray{});
    }

    /**
     * Create a LiveArray with an initial Object vector
     * @param array The vector of Objects
     * @return The LiveArray
     */
    static LiveArrayPtr create(ObjectArray&& array) {
        return std::make_shared<LiveArray>(std::move(array));
    }

    /**
     * Argument-based constructor.  Do not call this directly; use the
     * create(ObjectArray&&) method.
     * @param array The array to start with.
     */
    explicit LiveArray(ObjectArray&& array) : mArray(std::move(array)) {}

    // Override from LiveObject
    Object::ObjectType getType() const override { return Object::ObjectType::kArrayType; }

    /**
     * Check to see if there are no elements in the array.
     * @return True if the array is empty.
     */
    bool empty() const noexcept {
        return mArray.empty();
    }

    /**
     * Clear all elements from the array
     */
    void clear();

    /**
     * @return The number of elements in the array.
     */
    size_type size() const noexcept {
        return mArray.size();
    }

    /**
     * Retrieve an object in the array.  If position is out of bounds, a NULL object
     * will be returned.
     * @param position The index of the object to return.
     * @return The object or a null object.
     */
    Object at( size_type position ) const {
        if (position >= mArray.size())
            return Object::NULL_OBJECT();

        return mArray.at(position);
    }

    /**
     * Insert a new object into the array.  The position must fall within the range [0,size]
     * @param position The position at which to insert the object.
     * @param value The object to insert.
     * @return True if position was valid and the object was inserted.
     */
    bool insert( size_type position, const Object& value );

    /**
     * Move a new object into the array.  The position must fall within the range [0,size]
     * @param position The position at which to insert the object.
     * @param value The object to insert.
     * @return True if position was valid and the object was inserted.
     */
    bool insert( size_type position, Object && value );

    /**
     * Insert a range of objects into the array.  The position must fall within the range [0,size]
     * @tparam InputIt The source iterator type.
     * @param position The position at which to insert the objects.
     * @param first The starting iterator position.
     * @param last The ending iterator position.
     * @return True if the position was valid and at least one object was inserted.
     */
    template<class InputIt> bool
    insert( size_type position, InputIt first, InputIt last )
    {
        auto count = std::distance(first, last);
        if (count == 0 || position > mArray.size())
            return false;

        mArray.insert(mArray.begin() + position, first, last);
        broadcastInsert(position, count);
        return true;
    }

    /**
     * Remove an object from the array.  The position must fall within the range [0,size)
     * @param position The position at which the object will be removed.
     * @return True if the position was valid and the object was removed.
     */
    bool remove( size_type position, size_type count = 1 );

    /**
     * Change the value of an object at a position
     * @param position The position where the object will be changed
     * @param value The value it will be changed to.
     * @return True if the position was valid and the object was changed
     */
    bool update( size_type position, const Object& value );

    /**
     * Change the value of an object at a position
     * @param position The position where the object will be changed
     * @param value The value it will be changed to
     * @return True if the position was valid and the object was changed
     */
    bool update( size_type position, Object&& value );

    /**
     * Update a range of objects in the array.  The starting position must fall within the range
     * [0,size - count], where count is the number of objects being modified.
     * @tparam InputIt The source iterator type
     * @param position The starting position where objects should be updated
     * @param first The starting iterator position
     * @param last The ending iterator position
     * @return True if the position was valid and at least one object was updated.
     */
    template<class InputIt> bool
    update( size_type position, InputIt first, InputIt last )
    {
        auto count = std::distance(first, last);
        if (count == 0 || count > mArray.size() || position > mArray.size() - count)
            return false;

        for (auto it = first ; it != last ; it++)
            mArray.at(position + std::distance(first, it)) = *it;

        broadcastUpdate(position, count);
        return true;
    }

    /**
     * Push an object onto the back of the array.
     * @param value The object to push.
     */
    void push_back( const Object& value );

    /**
     * Push an object onto the back of the array
     * @param value The object to push.
     */
    void push_back( Object&& value );

    /**
     * Push a range of objects onto the array.
     * @tparam InputIt The source iterator type
     * @param first The starting iterator position
     * @param last The ending iterator position
     * @return True if at least one object was pushed onto the array.
     */
    template<class InputIt> bool
    push_back( InputIt first, InputIt last )
    {
        if (first == last)
            return false;

        auto position = mArray.size();
        for (auto it = first ; it != last ; it++)
            mArray.push_back(*it);

        broadcastInsert(position, std::distance(first, last));
        return true;
    }

    /**
     * @return The internal array.  Generally you should not use this.
     */
    const std::vector<Object>& getArray() const { return mArray; }

    /**
     * Add a change callback to this LiveArray.
     * @param callback The callback to invoke whenever the array changes.
     * @return An opaque token to be used to remove the change callback
     */
    int addChangeCallback(ChangeCallback&& callback) {
        int token = mChangeCallbackToken++;
        mChangeCallbacks.emplace(token, std::move(callback));
        return token;
    }

    /**
     * Remove a change callback from this LiveArray
     * @param token The opaque token from the addChangeCallback(ChangeCallback) method.
     */
    void removeChangeCallback(int token) {
        mChangeCallbacks.erase(mChangeCallbacks.find(token));
    }

private:
    void broadcast( const LiveArrayChange& command );
    void broadcastInsert( size_type position, size_type count );
    void broadcastUpdate( size_type position, size_type count );

private:
    ObjectArray mArray;
    int mChangeCallbackToken = 100;
    std::map<int, ChangeCallback> mChangeCallbacks;
};

} // namespace apl

#endif // _APL_LIVE_ARRAY_H
