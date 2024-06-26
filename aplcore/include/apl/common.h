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
#include <map>
#include <memory>
#include <set>

#include "apl/apl_config.h"

namespace apl {

/**
 * Standard type for unique IDs in components and dependents
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

/**
 * Objects are used in a lot of places and are often passed by reference.
 */
class Object;

/**
 * Common object types which are often used in shared pointers.  Each of
 * these objects is also declared as a shared pointer version of the form:
 *
 *     using MyClassPtr = std::shared_ptr<MyClass>
 */
class AccessibilityAction;
class Action;
class AudioPlayer;
class AudioPlayerFactory;
class Command;
class Component;
class Content;
class Context;
class ContextData;
class CoreComponent;
class CoreDocumentContext;
class CoreRootContext;
class DataSource;
class DataSourceProvider;
class Dependant;
class DependantManager;
class DocumentConfig;
class DocumentContext;
class DocumentContextData;
class DocumentManager;
class Easing;
class EmbedRequest;
class ExtensionClient;
class ExtensionCommandDefinition;
class ExtensionComponent;
class ExtensionMediator;
class Graphic;
class GraphicContent;
class GraphicElement;
class GraphicPattern;
class LiveArray;
class LiveMap;
class LiveObject;
class MediaManager;
class MediaObject;
class MediaPlayer;
class MediaPlayerFactory;
class Package;
class PackageManager;
class PackageResolver;
class PendingImportPackage;
class RootConfig;
class RootContext;
class SharedJsonData;
class SemanticPattern;
class SemanticVersion;
class Session;
class Settings;
class SharedContextData;
class StyleDefinition;
class StyleInstance;
class TextMeasurement;
class Timers;
class UIDObject;

using AccessibilityActionPtr = std::shared_ptr<AccessibilityAction>;
using ActionPtr = std::shared_ptr<Action>;
using AudioPlayerFactoryPtr = std::shared_ptr<AudioPlayerFactory>;
using AudioPlayerPtr = std::shared_ptr<AudioPlayer>;
using CommandPtr = std::shared_ptr<Command>;
using ComponentPtr = std::shared_ptr<Component>;
using ConstCommandPtr = std::shared_ptr<const Command>;
using ConstContextPtr = std::shared_ptr<const Context>;
using ContentPtr = std::shared_ptr<Content>;
using ContextPtr = std::shared_ptr<Context>;
using ContextDataPtr = std::shared_ptr<ContextData>;
using CoreComponentPtr = std::shared_ptr<CoreComponent>;
using ConstCoreComponentPtr = std::shared_ptr<const CoreComponent>;
using CoreDocumentContextPtr = std::shared_ptr<CoreDocumentContext>;
using CoreRootContextPtr = std::shared_ptr<CoreRootContext>;
using DataSourceProviderPtr = std::shared_ptr<DataSourceProvider>;
using DependantPtr = std::shared_ptr<Dependant>;
using DocumentConfigPtr = std::shared_ptr<DocumentConfig>;
using DocumentContextDataPtr = std::shared_ptr<DocumentContextData>;
using DocumentContextPtr = std::shared_ptr<DocumentContext>;
using DocumentContextWeakPtr = std::weak_ptr<DocumentContext>;
using DocumentManagerPtr = std::shared_ptr<DocumentManager>;
using EasingPtr = std::shared_ptr<Easing>;
using EmbedRequestPtr = std::shared_ptr<EmbedRequest>;
using ExtensionClientPtr = std::shared_ptr<ExtensionClient>;
using ExtensionCommandDefinitionPtr = std::shared_ptr<ExtensionCommandDefinition>;
using ExtensionComponentPtr = std::shared_ptr<ExtensionComponent>;
using ExtensionMediatorPtr = std::shared_ptr<ExtensionMediator>;
using GraphicContentPtr = std::shared_ptr<GraphicContent>;
using GraphicElementPtr = std::shared_ptr<GraphicElement>;
using GraphicPatternPtr = std::shared_ptr<GraphicPattern>;
using GraphicPtr = std::shared_ptr<Graphic>;
using LiveArrayPtr = std::shared_ptr<LiveArray>;
using LiveMapPtr = std::shared_ptr<LiveMap>;
using LiveObjectPtr = std::shared_ptr<LiveObject>;
using MediaManagerPtr = std::shared_ptr<MediaManager>;
using MediaObjectPtr = std::shared_ptr<MediaObject>;
using MediaPlayerFactoryPtr = std::shared_ptr<MediaPlayerFactory>;
using MediaPlayerPtr = std::shared_ptr<MediaPlayer>;
using PackagePtr = std::shared_ptr<Package>;
using PackageManagerPtr = std::shared_ptr<PackageManager>;
using PackageResolverPtr = std::shared_ptr<PackageResolver>;
using PendingImportPackagePtr = std::shared_ptr<PendingImportPackage>;
using RootConfigPtr = std::shared_ptr<RootConfig>;
using RootContextPtr = std::shared_ptr<RootContext>;
using SemanticPatternPtr = std::shared_ptr<SemanticPattern>;
using SemanticVersionPtr = std::shared_ptr<SemanticVersion>;
using SessionPtr = std::shared_ptr<Session>;
using SettingsPtr = std::shared_ptr<Settings>;
using SharedContextDataPtr = std::shared_ptr<SharedContextData>;
using StyleDefinitionPtr = std::shared_ptr<StyleDefinition>;
using StyleInstancePtr = std::shared_ptr<StyleInstance>;
using TextMeasurementPtr = std::shared_ptr<TextMeasurement>;
using TimersPtr = std::shared_ptr<Timers>;

/**
 * Convenience templates for creating sets of weak and strong pointers
 */
template<class T> using SharedPtrSet = std::set<std::shared_ptr<T>, std::owner_less<std::shared_ptr<T>>>;
template<class T> using WeakPtrSet = std::set<std::weak_ptr<T>, std::owner_less<std::weak_ptr<T>>>;
template<class Key, class T> using WeakPtrMap = std::map<std::weak_ptr<Key>,
                                                T,
                                                std::owner_less<std::weak_ptr<Key>>,
                                                std::allocator<std::pair<const std::weak_ptr<Key>, T>>
                                                >;

} // namespace apl


#endif //_APL_COMMON_H
