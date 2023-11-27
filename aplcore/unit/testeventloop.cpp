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

#include "testeventloop.h"

#include "apl/graphic/graphicelementcontainer.h"
#include "apl/graphic/graphicelementgroup.h"
#include "apl/graphic/graphicelementpath.h"
#include "apl/graphic/graphicelementtext.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/livedata/livemapobject.h"
#include "apl/touch/gesture.h"
#include "apl/time/executionresourceholder.h"

namespace apl {

std::ostream& operator<<(std::ostream& os, const Point& point) {
    streamer s;
    s << point;
    os << s.str();
    return os;
}

std::ostream& operator<<(std::ostream& os, const Transform2D& t) {
    apl::streamer s;
    s << t;
    return os << s.str();
}

std::ostream& operator<<(std::ostream& os, const Radii& r) {
    apl::streamer s;
    s << r;
    return os << s.str();
}

std::ostream& operator<<(std::ostream& os, const Object& object) {
    apl::streamer s;
    s << object;
    return os << s.str();
}

const char *TestEventCommand::COMMAND = "Custom";
const char *TestEventCommand::EVENT = "CustomEvent";

#ifdef DEBUG_MEMORY_USE
const std::map<std::string, std::function<CounterPair()>>&
getMemoryCounterMap() {
    static std::map<std::string, std::function<CounterPair()>> sMemoryCounters = {
        {"Action",                  Counter<Action>::itemsDelta},
        {"Command",                 Counter<Command>::itemsDelta},
        {"Component",               Counter<Component>::itemsDelta},
        {"Content",                 Counter<Content>::itemsDelta},
        {"Context",                 Counter<Context>::itemsDelta},
        {"DataSourceConnection",    Counter<DataSourceConnection>::itemsDelta},
        {"Dependant",               Counter<Dependant>::itemsDelta},
        {"ExecutionResourceHolder", Counter<ExecutionResourceHolder>::itemsDelta},
        {"DocumentContext",         Counter<DocumentContext>::itemsDelta},
        {"ExtensionClient",         Counter<ExtensionClient>::itemsDelta},
        {"Gesture",                 Counter<Gesture>::itemsDelta},
        {"Graphic",                 Counter<Graphic>::itemsDelta},
        {"GraphicElement",          Counter<GraphicElement>::itemsDelta},
        {"GraphicElementContainer", Counter<GraphicElementContainer>::itemsDelta},
        {"GraphicElementGroup",     Counter<GraphicElementGroup>::itemsDelta},
        {"GraphicElementPath",      Counter<GraphicElementPath>::itemsDelta},
        {"GraphicElementText",      Counter<GraphicElementText>::itemsDelta},
        {"GraphicPattern",          Counter<GraphicPattern>::itemsDelta},
        {"MediaObject",             Counter<MediaObject>::itemsDelta},
#ifdef SCENEGRAPH
        {"Node",                    Counter<sg::Node>::itemsDelta},
#endif // SCENEGRAPH
        {"Package",                 Counter<Package>::itemsDelta},
        {"SharedContextData",       Counter<SharedContextData>::itemsDelta},
        {"ContextData",             Counter<ContextData>::itemsDelta},
        {"Sequencer",               Counter<Sequencer>::itemsDelta},
        {"Styles",                  Counter<Styles>::itemsDelta},
        {"LayoutRebuilder",         Counter<LayoutRebuilder>::itemsDelta},
        {"LiveMapObject",           Counter<LiveMapObject>::itemsDelta},
        {"LiveArrayObject",         Counter<LiveArrayObject>::itemsDelta},
        {"LiveArray",               Counter<LiveArray>::itemsDelta},
        {"LiveMap",                 Counter<LiveMap>::itemsDelta},
    };
    return sMemoryCounters;
}
#endif

/**
 * Replicates (as closely as possible) process used in viewhosts for text measurement, but with
 * every symbol being 10x10 square. Doesn't account for line breaks.
 */
LayoutSize
SimpleTextMeasurement::measure(Component *component,
                                float width,
                                MeasureMode widthMode,
                                float height,
                                MeasureMode heightMode)
{
    auto len = component->getCalculated(kPropertyText).asString().size(); // Number of characters
    float singleLineWidth = static_cast<float>(len) * 10;
    float resultingWidth = 0;
    auto workingWidth = 10 * std::floor(width / 10); // width clamped to symbol size

    // There are 3 MeasureModes:
    //  1. Exactly - text should fit into provided metric, requested metric is reported as resulting
    //               measurement.
    //  2. AtMost - text should fit into provided metric, but can take less, actual text size
    //              reported as resulting measurement.
    //  3. Undefined - text is unbound in provided axis. Effectively AtMost with INFINITE or
    //                 UNDEFINED limit (NaN in case of Yoga), actual text size reported as
    //                 resulting measurement.
    switch (widthMode) {
        case MeasureMode::Exactly:
            resultingWidth = width;
            break;
        case MeasureMode::AtMost:
            resultingWidth = std::min(workingWidth, singleLineWidth);
            break;
        case MeasureMode::Undefined:
            resultingWidth = singleLineWidth;
            // Otherwise effectively undefined/NaN and height will be calculated in the wrong way.
            workingWidth = resultingWidth;
            break;
    }

    auto charactersPerLine = std::min(resultingWidth, workingWidth) / 10;

    // Can't layout as line can't hold single character.
    if (charactersPerLine <= 0) return {resultingWidth, 0};

    float workingHeight = 10 * std::ceil(static_cast<float>(len) / charactersPerLine);
    float resultingHeight = 0;
    switch (heightMode) {
        case MeasureMode::Exactly:
            resultingHeight = height;
            break;
        case MeasureMode::AtMost:
            resultingHeight = std::min(height, workingHeight);
            break;
        case MeasureMode::Undefined:
            resultingHeight = workingHeight;
            break;
    }

    return {resultingWidth, resultingHeight};
}

float
SimpleTextMeasurement::baseline(Component *component,
                                 float width,
                                 float height) {
    return 8;
}

LayoutSize
SpyTextMeasure::measure(Component *component,
                               float width,
                               MeasureMode widthMode,
                               float height,
                               MeasureMode heightMode)
{
    visualHashes.emplace_back(component->getCalculated(kPropertyVisualHash));
    return LayoutSize({ 90.0, 30.0 });
}

float
SpyTextMeasure::baseline(Component *component,
                                float width,
                                float height) {
    return 0;
}

} // namespace apl
