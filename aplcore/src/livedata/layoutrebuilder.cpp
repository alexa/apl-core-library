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

#include "apl/livedata/layoutrebuilder.h"

#include "apl/component/corecomponent.h"
#include "apl/content/content.h"
#include "apl/document/coredocumentcontext.h"
#include "apl/engine/builder.h"
#include "apl/livedata/livearraychange.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/utils/constants.h"

namespace apl {

const std::string COMPONENT_REBUILDER_TOKEN = "_token";

static const bool DEBUG_WALKER = false;

/**
 * Convenience class to walk through the existing components in a layout and reconcile them with the new items
 */
class ChildWalker {
public:
    /**
     * Construct the walker and step over a "firstItem".
     * @param layout The layout
     * @param hasFirstItem True if a "firstItem" property was specified that needs to be skipped.
     */
    ChildWalker(const CoreComponentPtr& layout, bool hasFirstItem)
        : mLayout(layout), mIndex(hasFirstItem ? 1 : 0)
    {
        LOG_IF(DEBUG_WALKER).session(layout) << "mIndex=" << mIndex << " total=" << mLayout->getChildCount();
    }

    /**
     * Throw away any remaining children, but save the "lastItem" child if it exists
     */
    void finish(bool hasLastItem) {
        LOG_IF(DEBUG_WALKER).session(mLayout) << "mIndex=" << mIndex << " total=" << mLayout->getChildCount() << " hasLast=" << hasLastItem;

        int lastIndex = mLayout->getChildCount();
        if (hasLastItem)
            lastIndex -= 1;

        for (int i = mIndex ; i < lastIndex ; i++)
            mLayout->removeChildAt(mIndex, true);
    }

    /**
     * Throw away children until we reach one with "dataIndex" equal to oldIndex
     * If we pass it, that means it didn't inflate last time, so we can stop.
     * @param oldIndex The old index to search for
     * @return True if we found it; false if we've gone beyond
     */
    bool advanceUntil(int oldIndex) {
        LOG_IF(DEBUG_WALKER).session(mLayout) << " oldIndex=" << oldIndex << " mIndex=" << mIndex << " total=" << mLayout->getChildCount();
        while (mIndex < mLayout->getChildCount()) {
            auto child = mLayout->getCoreChildAt(mIndex);
            auto dataIndex = child->getContext()->opt(COMPONENT_DATA_INDEX);
            if (dataIndex.isNumber()) {
                auto index = dataIndex.getInteger();
                if (index >= oldIndex) {
                    LOG_IF(DEBUG_WALKER).session(mLayout) << " matched index " << index;
                    return index == oldIndex;
                }
                LOG_IF(DEBUG_WALKER).session(mLayout) << " found data index of " << index << " but it didn't match";
            }
            LOG_IF(DEBUG_WALKER).session(mLayout) << " removing child at index " << mIndex;
            mLayout->removeChildAt(mIndex, true);
        }

        LOG(LogLevel::kError).session(mLayout) << "Failed to find child with dataIndex of " << oldIndex;
        return false;
    }

