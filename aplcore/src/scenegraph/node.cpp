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

#include "apl/scenegraph/node.h"

#include "apl/media/mediaplayer.h"
#include "apl/scenegraph/edittext.h"
#include "apl/scenegraph/edittextbox.h"
#include "apl/scenegraph/edittextconfig.h"
#include "apl/scenegraph/filter.h"
#include "apl/scenegraph/path.h"
#include "apl/scenegraph/pathop.h"
#include "apl/scenegraph/shadow.h"
#include "apl/scenegraph/textlayout.h"

#include <list>

namespace apl {
namespace sg {

Node::~Node()
{
    assert(!mNextSibling);

    auto child = mFirstChild;
    while (child) {
        auto nextChild = child->mNextSibling;
        child->mNextSibling.reset();
        child = nextChild;
    }

    mFirstChild.reset();
}

// TODO: Performance here is linear, but this is rarely called
void
Node::appendChild(const NodePtr& child)   // NOLINT(performance-unnecessary-value-param)
{
    assert(child);

    if (mFirstChild) {
        auto ptr = mFirstChild;
        while (ptr->next())
            ptr = ptr->next();
        ptr->mNextSibling = child;
    }
    else
        mFirstChild = child;
}

void
Node::removeAllChildren()
{
    if (!mFirstChild)
        return;

    auto child = mFirstChild;
    while (child) {
        auto nextChild = child->mNextSibling;
        child->mNextSibling.reset();
        child = nextChild;
    }

    mFirstChild.reset();
}

size_t
Node::childCount() const
{
    size_t result = 0;
    auto child = mFirstChild;
    while (child) {
        result += 1;
        child = child->mNextSibling;
    }
    return result;
}

bool
Node::needsRedraw() const
{
    // If there are no children and the children haven't changed, there is nothing to draw.
    // Note: this must be overridden by classes that actually draw something.
    if (!mFirstChild && !isFlagSet(kNodeFlagChildrenChanged))
        return false;

    // If something changed, we need to be drawn
    if (anyFlagSet())
        return true;

    for (auto child = mFirstChild ; child ; child = child->mNextSibling)
        if (child->needsRedraw())
            return true;

    return false;
}

bool
Node::visible() const
{
    for (auto child = mFirstChild ; child ; child = child->mNextSibling)
        if (child->visible())
            return true;

    return false;
}

rapidjson::Value
Node::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = rapidjson::Value(rapidjson::kObjectType);
    if (mFirstChild) {
        auto children = rapidjson::Value(rapidjson::kArrayType);
        for (auto child = mFirstChild ; child ; child = child->mNextSibling)
            children.PushBack(child->serialize(allocator), allocator);
        result.AddMember("children", children, allocator);
    }
    return result;
}

/************************************************************************************************/

std::string
GenericNode::toDebugString() const
{
    return "GenericNode";
}


rapidjson::Value
GenericNode::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Node::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("generic"), allocator);
    return result;
}

/************************************************************************************************/

bool
DrawNode::setPath(PathPtr path)
{
    if (path == mPath)
        return false;

    mPath = path;
    return true;
}

bool
DrawNode::setOp(PathOpPtr op)
{
    if (op == mOp)
        return false;

    mOp = op;
    return true;
}

std::string
DrawNode::toDebugString() const
{
    return "DrawNode";
}

bool
DrawNode::needsRedraw() const
{
    // TODO: Do something smarter with the paint
    return anyFlagSet();
}

bool
DrawNode::visible() const
{
    // Check to ensure that at least one path operation is visible
    for (auto op = mOp ; op ; op = op->nextSibling)
        if (op->visible())
            return true;

    return false;
}

rapidjson::Value
DrawNode::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Node::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("draw"), allocator);
    if (mPath)
        result.AddMember("path", mPath->serialize(allocator), allocator);
    if (mOp)
        result.AddMember("op", mOp->serialize(allocator), allocator);
    return result;
}

/************************************************************************************************/

bool
TextNode::setTextLayout(TextLayoutPtr textLayout)
{
    if (textLayout == mTextLayout)
        return false;

    mTextLayout = textLayout;
    return true;
}

