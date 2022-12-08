# SceneGraph Notes

## Usage

Call `RootContext::getSceneGraph()` to retrieve the current scene graph.
This operation will either build a new scene graph or update the existing
one.  The scene graph data structures must not be used by a different
thread while this call is running (lock it atomically), nor should you
call into other `RootContext`, `MediaManager`, or `MediaPlayer` methods.

The returned scene graph contains a layer tree and maps for created
and updated layers & nodes. 

Layers & Nodes are tracked using `std::shared_ptr` and inherit from 
the `UserData` template.  This allows view host data to be associated with
a Layer/Node and have its lifetime tracked.  The normal approach is to create
a *bridge* object on the view host side and store that object in the user data.
For example:

```
class LayerBridge {
public:
  static void release(void *ptr) {
    auto *bridge = static_cast<LayerBridge *>(ptr);
    delete bridge;
  }
  
  static LayerBridge* get(const apl::sg::LayerPtr& layer) {
    // Make sure the release function is installed
    apl::sg::Layer::setUserDataReleaseCallback(LayerBridge::release);
  
    auto *bridge = layer->getUserData<LayerBridge>();
    if (bridge)
      return bridge;
      
    bridge = new LayerBridge(layer);
    layer->setUserData(bridge);
    return bridge;
  }
  
  LayerBridge(const apl::sg::LayerPtr& layer);
  ~LayerBridge();
};
```

Then calling `LayerBridge::get(layer)` will return either the existing
layer bridge or a newly allocated one.  The `release` method ensures that
the layer bridge is deleted when the layer goes out of scope.

## Layers

Layers are intended to be used for caching the results of drawing operations on
GPU and accelerating animation effects.  Each layer contains the following:

* A **name** used purely for debugging.
* The **position** and **size** of the layer.  The position is with respect
  to its parent.  The size is a width and height. This information is stored in
  rectangular **bounds** of the layer.
* A **transformation** to be applied to the layer before drawing.  This is used
  to slide, rotate, scale, and otherwise shift the layer out of its normal 
  position.
* The **opacity** of the layer [0-1].  This affects everything within the layer,
  including the *content* and *child layers*.
* The **content** of the layer.  This is a `Node` tree of drawing elements.  The
  content is drawn before child layers.
* Shadow:
  * The **outline** of the layer.  This may be a rectangle or it may be a rounded
    rectangle.  The outline is used for drawing the layer shadow. If not specified,
    the **bounds** of the layer are used.
  * A `ShadowPtr` defining the **shadow** properties of the layer.  This may
    be null.
* Child layers:
  * An ordered list of **child layers**.  These are drawn in order; later layers
    overwrite earlier.
  * An optional **child transformation**.  This transformation is applied before
    drawing children.  It is used for scrolling.
  * An optional **child clipping** region.  This is a path drawn inside of the
    layer which clips children (for example, the *Frame* component clips children
    inside the border).
* An optional **accessibility** pointer for the layer.  This contains:
  * The accessibility **label** for the screen reader.
  * An accessibility **role**.
  * An array of **actions** which can be executed on this layer.
  * An accessibility **action callback** to be invoked if one of those actions
    is executed.

### Layer Changed Flags
The scene graph contains a set of *changed* layers. A changed layer sets one or
more of the following flags:

Flag | Meaning
-----| -------
`kFlagOpacityChanged` | The opacity of the layer changed
`kFlagPositionChanged` | The position of the layer changed
`kFlagSizeChanged` | The size of the layer changed
`kFlagTransformChanged` | The transformation of the layer changed
`kFlagChildOffsetChanged` | The child offset changed
`kFlagOutlineChanged` | The outline of this layer has been modified
`kFlagRedrawContent` | The content of the layer needs to be redrawn
`kFlagRedrawShadow` | The shadow path or outline needs to be redrawn
`kFlagChildrenChanged` | The list of child layers has changed
`kFlagChildClipChanged` | The clipping path for the children has been modified
`kFlagAccessibilityChanged` | The accessibility object has been modified
`kFlagInteractionChanged` | The interactivity of the layer has changed (see interaction flags)