    void advance() {
        LOG_IF(DEBUG_WALKER).session(mLayout) << "mIndex=" << mIndex << " total=" << mLayout->getChildCount();
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
    auto p = c->find(COMPONENT_REBUILDER_TOKEN);
    auto value = p.object().value();
    if (value.isNumber() && value.getInteger() == token)
        return p.context();

    LOG(LogLevel::kWarn).session(component) << "Unable to find token parent of context. Token=" << token;
    return nullptr;
}


int LayoutRebuilder::sRebuilderToken = 100;

std::shared_ptr<LayoutRebuilder>
LayoutRebuilder::create(const ContextPtr& context,
                        const CoreComponentPtr& layout,
                        const CoreComponentPtr& old,
                        const LiveArrayObjectPtr& array,
                        const ObjectArray& items,
                        const Path& childPath,
                        bool numbered)
{
    auto self = std::make_shared<LayoutRebuilder>(context, layout, old, array, items, childPath, numbered);
    layout->attachRebuilder(self);
    return self;
}

LayoutRebuilder::LayoutRebuilder(const ContextPtr& context,
                                 const CoreComponentPtr& layout,
                                 const CoreComponentPtr& old,
                                 const LiveArrayObjectPtr& array,
                                 const ObjectArray& items,
                                 const Path& childPath,
                                 bool numbered)
    : mContext(context),
      mLayout(layout),
      mOld(old),
      mArray(array),
      mItems(items),
      mChildPath(childPath),
      mNumbered(numbered),
      mRebuilderToken(sRebuilderToken++)
{
}

LayoutRebuilder::~LayoutRebuilder()
{
    if (mCallbackToken != -1) {
        auto array = mArray.lock();
        if (array)
            array->removeFlushCallback(mCallbackToken);
        mCallbackToken = -1;
    }
}

ContextPtr
LayoutRebuilder::buildBaseChildContext(const LiveArrayObjectPtr& array, size_t dataIndex, size_t insertIndex)
{
    auto length = array->size();
    const auto& data = array->at(dataIndex);
    auto childContext = Context::createFromParent(mContext);
    childContext->putSystemWriteable(COMPONENT_DATA, data);  // This can be changed
    childContext->putSystemWriteable(COMPONENT_INDEX, insertIndex);
    childContext->putSystemWriteable(COMPONENT_LENGTH, length);
    childContext->putSystemWriteable(COMPONENT_DATA_INDEX, dataIndex);  // This is an addition
    childContext->putSystemWriteable(COMPONENT_REBUILDER_TOKEN, mRebuilderToken);  // Drop a token for later sanity checking
    return childContext;
}

/**
 * Add the core children to the layout.  The first/last child is handled by the builder.
 */
void
LayoutRebuilder::build(bool useDirtyFlag)
{
    auto layout = mLayout.lock();
    auto array = mArray.lock();

    if (!layout || !array) {
        LOG(LogLevel::kError).session(layout) << "Attempting to build a layout without a layout or data array";
        return;
    }

    auto old = mOld.lock(); // Can be nullptr, so not checking

    int ordinal = 1;
    int index = 0;

    // Note: we do NOT evaluate the array items.  Why?  Well, I'm worried about consistency
    auto length = array->size();
    for (size_t dataIndex = 0; dataIndex < length; dataIndex++) {
        auto childContext = buildBaseChildContext(array, dataIndex, index);

        if (mNumbered)
            childContext->putSystemWriteable(COMPONENT_ORDINAL, ordinal);

        auto child = Builder(old).expandSingleComponentFromArray(childContext,
                                                              mItems,
                                                              Properties(),
                                                              layout,
                                                              mChildPath,
                                                              layout->shouldBeFullyInflated(index),
                                                              useDirtyFlag);

        Builder::registerRebuildDependencyIfRequired(layout, childContext, mItems, child != nullptr, {COMPONENT_DATA});

        if (child && child->isValid()) {
            layout->appendChild(child, useDirtyFlag);
            index++;

            if (mNumbered) {
                int numbering = child->getCalculated(kPropertyNumbering).getInteger();
                if (numbering == kNumberingNormal) ordinal++;
                else if (numbering == kNumberingReset) ordinal = 1;
            }
        }
    }

    // After the first build() we register for LiveDataFlush callbacks and connect these to layout rebuilds.
    mCallbackToken = array->addFlushCallback([this](const std::string& key, LiveDataObject& ldo){
        rebuild();
    });
}


void
LayoutRebuilder::rebuild()
{
    auto layout = mLayout.lock();
    auto array = mArray.lock();

    if (!layout || !array) {
        LOG(LogLevel::kError).session(layout) << "Attempting to rebuild a layout without a layout or data array";
        return;
    }

    // The old child array is in the correct order (we don't have reordering, but probably the
    // wrong location.  We'll walk through the new list and the old list, deleting and inserting
    // items as appropriate.

    auto walker = ChildWalker(layout, mHasFirstItem);

    auto old = mOld.lock();  // Can be nullptr, so not checking
    int ordinal = 1;
    int index = 0;

    // Walk the list of new items
    for (int newIndex = 0 ; newIndex < array->size() ; newIndex++) {
        const auto& data = array->at(newIndex);
        auto p = array->newToOld(newIndex);
        auto oldIndex = p.first;
        auto needsRefresh = p.second;

        auto reactiveHandling = CoreDocumentContext::cast(mContext->documentContext())
                                    ->content()
                                    ->reactiveConditionalInflation();

        if (oldIndex == -1 || (reactiveHandling && !walker.advanceUntil(oldIndex))) {  // Insert a new child - this one doesn't exist
            auto childContext = buildBaseChildContext(array, newIndex, index);

            if (mNumbered)
                childContext->putConstant(COMPONENT_ORDINAL, ordinal);

            auto child = Builder(old).expandSingleComponentFromArray(
                childContext,
                mItems,
                Properties(),
                layout,
                mChildPath,
                layout->shouldBeFullyInflated(index),
                true);
            Builder::registerRebuildDependencyIfRequired(layout, childContext, mItems, child != nullptr, {COMPONENT_DATA});
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
                auto isRemove = false;
                // TODO: Can we optimize recalculation by batching these and only changing some of them?
                childContext->systemUpdateAndRecalculate(COMPONENT_INDEX, index, true);

                if (needsRefresh)
                    childContext->systemUpdateAndRecalculate(COMPONENT_DATA, data, true);

                childContext->systemUpdateAndRecalculate(COMPONENT_LENGTH, array->size(), true);
                childContext->systemUpdateAndRecalculate(COMPONENT_DATA_INDEX, newIndex, true);
                childContext->systemUpdateAndRecalculate(COMPONENT_ORDINAL, ordinal, true);

                if (reactiveHandling) {
                    // Should be fully inflated if old currently attached and in ensured range
                    auto fullBuild = (child && child->isAttached()) || layout->shouldBeFullyInflated(index);
                    // No need to register dependency here. Existing children reuse context
                    auto newChild = Builder(layout).expandSingleComponentFromArray(
                        childContext, mItems, Properties(), layout, mChildPath,
                        fullBuild, true, child);
                    if (newChild != child) {
                        if (child) layout->removeChildAt(index, true);
                        if (newChild && newChild->isValid()) {
                            // Add new one with reused context
                            layout->insertChild(newChild, index, true);
                        } else {
                            isRemove = true;
                        }
                    }
                }

                if (!isRemove) {
                    walker.advance();
                    index += 1;
                }

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
    layout->processLayoutChanges(true, false);
    // And allow for full DOM to adjust any changed relative sizes
    layout->getLayoutRoot()->processLayoutChanges(true, false);
}

void
LayoutRebuilder::notifyItemOnScreen(int idx)
{
    auto layout = mLayout.lock();
    auto array = mArray.lock();
    if (!layout || !array)
        return;

    auto child = mLayout.lock()->getCoreChildAt(idx);
    auto dataIndex = child->getContext()->opt(COMPONENT_DATA_INDEX);
    if (dataIndex.isNumber()) {
        array->ensure(dataIndex.getInteger());
    }
}

void
LayoutRebuilder::notifyStartEdgeReached()
{
    if (auto array = mArray.lock()) {
        array->ensure(0);
    }
}

void
LayoutRebuilder::notifyEndEdgeReached()
{
    if (auto array = mArray.lock()) {
        array->ensure(array->size());
    }
}

void
LayoutRebuilder::inflateIfRequired(const CoreComponentPtr& child)
{
    // We only act on children
    auto ctx = child->getContext();

    auto item = ctx->opt("_item");
    if (item.isNull()) return;
    if (child->singleChild()) {
        Builder(mOld.lock()).populateSingleChildLayout(ctx, item, child, child->getPathObject(), true, true);
    } else if (child->multiChild()) {
        Builder(mOld.lock()).populateLayoutComponent(ctx, item, child, child->getPathObject(), true, true);
    }
    ctx->remove("_item");
}

} // namespace apl
