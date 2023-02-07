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

#ifndef _APL_SG_NODE_SUBCLASS
#define _APL_SG_NODE_SUBCLASS

#include <memory>
#include <vector>

#include "apl/common.h"
#include "apl/scenegraph/common.h"
#include "apl/component/componentproperties.h"
#include "apl/component/textmeasurement.h"
#include "apl/primitives/range.h"
#include "apl/primitives/rect.h"
#include "apl/primitives/transform2d.h"
#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/userdata.h"

namespace apl {
namespace sg {

/**
 * A node forms the basis of drawing the scene graph.
 *
 * Each node has a type which is the explicit subclass.  Instead of RTTI, we add explicit cast
 * operations inside each of the node subclasses.
 *
 * Nodes are stored in a linked-list tree structure.  Each node has a "nextSibling" and a "firstChild"
 * node which may be nullptr.  These are shared pointers.
 *
 * Nodes may only be modified by the core engine within the RootContext::getSceneGraph() method.
 * The view host calls that method to extract the current node tree.  The view host must treat the
 * node tree as immutable; it may not modify the structure of the node tree, but it is allowed
 * to store and retrieve data from individual nodes.
 */
class Node : public Counter<Node>,
             public UserData<Node>,
             public NonCopyable,
             public std::enable_shared_from_this<Node> {
    friend class SceneGraphUpdates;
    friend class Layer;

public:
    enum Type {
        kTransform,
        kClip,
        kOpacity,
        kDraw,
        kText,
        kImage,
        kVideo,
        kShadow,
        kEditText,
    };

    ~Node() override;

    /**
     * @return The type of this node.
     */
    Type type() const { return mType; }

    /**
     * Set the child of this node.
     * @param child The child
     */
    bool setChild(const NodePtr& child);

    /**
     * Set the sibling of this node
     * @param sibling The new sibling
     */
    NodePtr setNext(const NodePtr& sibling);

    /**
     * Add a sibling to the chain of siblings pointed to by this node.
     * The sibling is added to the end of the chain.
     * @param head The head of the sibling chain.  May be null.
     * @param sibling The sibling to add to the end of the chain
     * @return The head of the chain
     */
    static NodePtr appendSiblingToNode(const NodePtr& head, const NodePtr& sibling);

    /**
     * Remove all children from this node
     */
    void removeAllChildren();

    /**
     * @return The first child of this node.  May be null.
     */
    const NodePtr& child() const { return mFirstChild; }

    /**
     * @return The next sibling of this node.  May be null.
     */
    const NodePtr& next() const { return mNextSibling; }

    /**
     * @return A human-readable debugging string
     */
    virtual std::string toDebugString() const = 0;

    /**
     * @return True if this node has no children
     */
    bool empty() const { return !mFirstChild; }

    /**
     * @return The number of children
     */
    size_t childCount() const;

    /**
     * @return True if this node draws something on the screen
     */
    virtual bool visible() const;

    /**
     * Calculate the bounding box of this node including its children and siblings
     * @return The bounding box of this node, children, and siblings
     */
    Rect boundingBox(const Transform2D& transform) const;

    /**
     * Calculate the bounding box of a node and all siblings.  This is a convenience method that
     * works on nullptr nodes.
     *
     * @param node The node to start with
     * @return A rectangle that just encloses all of this node, siblings, and children
     */
    static Rect calculateBoundingBox(const NodePtr& node, const Transform2D& transform = Transform2D()) {
        if (!node)
            return {};
        return node->boundingBox(transform);
    }

    /**
     * Serialize this node
     * @param allocator RapidJSON memory allocator
     * @return The serialized value
     */
    virtual rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

protected:
    explicit Node(Type type) : mType(type) {}
    virtual Rect localBoundingBox(const Transform2D& transform) const;

    const Type mType;

    NodePtr mFirstChild;
    NodePtr mNextSibling;
    NodePtr mNextModified;
};

#define NODE_SUBCLASS(TYPE_NAME, TYPE_ENUM)    \
  public:                                                   \
    TYPE_NAME() : Node(TYPE_ENUM) {}                 \
    static TYPE_NAME *cast(Node *node) {                    \
        assert(node != nullptr && node->type() == TYPE_ENUM); \
        return reinterpret_cast<TYPE_NAME*>(node);          \
    }                                                       \
    static const TYPE_NAME *cast(const Node *node) {                    \
        assert(node != nullptr && node->type() == TYPE_ENUM); \
        return reinterpret_cast<const TYPE_NAME*>(node);          \
    }                                                       \
    static TYPE_NAME *cast(const NodePtr& node) {           \
        return cast(node.get());                            \
    }                                                       \
    static bool is_type(const Node *node) { return node && node->type() == TYPE_ENUM; } \
    static bool is_type(const NodePtr& node) { return is_type(node.get()); }          \
    std::string toDebugString() const override;\
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override

/**
 * Draw geometry on the screen.  Contains a path to draw and one or more drawing operations.
 * Normally doesn't have children.
 */
class DrawNode : public Node {
    NODE_SUBCLASS(DrawNode, kDraw);

