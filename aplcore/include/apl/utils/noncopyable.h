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

#ifndef _APL_NONCOPYABLE_H
#define _APL_NONCOPYABLE_H

namespace apl {

/**
 * A trivial mix-in class that prevents a component from being directly copied.  Add this class as a public
 * superclass and it will block the copy constructor and copy assignment operator for any subclasses.  As a good
 * rule, if a class supports std::enable_shared_from_this<> you should also inherit from NonCopyable to prevent
 * someone from making a stack-based copy of something that is fundamentally a heap-based object.
 *
 * For example, the Component class inherits from std::enable_shared_from_this<>, so it is a good idea to make
 * it non-copyable:
 *
 *     class Component : public NonCopyable,
 *                       public std::enable_shared_from_this<Component>
 *                       ...and other
 *
 * This blocks the easy "auto" error where the developer forgets to make the auto value a reference.  For example:
 *
 *     void func(Component& component) {
 *       auto c = component;   // ERROR!  The reference is dropped; this is a copy operation
 *       auto& c = component;  // Okay
 *     }
 */
class NonCopyable
{
public:
    NonCopyable() = default;
    ~NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    void operator=(const NonCopyable&) = delete;
};

} // namespace apl

#endif // _APL_NONCOPYABLE_H