bool
TextNode::setOp(PathOpPtr op)
{
    if (op == mOp)
        return false;

    mOp = op;
    return true;
}

bool
TextNode::setRange(Range range)
{
    if (mRange == range)
        return false;

    mRange = range;
    return true;
}

std::string
TextNode::toDebugString() const
{
    auto result = std::string("TextNode");
    if (mTextLayout) {
        result += " size=" + mTextLayout->getSize().toString() +
                  " range=" + mRange.toDebugString() +
                  " text=" + mTextLayout->toDebugString();
    }
    return result;
}

bool
TextNode::needsRedraw() const
{
    // TODO: Do something smarter with the paint
    return anyFlagSet();
}

bool
TextNode::visible() const
{
    if (mTextLayout->empty())
        return false;

    // Check to ensure that at least one path operation is visible
    for (auto op = mOp ; op ; op = op->nextSibling)
        if (op->visible())
            return true;

    return false;
}

rapidjson::Value
TextNode::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Node::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("text"), allocator);
    if (mOp)
        result.AddMember("op", mOp->serialize(allocator), allocator);
    if (!mRange.empty())
        result.AddMember("range", mRange.serialize(allocator), allocator);
    if (mTextLayout && !mTextLayout->empty())
        result.AddMember("layout", mTextLayout->serialize(allocator), allocator);
    return result;
}

/************************************************************************************************/

bool
TransformNode::setTransform(Transform2D transform)
{
    if (transform == mTransform)
        return false;

    mTransform = transform;
    return true;
}

std::string
TransformNode::toDebugString() const
{
    return std::string("TransformNode transform="+ mTransform.toDebugString());
}

rapidjson::Value
TransformNode::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Node::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("transform"), allocator);
    result.AddMember("transform", mTransform.serialize(allocator), allocator);
    return result;
}

/************************************************************************************************/

bool
ClipNode::setPath(PathPtr path)
{
    if (path == mPath)
        return false;

    mPath = path;
    return true;
}

std::string
ClipNode::toDebugString() const
{
    return "ClipNode";
}

rapidjson::Value
ClipNode::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Node::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("clip"), allocator);
    if (mPath)
        result.AddMember("path", mPath->serialize(allocator), allocator);
    return result;
}

/************************************************************************************************/

bool
OpacityNode::setOpacity(float opacity)
{
    opacity = std::min(1.0f, std::max(0.0f, opacity));
    if (opacity == mOpacity)
        return false;

    mOpacity = opacity;
    return true;
}

std::string
OpacityNode::toDebugString() const
{
    return "OpacityNode opacity=" + std::to_string(mOpacity);
}

bool
OpacityNode::needsRedraw() const
{
    // If there are no children and the children haven't changed, there is nothing to draw.
    // Note: this must be overridden by classes that actually draw something.
    if (!mFirstChild && !isFlagSet(kNodeFlagChildrenChanged))
        return false;

    if (mOpacity == 0.0f && !isFlagSet(kNodeFlagModified))
        return false;

    // Ask the children if they need to be drawn
    if (mOpacity > 0.0f && !anyFlagSet()) {
        for (auto child = mFirstChild ; child ; child = child->next())
            if (child->needsRedraw())
                return true;

        return false;
    }

    return true;
}

bool
OpacityNode::visible() const
{
    if (mOpacity == 0.0f)
        return false;

    return Node::visible();
}

rapidjson::Value
OpacityNode::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Node::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("opacity"), allocator);
    result.AddMember("opacity", mOpacity, allocator);
    return result;
}

/************************************************************************************************/

bool
ImageNode::setImage(FilterPtr image)
{
    if (image == mImage)
        return false;

    mImage = image;
    return true;
}

bool
ImageNode::setTarget(Rect target)
{
    if (target == mTarget)
        return false;

    mTarget = target;
    return true;
}

bool
ImageNode::setSource(Rect source)
{
    if (source == mSource)
        return false;

    mSource = source;
    return true;
}

std::string
ImageNode::toDebugString() const
{
    return "ImageNode target="+mTarget.toDebugString() + " source=" + mSource.toDebugString();
}

