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

#ifndef _APL_COLOR_H
#define _APL_COLOR_H

#include <string>
#include <cstdio>
#include <unordered_map>

#include <rapidjson/document.h>

#include "apl/common.h"
#include "apl/utils/streamer.h"
#include "apl/primitives/objecttype.h"

namespace apl {

/**
 * A class to store and manipulate RGBA colors.  The colors are internally stored as
 * unsigned 32 bit integers in the form RRGGBBAA.  They are visually displayed (and
 * parsed) using the "#RRGGBBAA" syntax.
 *
 * Please note that operating systems store colors differently; in particular, Android
 * uses an ARGB format.
 */
class Color {
public:
    enum ColorConstants : uint32_t {
        TRANSPARENT = 0x00000000,
        BLACK = 0x000000ff,
        SILVER = 0xc0c0c0ff,
        GRAY = 0x808080ff,
        GREY = 0x808080ff,
        WHITE = 0xffffffff,
        MAROON = 0x800000ff,
        RED = 0xff0000ff,
        PURPLE = 0x800080ff,
        FUCHSIA = 0xff00ffff,
        GREEN = 0x008000ff,
        LIME = 0x00ff00ff,
        OLIVE = 0x808000ff,
        YELLOW = 0xffff00ff,
        NAVY = 0x000080ff,
        BLUE = 0x0000ffff,
        TEAL = 0x008080ff,
        AQUA = 0x00ffffff,
    };

    /**
     * Default constructor.  Initializes the color to transparent.
     */
    Color() : mColor(0) {}

    /**
     * Build a color from a string (does not apply data-binding)
     * @param session The logging session (used if the color is malformed)
     * @param color The named color or expression.
     */
    Color(const SessionPtr& session, const char *color) : mColor(parse(session, color)) {}

    /**
     * Build a color from a string (does not apply data-binding)
     * @param session The logging session
     * @param color The named color or expression.
     */
    Color(const SessionPtr& session, const std::string& color) : mColor(parse(session, color.c_str())) {}

    /**
     * Default argument-provided color.
     * @param color The color in the form RGBA
     */
    Color(uint32_t color) : mColor(color) {}

    Color(const Color&) = default;
    Color(Color&&) = default;
    Color& operator=(const Color&) = default;

    /**
     * @return The internal uint32_t value of the color
     */
    uint32_t get() const { return mColor; }

    /**
     * @return The red component [0-255]
     */
    int red() const { return (mColor >> 24u) & 0xffu; }

    /**
     * @return The green component [0-255]
     */
    int green() const { return (mColor >> 16u) & 0xffu; }

    /**
     * @return The blue component [0-255]
     */
    int blue() const { return (mColor >> 8u) & 0xffu; }

    /**
     * @return The alpha component [0-255]
     */
    int alpha() const { return (mColor & 0xffu); }

    /**
     * @return True if the color is transparent
     */
    bool transparent() const { return alpha() == 0; }

    friend streamer& operator<<(streamer& os, const Color& color) {
        return os << color.asString();
    }

    /**
     * @return This color in '#RRGGBBAA' format
     */
    std::string asString() const {
        uint32_t a = mColor & 0xffu;
        uint32_t b = (mColor >> 8u) & 0xffu;
        uint32_t g = (mColor >> 16u) & 0xffu;
        uint32_t r = (mColor >> 24u) & 0xffu;
        char hex[10];
        snprintf(hex, sizeof(hex), "#%02x%02x%02x%02x", r, g, b, a);
        return std::string(hex);
    }

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const {
        return rapidjson::Value(asString().c_str(), allocator);
    }

    friend bool operator==( const Color& lhs, const Color& rhs) {
        return lhs.mColor == rhs.mColor;
    }

    friend bool operator!=( const Color& lhs, const Color& rhs) {
        return lhs.mColor != rhs.mColor;
    }

    /**
     * Lookup a color in the default set of color names.
     * @param name The name to retrieve.
     * @return A pair containing true/false if the color was found and the returned value.
     */
    static std::pair<bool, uint32_t> lookup(const std::string& name) {
        auto it = sColorMap.find(name);
        if (it == sColorMap.end())
            return { false, 0 };
        return { true, it->second };
    }

    class ObjectType final : public SimpleObjectType<Color> {
    public:
        bool truthy(const Object::DataHolder&) const override { return true; }

        std::string asString(const Object::DataHolder& dataHolder) const override {
            return Color(dataHolder.value).asString();
        }

        Color asColor(const Object::DataHolder& dataHolder, const SessionPtr&) const override {
            return Color(dataHolder.value);
        }

        uint32_t getColor(const Object::DataHolder& dataHolder) const override {
            return static_cast<uint32_t>(dataHolder.value);
        }

        std::size_t hash(const Object::DataHolder& dataHolder) const override {
            return std::hash<double>{}(dataHolder.value);
        }

        rapidjson::Value serialize(
            const Object::DataHolder& dataHolder,
            rapidjson::Document::AllocatorType& allocator) const override
        {
            return rapidjson::Value(asString(dataHolder).c_str(), allocator);
        }

        std::string toDebugString(const Object::DataHolder& dataHolder) const override {
            return asString(dataHolder);
        }

        bool equals(const Object::DataHolder& lhs, const Object::DataHolder& rhs) const override {
            return lhs.value == rhs.value;
        }
    };

private:
    /**
     * Convert from a color string representation to a color
     * @param color The color string
     * @return An color as a 32 bit unsigned integer in the form RGBA
     */
    static uint32_t parse(const SessionPtr& session, const char *color);

    uint32_t mColor;
    static const std::unordered_map<std::string, uint32_t> sColorMap;
};

}  // namespace apl

#endif // _APL_COLOR_H