These flags are supplied to the view host and may be used for optimizing rendering.

### Layer Interaction Flags

Each layer also has a set of **interaction** flags:

Flag | Meaning
---- | -------
`kInteractionDisabled` | The layer has been disabled
`kInteractionChecked` | The layer has been checked
`kInteractionPressable` | Supports onPress or onTap
`kInteractionScrollHorizontal` | Can be scrolled horizontally (including normal pagers)
`kInteractionScrollVertical` | Can be scrolled vertically.  May include a pager.

Interaction flags are used by the accessibility system in the view host.

## Drawing Nodes

Drawing nodes are used in the *content* section of a Layer to draw on the screen.

The basic drawing node has the following properties:
* A **type** used for dynamic casting.
* A pointer to the **first child** of this node.
* A pointer to the **next sibling** of this node.
* A pointer to the **next modified** drawing node.  This is the *dirty* list of nodes
  that have changed since the last scene graph retrieval.
* A set of boolean **flags**.  The two supported flags are:
  * `kNodeFlagChildrenChanged`: The sibling child list has been modified
  * `kNodeFlagModified`: Some property of this node has changed.

Drawing nodes come in the following types:

Node | Children | Description
---- | -------- | -----------
`Node::kTransform` | Yes | Modify the coordinate system
`Node::kClip` | Yes | Set a clipping path
`Node::kOpacity` | Yes | Apply an opacity multiplier
`Node::kDraw` | No | A single path and a list of path operations
`Node::kText` | No | A TextLayout and a list of path operations
`Node::kImage` | No | A Filter (set of one or more images with composition), source, and target rectangles
`Node::kVideo` | No | A MediaPlayer, target rectangle, and video scaling rule
`Node::kShadow` | Yes | Apply a shadow to the contained content
`Node::kEditText` | No | An EditTextBox, reference, and configuration information

Drawing nodes may be freely changed each time the scene graph is returned; there
is no guarantee that the same drawing nodes will be kept each time.  However, internally
the TextLayout, Filter, MediaPlayer, AudioPlayer, and EditText objects will be reused.

It's possible that the view host will want to cache certain drawing elements such
as paths, patterns, gradients, etc.  These should be attached to the drawing node
using the bridge pattern.  

The `Node::visible() → Boolean ` method returns *true* if the node draws some content
on the screen.  This is a recursive search.

## The `SceneGraph` object

The `SceneGraph` object holds the information about the current scene graph.
It contains just two pieces:
1. A pointer to the top *layer* for display
2. A `SceneGraphUpdates` structure which contains:
   1. A set of *layers* which have changed.
   2. A set of *layers* which are new and have been created.

The new layers are tracked to keep them from getting into the *changed* list.
The view host typically updates the view by iterating over the *changed* list
and updating each layer appropriately, then verifying that the top-level
layer hasn't been modified.  A layer with the `kFlagChildrenChanged` bit set
rebuilds its child layers, which means that newly created layers will be added
automatically.

The normal way for a view host to build a layer is to use an `ensure` method
which changes the `LayerPtr` for an attached view-host data structure.  If it
doesn't find an attached view-host data structure, it creates a new one and
attaches it to the layer.


### Algorithm used for *RedrawContent*

A fast and reliable method of calculating the *RedrawContent* flag is crucial to
view host performance.  Note that each layer is associated with a single component
in the APL DOM.  When that component is marked as dirty, one or more of the dirty
changes may affect the content.  The algorithm works as follows:

1. If a component has dirty properties, the `CoreComponent::updateSceneeGraph`
   method is invoked on it.
2. The `updateSceneGraph` method checks all of the properties that are commmon
   to the *layer* held by that component.  Layer-properties (see [Layer Changed Flags]())
   are updated and the appropriate flag is set for the layer.
3. The `updateSceneGraphicInternal → Boolean` virtual method handles the
   component-specific updates which includes all drawing content.  This method
   returns *true* if the layer needs to redraw its content, which sets the
   *RedrawContent* flag.
