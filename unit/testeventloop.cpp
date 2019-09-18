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

namespace apl {

std::ostream& operator<<(std::ostream& os, const Point& point)
{
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

const char * TestEventCommand::COMMAND = "Custom";
const char * TestEventCommand::EVENT = "CustomEvent";

} // namespace apl
