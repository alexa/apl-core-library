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

* The **position** and **size** of the layer.  The position is with respect
  to its parent.  The size is a width and height.
* A **transformation** to be applied to the layer before drawing.  This is used
  to slide, rotate, scale, and otherwise shift the layer out of its normal 
  position.
* The **opacity** of the layer [0-1].  This affects everything within the layer,
  including the *content* and *child layers*
* The **content** of the layer.  This is a `Node` tree of drawing elements.  The
  content is drawn before child layers
* Shadow:
  * The **outline** of the layer.  This may be a rectangle or it may be a rounded
    rectangle.  The outline is used for drawing the layer shadow.
  * A `ShadowPathOp` defining the **shadow** properties of the layer.  This may
    be null.
* Child layers:
  * An ordered list of **child layers**.  These are drawn in order; later layers
    overwrite earlier.
  * An optional **child transformation**.  This transformation is applied before
    drawing children.  It is used for scrolling.
  * An optional **child clipping** region.  This is a path drawn inside of the
    layer which clips children (for example, the *Frame* component clips children
    inside the border).

The scene graph contains a set of *changed* layers. A changed layer sets one or
more of the following flags:

Flag | Meaning
-----| -------
OpacityChanged | The opacity of the layer changed
PositionChanged | The position of the layer changed
SizeChanged | The size of the layer changed
TransformChanged | The transformation of the layer changed
ChildTransformChanged | The child layer transformation changed
RedrawContent | The content of the layer needs to be redrawn
RedrawShadow | The shadow path or outline needs to be redrawn
ChildrenChanged | The list of child layers has changed

These flags are supplied to the view host and may be used for optimizing rendering.

## Drawing Nodes

Drawing nodes are used in the *content* section of a Layer to draw on the screen.
Drawing nodes come in the following types:

Node | Children | Description
---- | -------- | -----------
Generic | Yes | No operation
Transform | Yes | Modify the coordinate system
Clip | Yes | Set a clipping path
Opacity | Yes | Apply an opacity multiplier
Draw | No | A single path and a list of path operations
Text | No | A TextLayout and a list of path operations
Image | No | A Filter (set of one or more images with composition), source, and target rectangles
Video | No | A MediaPlayer, target rectangle, and video scaling rule

Drawing nodes may be freely changed each time the scene graph is returned; there
is no guarantee that the same drawing nodes will be kept each time.  However, internally
the TextLayout, Filter, and MediaPlayer objects will be reused.

It's possible that the view host will want to cache certain drawing elements such
as paths, patterns, gradients, etc.  These should be attached to the drawing node
using the bridge pattern.  

### Algorithm used for *Visible*

A drawing node is visible if it renders some content on the screen.

### Algorithm used for *RedrawContent*

A fast and reliable method of calculating the *RedrawContent* flag is crucial to
view host performance.  Note that each layer is associated with a single component
in the APL DOM.  When that component is marked as dirty, one or more of the dirty
changes may affect the content.  The algorithm works as follows:

1. Each dirty property that affects content is mapped to a drawing *node*.
2. Values are set on the drawing nodes.  If the value that is set changes the
   drawing node, a *changed* flag is set on the node.  If the value that is set
   changes the *children* of the node, then a *childrenChanged* flag is set.
3. Each drawing node that has a flag set is added to a linked list of *changed* or
   *childrenChanged* nodes.  We use an intrusive list with a terminal to efficiently
   determine if the drawing node is already on the list.
4. Once all dirty properties have been processed, we walk the tree to determine if
   any of the changes are *visible* and hence require a content redraw. The tree-walk
   algorithm is described below.
5. Finally, the linked list is walked and the *changed* and *childrenChanged* flags 
   are cleared.

The tree-walk algorithm starts at the top of the content tree and performs a recursive,
depth-first search.  It terminates if it finds a *changed* node that is visible.  The
algorithm for each node type:

* **Opacity Node**
  * If there are no children and `!childrenChanged` &rightarrow; false
  * If `opacity==0 && !changed` &rightarrow;  false.
  * If `opacity>0 && !changed && !childrenChanged` ask the
    children if they need a redraw.
  * Otherwise &rightarrow;  true.

* **Text/Draw/Image/Video Node**
  * If the paint has no alpha and hasn't changed &rightarrow;  false.
  * Otherwise, return `changed`

* **Generic/Transform/Clip**
  * If there are no children and `!childrenChanged` &rightarrow;  false
  * If `changed || childrenChanged` &rightarrow;  true
  * Otherwise, ask the children if they need a redraw


## Basic Principles:

1. Each scene graph object will only be modified when the `RootContext` calls
   `getSceneGraph()`
2. Children of a scene graph element are stored in an intrusive linked list
3. 