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


#ifndef _APL_EASING_H
#define _APL_EASING_H

#include "apl/primitives/objectdata.h"

namespace apl {

class CoreEasing;

class Easing : public ObjectData {
public:
    struct Bounds { float start; float end; float minimum; float maximum; };

    /**
     * Generate an easing curve from a string.  If the string is invalid, we return a linear easing curve
     * @param easing The character string.
     * @return A reference to the easing curve.
     */
    static EasingPtr parse(const SessionPtr& session, const std::string& easing);

    /**
     * @return The default linear easing curve
     */
    static EasingPtr linear();

    /**
     * Check to see if an easing curve has been defined.
     * @param easing The character string of the easing curve.
     * @return True if the easing curve exists.
     */
    static bool has(const char *easing);

    /**
     * Evaluate the easing curve at a given time between 0 and 1.
     * @param time
     * @return The value
     */
    virtual float calc(float time) = 0;

    /**
     * @return A bounding box of the easing curve.
     */
    virtual Bounds bounds() = 0;

    // Standard ObjectData methods
    bool operator==(const ObjectData& other) const override;
    bool empty() const override { return false; }
    bool truthy() const override { return true; }
    Object call(const ObjectArray& args) const override;

    virtual bool operator==(const Easing& other) const = 0;
    virtual bool operator==(const CoreEasing& other) const = 0;
    virtual ~Easing() noexcept;
};

} // namespace apl


#endif //APL_EASING_H
