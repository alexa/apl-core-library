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

#ifndef _APL_CONTEXT_WRAPPER_H
#define _APL_CONTEXT_WRAPPER_H

#include "apl/primitives/objecttype.h"
#include "apl/engine/context.h"

namespace apl {

/**
 * Wrap a weak pointer to a Context so that it can be stored in an Object.
 *
 * The context wrapper acts like a map to retrieve bound properties in the context (so "get", "has", and "opt" work).
 * The context wrapper returns 0 for the size of the map and an empty object upon serialization because the intended
 * use of the ContextWrapper is to expose bound properties to the "event.source.bind" and "event.target.bind"
 * properties.  If we serialized the entire data-binding context then the user could deliberately or inadvertently
 * send the entire data-binding context with a command like { "type": "SendEvent", "arguments": [ "${event}" ] }
 */
class ContextWrapper : public ObjectData {
public:
    static std::shared_ptr<ContextWrapper> create(const ConstContextPtr& context) {
        return std::make_shared<ContextWrapper>(context);
    }

    ContextWrapper(const ConstContextPtr& context) : mContext(context) {}

    std::string toDebugString() const override {
        return "Context<>";
    }

    Object get(const std::string& key) const override {
        auto context = mContext.lock();
        if (context)
            return context->opt(key);

        return Object::NULL_OBJECT();
    }

    bool has(const std::string& key) const override {
        auto context = mContext.lock();
        if (context)
            return context->has(key);

        return false;
    }

    Object opt(const std::string& key, const Object& def) const override {
        auto context = mContext.lock();
        if (context) {
            auto cr = context->find(key);
            if (!cr.empty())
                return cr.object().value();
        }

        return def;
    }

    // Context wrappers intentionally return a size of zero
    std::uint64_t size() const override { return 0; }

    bool truthy() const override { return !mContext.expired(); }

    bool empty() const override { return mContext.expired(); }

    bool operator==(const ContextWrapper& rhs) const {
        return !mContext.owner_before(rhs.mContext) && !rhs.mContext.owner_before(mContext);
    }

    // Context wrappers intentionally return an empty object
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override {
        return rapidjson::Value(rapidjson::kObjectType);
    }

    class ObjectType final : public MapLikeObjectType<ContextWrapper> {
    public:
        rapidjson::Value serialize(
            const Object::DataHolder& dataHolder,
            rapidjson::Document::AllocatorType& allocator) const override
        {
            return dataHolder.data->serialize(allocator);
        }

        bool equals(const Object::DataHolder& lhs, const Object::DataHolder& rhs) const override {
            return *std::static_pointer_cast<ContextWrapper>(lhs.data) ==
                   *std::static_pointer_cast<ContextWrapper>(rhs.data);
        }
    };

private:
    std::weak_ptr<const Context> mContext;
};

}
#endif // _APL_CONTEXT_WRAPPER_H
