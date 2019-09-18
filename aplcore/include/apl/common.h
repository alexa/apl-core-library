/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef _APL_COMMON_H
#define _APL_COMMON_H

#include <functional>
#include <memory>

namespace apl {

/**
 * Standard type for unique IDs in components
 */
using id_type = unsigned int;

/**
 * System value for tracking time.  Nominally milliseconds since the epoch.
 */
using apl_time_t = unsigned long;

/**
 * Associate a unique ID with a timeout
 */
using timeout_id = unsigned int;

/**
 * Change in time, in milliseconds.
 */
using apl_duration_t = long;

// Common definitions of shared pointer data structures.  We define the XXXPtr variations
// here so they can be conveniently used from any source file.

class Action;
class Command;
class Component;
class Content;
class Context;
class CoreComponent;
class GraphicContent;
class GraphicElement;
class Graphic;
class Package;
class RootContext;
class Session;
class StyleDefinition;
class StyleInstance;
class TextMeasurement;
class Timers;

using ActionPtr = std::shared_ptr<Action>;
using CommandPtr = std::shared_ptr<Command>;
using ComponentPtr = std::shared_ptr<Component>;
using ContentPtr = std::shared_ptr<Content>;
using ContextPtr = std::shared_ptr<Context>;
using CoreComponentPtr = std::shared_ptr<CoreComponent>;
using GraphicContentPtr = std::shared_ptr<GraphicContent>;
using GraphicElementPtr = std::shared_ptr<GraphicElement>;
using GraphicPtr = std::shared_ptr<Graphic>;
using PackagePtr = std::shared_ptr<Package>;
using RootContextPtr = std::shared_ptr<RootContext>;
using SessionPtr = std::shared_ptr<Session>;
using StyleDefinitionPtr = std::shared_ptr<StyleDefinition>;
using StyleInstancePtr = std::shared_ptr<StyleInstance>;
using TextMeasurementPtr = std::shared_ptr<TextMeasurement>;
using TimersPtr = std::shared_ptr<Timers>;


} // namespace apl


#endif //_APL_COMMON_H