bool
ImageNode::needsRedraw() const
{
    // TODO: Do something smarter with the paint
    return anyFlagSet();
}

bool
ImageNode::visible() const
{
    return mImage != nullptr;
}

rapidjson::Value
ImageNode::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Node::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("image"), allocator);
    result.AddMember("target", mTarget.serialize(allocator), allocator);
    result.AddMember("source", mSource.serialize(allocator), allocator);
    if (mImage)
        result.AddMember("image", mImage->serialize(allocator), allocator);
    return result;
}

/************************************************************************************************/

bool
VideoNode::setMediaPlayer(MediaPlayerPtr player)
{
    if (player == mPlayer)
        return false;

    mPlayer = player;
    return true;
}

bool
VideoNode::setTarget(Rect target)
{
    if (target == mTarget)
        return false;

    mTarget = target;
    return true;
}

bool
VideoNode::setScale(VideoScale scale)
{
    if (scale == mScale)
        return false;

    mScale = scale;
    return true;
}

std::string
VideoNode::toDebugString() const
{
    auto result = "VideoNode target="+mTarget.toDebugString();
    if (mPlayer)
        result += " PLAYER";
    return result;
}

bool
VideoNode::needsRedraw() const
{
    // TODO: Do something smarter with the paint
    return anyFlagSet();
}

bool
VideoNode::visible() const
{
    return mPlayer != nullptr;
}

rapidjson::Value
VideoNode::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Node::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("video"), allocator);
    result.AddMember("target", mTarget.serialize(allocator), allocator);
    result.AddMember("scale", rapidjson::Value(sVideoScaleMap.at(mScale).c_str(), allocator), allocator);
    if (mPlayer)
        result.AddMember("player", mPlayer->serialize(allocator), allocator);
    return result;
}

/************************************************************************************************/

bool
ShadowNode::setShadow(ShadowPtr shadow)
{
    if (shadow == mShadow)
        return false;

    mShadow = shadow;
    return true;
}

std::string
ShadowNode::toDebugString() const
{
    return "ShadowNode";
}

rapidjson::Value
ShadowNode::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Node::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("shadow"), allocator);
    result.AddMember("shadow", mShadow->serialize(allocator), allocator);
    return result;
}

/************************************************************************************************/

bool
EditTextNode::setEditText(EditTextPtr editText)
{
    if (editText == mEditText)
        return false;

    mEditText = editText;
    return true;
}

bool
EditTextNode::setEditTextBox(EditTextBoxPtr editTextBox)
{
    if (editTextBox == mEditTextBox)
        return false;

    mEditTextBox = editTextBox;
    return true;
}

bool
EditTextNode::setEditTextConfig(EditTextConfigPtr editTextConfig)
{
    if (editTextConfig == mEditTextConfig)
        return false;

    mEditTextConfig = editTextConfig;
    return true;
}

bool
EditTextNode::setText(const std::string& text)
{
    if (text == mText)
        return false;

    mText = text;
    return true;
}

std::string
EditTextNode::toDebugString() const
{
    auto result = std::string("EditTextNode");
    if (mEditText) {
        result += " text=" + mText + " color=" + mEditTextConfig->textColor().asString();
    }
    return result;
}

bool
EditTextNode::needsRedraw() const
{
    // TODO: Do something smarter with the paint
    return anyFlagSet();
}

bool
EditTextNode::visible() const
{
    return true;
}

rapidjson::Value
EditTextNode::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = Node::serialize(allocator);
    result.AddMember("type", rapidjson::StringRef("edit"), allocator);
    if (mEditTextBox) {
        auto box = rapidjson::Value(rapidjson::kObjectType);
        box.AddMember("size", mEditTextBox->getSize().serialize(allocator), allocator);
        box.AddMember("baseline", mEditTextBox->getBaseline(), allocator);
        result.AddMember("box", box, allocator);
    }

    if (mEditTextConfig)
        result.AddMember("config", mEditTextConfig->serialize(allocator), allocator);
    result.AddMember("text", rapidjson::Value(mText.c_str(), allocator), allocator);
    return result;
}

} // namespace sg
} // namespace apl
