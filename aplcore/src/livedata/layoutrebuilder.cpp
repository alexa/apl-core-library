/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/livedata/livearraychange.h"
#include "apl/livedata/layoutrebuilder.h"
#include "apl/component/corecomponent.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/engine/builder.h"

namespace apl {

static const bool DEBUG_WALKER = false;

/**
 * Convenience class to walk through the existing components in a layout and reconcile them with the new items
 */
class ChildWalker {
public:
    /**
     * Construct the walker and step over a "firstItem".
     * @param layout
     */
    ChildWalker(const CoreComponentPtr& layout, bool hasFirstItem)
        : mLayout(layout), mIndex(hasFirstItem ? 1 : 0)
    {
        LOG_IF(DEBUG_WALKER) << "mIndex=" << mIndex << " total=" << mLayout->getChildCount();
    }

    /**
     * Throw away any remaining children, but save the "lastItem" child if it exists
     */
    void finish(bool hasLastItem) {
        LOG_IF(DEBUG_WALKER) << "mIndex=" << mIndex << " total=" << mLayout->getChildCount() << " hasLast=" << hasLastItem;

        int lastIndex = mLayout->getChildCount();
        if (hasLastItem)
            lastIndex -= 1;

        for (int i = mIndex ; i < lastIndex ; i++)
            mLayout->removeChildAt(mIndex, true);
    }

    /**
     * Throw away children until we reach one with "dataIndex" equal to oldIndex
     * If we pass it, that means it didn't inflate last time, so we can stop.
     * @param oldIndex
     * @return True if we found it; false if we've gone beyond
     */
    bool advanceUntil(int oldIndex) {
        LOG_IF(DEBUG_WALKER) << " oldIndex=" << oldIndex << " mIndex=" << mIndex << " total=" << mLayout->getChildCount();
        while (mIndex < mLayout->getChildCount()) {
            auto child = mLayout->getCoreChildAt(mIndex);
            auto dataIndex = child->getContext()->opt("dataIndex");
            if (dataIndex.isNumber()) {
                auto index = dataIndex.getInteger();
                if (index >= oldIndex) {
                    LOG_IF(DEBUG_WALKER) << " matched index " << index;
                    return index == oldIndex;
                }
                LOG_IF(DEBUG_WALKER) << " found data index of " << index << " but it didn't match";
            }
            LOG_IF(DEBUG_WALKER) << " removing child at index " << mIndex;
            mLayout->removeChildAt(mIndex, true);
        }

        LOG(LogLevel::ERROR) << "Failed to find child with dataIndex of " << oldIndex;
        return false;
    }

    void advance() {
        LOG_IF(DEBUG_WALKER) << "mIndex=" << mIndex << " total=" << mLayout->getChildCount();
        mIndex++;
    }

