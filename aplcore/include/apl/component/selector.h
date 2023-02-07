/*
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

#ifndef _APL_SELECTOR_H
#define _APL_SELECTOR_H

#include "apl/common.h"

namespace apl {

/**
 * Represent a selector that identifies and returns a component (or nullptr) from a hierarchy
 * of components
 *
 * This grammar parses component selectors and returns a Selector object.  The grammar
 * is defined as:
 *
 * componentId  ::= element? modifier*
 * element      ::= uid | id | ":source" | ":root"
 * modifier     ::= modifierType "(" arg? ")"
 * modifierType ::= ":parent" | ":child" | ":find": | ":next" | ":previous"
 * arg          ::= number | "id=" id | "type=" type
 * uid          ::= ":" [0-9]+
 * id           ::= [-_a-zA-Z0-9]+
 * number       ::= "0" | "-"? [1-9][0-9]*
 * type         ::= STRING
 *
 * Note that the 'id' syntax is more permissive than what is allowed in the APL specification.
 * This is to support backwards compatibility.
 *
 * Valid examples of this grammar:
 *
 * FOO                       # The first component with id=FOO
 * :1003                     # The component with unique ID ":1003"
 * :source                   # The component that issued the command
 * :root                     # The top of the component hierarchy
 * :source:child(2)          # The third child of the source component
 * FOO:child(-1)             # The last child of the FOO component
 * FOO:parent()              # The parent of FOO
 * FOO:child(id=BAR)         # The first direct child of FOO where id=BAR
 * FOO:find(id=BAR)          # The first descendant of FOO where id=BAR
 * FOO:child(type=Text)      # The first direct child of FOO where type=Text
 * FOO:parent:child(id=BAR)  # The first sibling of FOO where id=BAR
 * FOO:next(id=BAR)          # The first sibling searching forwards of FOO where id=Bar
 * FOO:parent(2)             # The grandparent of FOO
 * FOO:previous(1)           # The sibling immediately before FOO
 */
class Selector {
public:
    /**
     * Parse a 'componentId' string and return the matching component
     * @param string The string to parse
     * @param context The current data-binding context
     * @param source The starting component (may be nullptr)
     * @return The target component (may be nullptr)
     */
    static CoreComponentPtr resolve(const std::string& string,
                                    const ContextPtr& context,
                                    const CoreComponentPtr& source=nullptr);
};

} // namespace apl

#endif
