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

#ifndef _APL_COMMON_H
#define _APL_COMMON_H

#include <functional>
#include <memory>
#include <set>

namespace apl {

/**
 * Standard type for unique IDs in components
 */
using id_type = unsigned int;

/**
 * Associate a unique ID with a timeout
 */
using timeout_id = unsigned int;

/**
 * System value for tracking time.  Nominally milliseconds since the epoch.
 * We use double values because certain scripting languages have difficulties with 64 bit integers.
 */
using apl_time_t = double;

/**
 * Change in time, in milliseconds.
 */
using apl_duration_t = double;

// Common definitions of shared pointer data structures.  We define the XXXPtr variations
// here so they can be conveniently used from any source file.

class Action;
class CharacterRanges;
class Command;
class Component;
class Content;
class Context;
class CoreComponent;
class DataSource;
class DataSourceProvider;
class Easing;
class ExtensionClient;
class ExtensionMediator;
class GraphicContent;
class GraphicElement;
class Graphic;
class GraphicPattern;
class LiveArray;
class LiveMap;
class LiveObject;
class MediaManager;
class MediaObject;
class Package;
class RootConfig;
class RootContext;
class Session;
class Settings;
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
using DataSourcePtr = std::shared_ptr<DataSource>;
using DataSourceProviderPtr = std::shared_ptr<DataSourceProvider>;
using EasingPtr = std::shared_ptr<Easing>;
using ExtensionClientPtr = std::shared_ptr<ExtensionClient>;
using ExtensionMediatorPtr = std::shared_ptr<ExtensionMediator>;
using GraphicContentPtr = std::shared_ptr<GraphicContent>;
using GraphicElementPtr = std::shared_ptr<GraphicElement>;
using GraphicPtr = std::shared_ptr<Graphic>;
using GraphicPatternPtr = std::shared_ptr<GraphicPattern>;
using LiveArrayPtr = std::shared_ptr<LiveArray>;
using LiveMapPtr = std::shared_ptr<LiveMap>;
using LiveObjectPtr = std::shared_ptr<LiveObject>;
using MediaManagerPtr = std::shared_ptr<MediaManager>;
using MediaObjectPtr = std::shared_ptr<MediaObject>;
using PackagePtr = std::shared_ptr<Package>;
using RootConfigPtr = std::shared_ptr<RootConfig>;
using RootContextPtr = std::shared_ptr<RootContext>;
using SessionPtr = std::shared_ptr<Session>;
using SettingsPtr = std::shared_ptr<Settings>;
using StyleDefinitionPtr = std::shared_ptr<StyleDefinition>;
using StyleInstancePtr = std::shared_ptr<StyleInstance>;
using TextMeasurementPtr = std::shared_ptr<TextMeasurement>;
using TimersPtr = std::shared_ptr<Timers>;
using CharacterRangesPtr = std::shared_ptr<CharacterRanges>;

// Convenience templates for creating sets of weak and strong pointers
template<class T> using WeakPtrSet = std::set<std::weak_ptr<T>, std::owner_less<std::weak_ptr<T>>>;
template<class T> using SharedPtrSet = std::set<std::shared_ptr<T>, std::owner_less<std::shared_ptr<T>>>;

} // namespace apl


#endif //_APL_COMMON_H
