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

#ifndef _APL_SG_COMMON_H
#define _APL_SG_COMMON_H

#include <memory>

namespace apl {
namespace sg {

class SceneGraphUpdates;

class EditText;
class EditTextBox;
class EditTextConfig;
class EditTextFactory;

class EditTextBox;
class TextChunk;
class TextProperties;
class TextPropertiesCache;
class TextLayout;
class TextLayoutCache;

class Accessibility;
class Filter;
class Layer;
class Node;
class GraphicFragment;
class Paint;
class Path;
class PathOp;
class SceneGraph;
class Shadow;

using AccessibilityPtr = std::shared_ptr<Accessibility>;
using EditTextPtr = std::shared_ptr<EditText>;
using EditTextBoxPtr = std::shared_ptr<EditTextBox>;
using EditTextConfigPtr = std::shared_ptr<EditTextConfig>;
using EditTextFactoryPtr = std::shared_ptr<EditTextFactory>;
using FilterPtr = std::shared_ptr<Filter>;
using GraphicFragmentPtr = std::shared_ptr<GraphicFragment>;
using LayerPtr = std::shared_ptr<Layer>;
using NodePtr = std::shared_ptr<Node>;
using PaintPtr = std::shared_ptr<Paint>;
using PathPtr = std::shared_ptr<Path>;
using PathOpPtr = std::shared_ptr<PathOp>;
using SceneGraphPtr = std::shared_ptr<SceneGraph>;
using ShadowPtr = std::shared_ptr<Shadow>;
using TextChunkPtr = std::shared_ptr<TextChunk>;
using TextLayoutPtr = std::shared_ptr<TextLayout>;
using TextPropertiesPtr = std::shared_ptr<TextProperties>;

} // namespace sg
} // namespace apl

#endif // _APL_SG_COMMON_H