    CoreComponentPtr currentChild() {
        return mLayout->getCoreChildAt(mIndex);
    }

private:
    CoreComponentPtr mLayout;
    int mIndex;
};

/**
 * Convenience method to search through contexts to find with the magic "_token" that matches.
 */
inline ContextPtr findToken(const CoreComponentPtr& component, int token)
{
    auto c = component->getContext();
    auto p = c->find("_token");
    auto value = p.object().value();
    if (value.isNumber() && value.getInteger() == token)
        return p.context();

    LOG(LogLevel::WARN) << "Unable to find token parent of context. Token=" << token;
    return nullptr;
}


int LayoutRebuilder::sRebuilderToken = 100;

std::shared_ptr<LayoutRebuilder>
LayoutRebuilder::create(const ContextPtr& context,
                        const CoreComponentPtr& layout,
                        const std::shared_ptr<LiveArrayObject>& array,
                        const std::vector<Object>& items,
                        const Path& childPath,
                        bool numbered)
{
    auto self = std::make_shared<LayoutRebuilder>(context, layout, array, items, childPath, numbered);
    layout->attachRebuilder(self);
    return self;
}

LayoutRebuilder::LayoutRebuilder(const ContextPtr& context,
                                 const CoreComponentPtr& layout,
                                 const std::shared_ptr<LiveArrayObject>& array,
                                 const std::vector<Object>& items,
                                 const Path& childPath,
                                 bool numbered)
    : mContext(context),
      mLayout(layout),
      mArray(array),
      mItems(items),
      mChildPath(childPath),
      mNumbered(numbered),
      mRebuilderToken(sRebuilderToken++)
{
    registerObjectWatcher(array);
}

LayoutRebuilder::~LayoutRebuilder()
{
    unregisterObjectWatcher();
}

/**
 * Add the core children to the layout.  The first/last child is handled by the builder.
 */
void
LayoutRebuilder::build()
{
    auto layout = mLayout.lock();
    auto array = mArray.lock();

    if (!layout || !array) {
        LOG(LogLevel::ERROR) << "Attempting to build a layout without a layout or data array";
        return;
    }

    int ordinal = 1;
    int index = 0;

    // Note: we do NOT evaluate the array items.  Why?  Well, I'm worried about consistency
    // TODO: Move some of this common child-generation logic into a separate routine and share it with the rebuild code.
    auto length = array->size();
    for (size_t dataIndex = 0; dataIndex < length; dataIndex++) {
        const auto& data = array->at(dataIndex);
        auto childContext = Context::create(mContext);
        childContext->putSystemWriteable("data", data);  // This can be changed
        childContext->putSystemWriteable("index", index);
        childContext->putSystemWriteable("length", length);
        childContext->putSystemWriteable("dataIndex", dataIndex);  // This is an addition
        childContext->putSystemWriteable("_token", mRebuilderToken);  // Drop a token for later sanity checking

        if (mNumbered)
            childContext->putSystemWriteable("ordinal", ordinal);

        Properties childProps;
        auto child = Builder().expandSingleComponentFromArray(childContext,
                                                              mItems,
                                                              childProps,
                                                              layout,
                                                              mChildPath);
        if (child && child->isValid()) {
            layout->appendChild(child, false);
            index++;

            if (mNumbered) {
                int numbering = child->getCalculated(kPropertyNumbering).getInteger();
                if (numbering == kNumberingNormal) ordinal++;
                else if (numbering == kNumberingReset) ordinal = 1;
            }
        }
    }
}


void
LayoutRebuilder::rebuild()
{
    auto layout = mLayout.lock();
    auto array = mArray.lock();

    if (!layout || !array) {
        LOG(LogLevel::ERROR) << "Attempting to rebuild a layout without a layout or data array";
        return;
    }

    // The old child array is in the correct order (we don't have reordering, but probably the
    // wrong location.  We'll walk through the new list and the old list, deleting and inserting
    // items as appropriate.

    auto walker = ChildWalker(layout, mHasFirstItem);

    int ordinal = 1;
    int index = 0;

    // Walk the list of new items
    for (int newIndex = 0 ; newIndex < array->size() ; newIndex++) {
        const auto& data = array->at(newIndex);
        auto p = array->newToOld(newIndex);
        auto oldIndex = p.first;
        auto needsRefresh = p.second;

        if (oldIndex == -1) {  // Insert a new child - this one doesn't exist
            auto childContext = Context::create(mContext);
            childContext->putSystemWriteable("data", data);
            childContext->putSystemWriteable("index", index);
            childContext->putSystemWriteable("length", array->size());
            childContext->putSystemWriteable("dataIndex", newIndex);  // This is an addition
            childContext->putSystemWriteable("_token", mRebuilderToken);  // Drop a token for later sanity checking

            if (mNumbered)
                childContext->putConstant("ordinal", ordinal);

            Properties childProps;
            auto child = Builder().expandSingleComponentFromArray(childContext,
                                                                  mItems,
                                                                  childProps,
                                                                  layout,
                                                                  mChildPath);
            if (child && child->isValid()) {
                layout->insertChild(child, index + (mHasFirstItem ? 1 : 0), true);
                index++;
                walker.advance();  // Must step over the child we just inserted

                if (mNumbered) {
                    int numbering = child->getCalculated(kPropertyNumbering).getInteger();
                    if (numbering == kNumberingNormal) ordinal++;
                    else if (numbering == kNumberingReset) ordinal = 1;
                }
            }
        } else if (walker.advanceUntil(oldIndex)) {
            // If we get here we found the old index and it should be updated
            auto child = walker.currentChild();
            auto childContext = findToken(child, mRebuilderToken);  // Search up through contexts to find the right one to modify
            if (childContext) {
                // TODO: Can we optimize recalculation by batching these and only changing some of them?
                childContext->systemUpdateAndRecalculate("index", index, true);

                if (needsRefresh)
                    childContext->systemUpdateAndRecalculate("data", data, true);

                childContext->systemUpdateAndRecalculate("length", array->size(), true);
                childContext->systemUpdateAndRecalculate("dataIndex", newIndex, true);
                childContext->systemUpdateAndRecalculate("ordinal", ordinal, true);

                index += 1;
                walker.advance();

                if (mNumbered) {
                    int numbering = child->getCalculated(kPropertyNumbering).getInteger();
                    if (numbering == kNumberingNormal) ordinal++;
                    else if (numbering == kNumberingReset) ordinal = 1;
                }
            }
        }
    }

    walker.finish(mHasLastItem);

    // Allow lazy components to process new children layout (if any).
    layout->processLayoutChanges(true);
}

void
LayoutRebuilder::notifyItemOnScreen(int idx) {
    auto layout = mLayout.lock();
    auto array = mArray.lock();
    if (!layout || !array)
        return;

    auto child = mLayout.lock()->getCoreChildAt(idx);
    auto dataIndex = child->getContext()->opt("dataIndex");
    if (dataIndex.isNumber()) {
        array->ensure(dataIndex.getInteger());
    }
}


} // namespace apl