    bool setPath(PathPtr path);
    PathPtr getPath() const { return mPath; }

    bool setOp(PathOpPtr op);
    PathOpPtr getOp() const { return mOp; }

    bool visible() const override;

protected:
    Rect localBoundingBox(const Transform2D& transform) const override;

private:
    PathPtr mPath;
    PathOpPtr mOp;
};

/**
 * Draw text on the screen.  Contains a text layout and one or more drawing operations.
 * The range is optional.  If the range is empty, draw the entire text block.  Otherwise,
 * the range sets the lines to draw.
 */
class TextNode : public Node {
    NODE_SUBCLASS(TextNode, kText);

    bool setTextLayout(TextLayoutPtr textLayout);
    TextLayoutPtr getTextLayout() const { return mTextLayout; }

    bool setOp(PathOpPtr op);
    PathOpPtr getOp() const { return mOp; }

    bool setRange(Range range);
    Range getRange() const { return mRange; }

    bool visible() const override;

protected:
    Rect localBoundingBox(const Transform2D& transform) const override;

private:
    TextLayoutPtr mTextLayout;
    PathOpPtr mOp;
    Range mRange;
};

/**
 * Apply a 2D transformation to its children.
 */
class TransformNode : public Node {
    NODE_SUBCLASS(TransformNode, kTransform);

    bool setTransform(Transform2D transform);
    const Transform2D& getTransform() const { return mTransform; }

protected:
    Rect localBoundingBox(const Transform2D& transform) const override;

private:
    Transform2D mTransform;
};

/**
 * Clip child drawing to fall within the path.
 */
class ClipNode : public Node {
    NODE_SUBCLASS(ClipNode, kClip);

    bool setPath(PathPtr path);
    PathPtr getPath() const { return mPath; }

protected:
    Rect localBoundingBox(const Transform2D& transform) const override;

private:
    PathPtr mPath;
};

/**
 * Opacity multiplier for all child nodes
 */
class OpacityNode : public Node {
    NODE_SUBCLASS(OpacityNode, kOpacity);

    bool setOpacity(float opacity);
    float getOpacity() const { return mOpacity; }

    bool visible() const override;

private:
    float mOpacity = 1.0;
};

/**
 * Render an image. Contains the image itself (a FilterPtr), a rectangle specifying the subset
 * of the image to copy from, and a rectangle showing where to copy the image to on the screen.
 */
class ImageNode : public Node {
    NODE_SUBCLASS(ImageNode, kImage);

    bool setImage(FilterPtr image);
    FilterPtr getImage() const { return mImage; }

    bool setTarget(Rect target);
    Rect getTarget() const { return mTarget; }

    bool setSource(Rect source);
    Rect getSource() const { return mSource; }

    bool visible() const override;

protected:
    Rect localBoundingBox(const Transform2D& transform) const override;

private:
    FilterPtr mImage;
    Rect mTarget;
    Rect mSource;
};

/**
 * Render a video clip.
 */
class VideoNode : public Node {
    NODE_SUBCLASS(VideoNode, kVideo);

    bool setMediaPlayer(MediaPlayerPtr player);
    MediaPlayerPtr getMediaPlayer() const { return mPlayer; }

    bool setTarget(Rect target);
    Rect getTarget() const { return mTarget; }

    bool setScale(VideoScale scale);
    VideoScale getScale() const { return mScale; }

    bool visible() const override;

protected:
    Rect localBoundingBox(const Transform2D& transform) const override;

private:
    MediaPlayerPtr mPlayer;
    Rect mTarget;
    VideoScale mScale = kVideoScaleBestFit;
};

/**
 * Apply a shadow to the children of this node
 */
class ShadowNode : public Node {
    NODE_SUBCLASS(ShadowNode, kShadow);

    bool setShadow(ShadowPtr shadow);
    ShadowPtr getShadow() const { return mShadow; }

protected:
    Rect localBoundingBox(const Transform2D& transform) const override;

private:
    ShadowPtr mShadow;
};

/**
 * Draw an edit text on the screen.  Contains an edit text and one or more drawing operations.
 */
class EditTextNode : public Node {
    NODE_SUBCLASS(EditTextNode, kEditText);

    bool setEditText(EditTextPtr editText);
    EditTextPtr getEditText() const { return mEditText; }

    bool setEditTextBox(EditTextBoxPtr editTextBox);
    EditTextBoxPtr getEditTextBox() const { return mEditTextBox; }

    bool setEditTextConfig(EditTextConfigPtr editTextConfig);
    EditTextConfigPtr getEditTextConfig() const { return mEditTextConfig; }

    bool setText(const std::string& text);
    std::string getText() const { return mText; }

    bool visible() const override;

private:
    EditTextPtr mEditText;
    EditTextBoxPtr mEditTextBox;
    EditTextConfigPtr mEditTextConfig;
    std::string mText;
};


}; // namespace sg
} // namespace apl

#endif // _APL_SG_NODE_SUBCLASS
