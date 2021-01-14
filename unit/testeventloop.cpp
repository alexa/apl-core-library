/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include "apl/livedata/livearrayobject.h"
#include "apl/livedata/livemapobject.h"

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
        {"Action",               Counter<Action>::itemsDelta},
        {"Command",              Counter<Command>::itemsDelta},
        {"Component",            Counter<Component>::itemsDelta},
        {"Content",              Counter<Content>::itemsDelta},
        {"Context",              Counter<Context>::itemsDelta},
        {"DataSourceConnection", Counter<DataSourceConnection>::itemsDelta},
        {"Dependant",            Counter<Dependant>::itemsDelta},
        {"ExtensionClient",      Counter<ExtensionClient>::itemsDelta},
        {"Graphic",              Counter<Graphic>::itemsDelta},
        {"GraphicElement",       Counter<GraphicElement>::itemsDelta},
        {"Package",              Counter<Package>::itemsDelta},
        {"RootContextData",      Counter<RootContextData>::itemsDelta},
        {"Sequencer",            Counter<Sequencer>::itemsDelta},
        {"Styles",               Counter<Styles>::itemsDelta},
        {"LayoutRebuilder",      Counter<LayoutRebuilder>::itemsDelta},
        {"LiveMapObject",        Counter<LiveMapObject>::itemsDelta},
        {"LiveArrayObject",      Counter<LiveArrayObject>::itemsDelta},
        {"LiveArray",            Counter<LiveArray>::itemsDelta},
        {"LiveMap",              Counter<LiveMap>::itemsDelta},
    };
    return sMemoryCounters;
}
#endif

} // namespace apl
