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

#include "apl/scenegraph/layer.h"
#include "apl/scenegraph/accessibility.h"
#include "apl/scenegraph/node.h"

namespace apl {
namespace sg {

Layer::Layer(const std::string& name, Rect bounds, float opacity, Transform2D transform)
    : mName(name),
      mBounds(bounds),
      mTransform(transform),
      mOpacity(std::min(1.0f, std::max(0.0f, opacity)))
{
}

Layer::~Layer() = default;

std::string
Layer::debugFlagString() const
{
    // WARNING: Keep this in sync with the enum Flags definitions
    static const char *PARTS[] = {
        "OPACITY",
        "POSITION",
        "SIZE",
        "TRANSFORM",
        "CHILD_OFFSET",
        "OUTLINE",
        "CONTENT",
        "SHADOW",
        "CHILDREN",
        "CHILD_CLIP",
        "ACCESSIBILITY",
        "INTERACTION",
        nullptr
    };

    std::string result;
    for (int i = 0 ; PARTS[i] ; i++) {
        if ((mFlags & (1u << i)) != 0) {
            if (!result.empty())
                result += ' ';
            result += PARTS[i];
        }
    }
    return result;
}

std::string
Layer::debugInteractionString() const
{
    static const char *INTERACTION_NAMES[] = {
        "disabled",           // 1u << 0
        "checked",            // 1u << 1
        "pressable",          // 1u << 2
        "scrollHorizontal",   // 1u << 3
        "scrollVertical",     // 1u << 4
        nullptr
    };

    std::string result;
    for (int i = 0 ; INTERACTION_NAMES[i] ; i++) {
        if ((mInteraction & (1u << i)) != 0) {
            if (!result.empty())
                result += ' ';
            result += INTERACTION_NAMES[i];
        }
    }
    return result;
}


void
Layer::updateInteraction(unsigned int interaction, bool isSet)
{
    auto old = mInteraction;
    if (isSet)
        mInteraction |= interaction;
    else
        mInteraction &= ~interaction;

    if (old != mInteraction)
        setFlag(kFlagInteractionChanged);
}

void
Layer::removeAllChildren()
{
    if (mChildren.empty())
        return;

    mChildren.clear();
    setFlag(kFlagChildrenChanged);
}

void
Layer::appendChild(const LayerPtr& child)
{
    assert(child);

    mChildren.emplace_back(child);
    setFlag(kFlagChildrenChanged);
}

void
Layer::setContent(const NodePtr& node)
{
    if (node != mContent) {
        mContent = node;
        setFlag(kFlagRedrawContent);
    }
}

bool
Layer::setBounds(const Rect& bounds)
{
    if (bounds == mBounds)
        return false;

    if (bounds.getTopLeft() != mBounds.getTopLeft())
        setFlag(kFlagPositionChanged);

    if (bounds.getSize() != mBounds.getSize())
        setFlag(kFlagSizeChanged);

    mBounds = bounds;
    return true;
}

bool
Layer::setOutline(const PathPtr& outline)
{
    if (outline == mOutline)
        return false;

    mOutline = outline;
    setFlag(kFlagOutlineChanged);
    return true;
}

bool
Layer::setChildClip(const PathPtr& childClip)
{
    if (childClip == mChildClip)
        return false;

    mChildClip = childClip;
    setFlag(kFlagChildClipChanged);
    return true;
}

bool
Layer::setOpacity(float opacity)
{
    opacity = std::min(1.0f, std::max(0.0f, opacity));
    if (opacity == mOpacity)
        return false;

    mOpacity = opacity;
    setFlag(kFlagOpacityChanged);
    return true;
}

bool
Layer::setTransform(Transform2D transform)
{
    if (transform == mTransform)
        return false;

    mTransform = transform;
    setFlag(kFlagTransformChanged);
    return true;
}

bool
Layer::setChildOffset(Point childOffset)
{
    if (childOffset == mChildOffset)
        return false;

    mChildOffset = childOffset;
    setFlag(kFlagChildOffsetChanged);
    return true;
}

bool
Layer::setShadow(const ShadowPtr& shadow)
{
    if (shadow == mShadow)
        return false;

    mShadow = std::move(shadow);
    setFlag(kFlagRedrawShadow);
    return true;
}

bool
Layer::setAccessibility(const AccessibilityPtr& accessibility)
{
    if (accessibility == mAccessibility)
        return false;

    mAccessibility = accessibility;
    setFlag(kFlagAccessibilityChanged);
    return true;
}

std::string
Layer::toDebugString() const
{
    return "Layer";
}

bool
Layer::visible() const
{
    if (mBounds.empty() || mOpacity <= 0.0f)
        return false;

    if (mShadow && mShadow->visible())
        return true;

    for (auto node = mContent; node; node = node->next())
        if (node->visible())
            return true;

    for (const auto& m : mChildren)
        if (m->visible())
            return true;

    return false;
}

rapidjson::Value
Layer::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto out = rapidjson::Value(rapidjson::kObjectType);

    out.AddMember("name", rapidjson::Value(mName.c_str(), allocator), allocator );
    out.AddMember("opacity", mOpacity, allocator);
    out.AddMember("bounds", mBounds.serialize(allocator), allocator);
    out.AddMember("transform", mTransform.serialize(allocator), allocator);
    out.AddMember("childOffset", mChildOffset.serialize(allocator), allocator);

    if (mAccessibility)
        out.AddMember("accessibility", mAccessibility->serialize(allocator), allocator);
    if (mOutline)
        out.AddMember("outline", mOutline->serialize(allocator), allocator);
    if (mChildClip)
        out.AddMember("childClip", mChildClip->serialize(allocator), allocator);
    if (mShadow)
        out.AddMember("shadow", mShadow->serialize(allocator), allocator);

    if (mContent) {
        auto contentArray = rapidjson::Value(rapidjson::kArrayType);
        for (auto node = mContent ; node != nullptr ; node = node->next())
            contentArray.PushBack(node->serialize(allocator), allocator);
        out.AddMember("content", contentArray, allocator);
    }

    if (!mChildren.empty()) {
        auto childrenArray = rapidjson::Value(rapidjson::kArrayType);
        for (const auto& child : mChildren)
            childrenArray.PushBack(child->serialize(allocator), allocator);
        out.AddMember("children", childrenArray, allocator);
    }

    out.AddMember("interaction", mInteraction, allocator);

    return out;
}


} // namespace sg
} // namespace apl
