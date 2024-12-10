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

#ifndef _APL_LAYER_H
#define _APL_LAYER_H

#include <cstdint>

#include "apl/common.h"
#include "apl/primitives/transform2d.h"
#include "apl/scenegraph/path.h"
#include "apl/scenegraph/pathop.h"
#include "apl/scenegraph/shadow.h"
#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/userdata.h"

namespace apl {
namespace sg {

/**
 * Wraps a component to specify where to draw (bounds), component opacity, and
 * an arbitrary 2D transformation (modifies the bounds).  This is a consolidation of
 * a Transform, Clip rectangle, and Opacity node.
 */
class Layer : public Counter<Layer>,
              public UserData<Layer>,
              public NonCopyable,
              public std::enable_shared_from_this<Layer>
{
    friend class SceneGraph;

public:
    // WARNING: If you change these, update the layers.cpp file to fix the debugFlagString
    enum Flags {
        kFlagOpacityChanged = 1u << 0,
        kFlagPositionChanged = 1u << 1,
        kFlagSizeChanged = 1u << 2,
        kFlagTransformChanged = 1u << 3,
        kFlagChildOffsetChanged = 1u << 4,
        kFlagOutlineChanged = 1u << 5,
        kFlagRedrawContent = 1u << 6,
        kFlagRedrawShadow = 1u << 7,
        kFlagChildrenChanged = 1u << 8,
        kFlagChildClipChanged = 1u << 9,
        kFlagAccessibilityChanged = 1u << 10,
        kFlagInteractionChanged = 1u << 11,
    };

    using FlagType = std::uint16_t;

    // WARNING: If you changed these, update the layers.cpp file to fix the constant array
    enum Interaction {
        kInteractionNone = 0,
        kInteractionDisabled = 1u << 0,           // Disabled state set
        kInteractionChecked = 1u << 1,            // Checked state set
        kInteractionPressable = 1u << 2,          // Supports onPress or onTap
        kInteractionScrollHorizontal = 1u << 3,   // Can be scrolled horizontally.  Includes pagers
        kInteractionScrollVertical = 1u << 4,     // Can be scrolled vertically
    };

    using InteractionType = std::uint8_t;

    // WARNING: If you changed these, update the layers.cpp file to fix the constant array
    enum Characteristics {
        kCharacteristicDoNotClipChildren = 1u << 0,    // Do not clip children to the layer outline or clip path
        kCharacteristicRenderOnly = 1u << 1,           // This layer is only for rendering (no text input or touch events)
        kCharacteristicHasMedia = 1u << 2,             // This layer contains images or videos.
        kCharacteristicHasText = 1u << 3               // This layer contains a text node.
    };

    using CharacteristicsType = std::uint8_t;

    Layer(const std::string& name, Rect bounds, float opacity, Transform2D transform);
    ~Layer() override;

    const std::string& getName() const { return mName; }

    void setFlag(FlagType flag) { mFlags |= flag; }
    bool isFlagSet(FlagType flag) const { return (mFlags & flag) != 0; }
    bool anyFlagSet() const { return mFlags != 0; }
    void clearFlags() { mFlags = 0; }
    FlagType getAndClearFlags() { auto result = mFlags; mFlags = 0; return result; }
    std::string debugFlagString() const;

    void setInteraction(InteractionType interaction) { mInteraction |= interaction; }
    void updateInteraction(InteractionType interaction, bool isSet);
    InteractionType getInteraction() const { return mInteraction; }
    std::string debugInteractionString() const;

    void setCharacteristic(CharacteristicsType characteristic) {
        mCharacteristics |= characteristic; }
    CharacteristicsType getCharacteristic() const { return mCharacteristics; }
    bool isCharacteristicSet(CharacteristicsType characteristic) const { return (mCharacteristics & characteristic) != 0; }
    std::string debugCharacteristicString() const;

    void removeAllChildren();
    void appendChild(const LayerPtr& layer);
    void appendChildren(const std::vector<LayerPtr>& children);
    const std::vector<LayerPtr>& children() const { return mChildren; }

    void setContent(const NodePtr& node);
    const NodePtr& content() const { return mContent; }

    /**
     * The content node does not always have the same origin as the layer.
     */
    void setContentOffset(const Point& offset);
    Point getContentOffset() const { return mContentOffset; }

    /**
     * The bounds of the layer are its outline and position relative to the containing layer.
     */
    bool setBounds(const Rect& bounds);
    Rect getBounds() const { return mBounds; }

    /**
     * The outline is relative to the layer-coordinates (where the top-left is 0,0).
     * If the outline is not set, use the bounds for clipping
     */
    bool setOutline(const PathPtr& outline);
    PathPtr getOutline() const { return mOutline; }

    /**
     * The optional child clipping path restricts where children can be drawn
     */
    bool setChildClip(const PathPtr& childClip);
    PathPtr getChildClip() const { return mChildClip; }

    /**
     * The opacity of the layer applies to both contents and children
     */
    bool setOpacity(float opacity);
    float getOpacity() const { return mOpacity; }

    /**
     * The transformation is relative to the center of the bounds.
     */
    bool setTransform(Transform2D transform);
    Transform2D getTransform() const { return mTransform; }

    /**
     * The child transformation is relative to the center of the bounds
     */
    bool setChildOffset(Point childOffset);
    Point getChildOffset() const { return mChildOffset; }

    /**
     * If set, the shadow is drawn using the outline or bounds of the layer
     */
    bool setShadow(const ShadowPtr& shadow);
    ShadowPtr getShadow() const { return mShadow; }

    /**
     * Accessibility labels come directly from the component
     */
    bool setAccessibility(const AccessibilityPtr& accessibility);
    AccessibilityPtr getAccessibility() const { return mAccessibility; }

    bool visible() const;
    std::string toDebugString() const;

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

private:
    std::string mName;
    std::vector<LayerPtr> mChildren;
    NodePtr mContent;
    Point mContentOffset;  // Local transform applied before drawing content

    Rect mBounds;     // The bounds of the layer.  The position is the top-left corner in the parent

    Transform2D mTransform;  // Global transform applied to this entire Layer
    Point mChildOffset;  // Local transform applied before drawing children (good for scrolling)

    PathPtr mOutline;   // Optional outline if you don't want to use mBounds
    PathPtr mChildClip; // Optional internal child clipping path
    ShadowPtr mShadow;  // Shadow (drawn using mOutline or mBounds)

    AccessibilityPtr mAccessibility;   // Accessibility information

    float mOpacity = 1.0f;   // Common layer opacity
    FlagType mFlags = 0;
    InteractionType mInteraction = 0;
    CharacteristicsType mCharacteristics = 0;
};


} // namespace sg
} // namespace apl

#endif // _APL_LAYER_H
