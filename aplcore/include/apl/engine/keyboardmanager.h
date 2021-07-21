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

#ifndef _APL_KEY_MASTER_H
#define _APL_KEY_MASTER_H

#include <memory>

#include "apl/primitives/keyboard.h"

namespace apl {

class CoreComponent;

class KeyboardManager {
public:

    /**
     * Handle a keyboard update on a component.
     * @param component The component receiving the key press.  If null, ignored.
     * @param keyboard The key press definition.
     * @param document The Content document.
     * @result True, if the key was consumed.
     */
    bool handleKeyboard(KeyHandlerType type, const CoreComponentPtr& component, const Keyboard& keyboard,
                        const RootContextPtr& rootContext);

    /**
     * @param type The Keyboard handler type.
     * @return The Keyboard handler PropertyKey for the Keyboard update.
     */
    static PropertyKey getHandlerPropertyKey(KeyHandlerType type);

    /**
     * @param type The Keyboard handler type.
     * @return The handler identifier for the Keyboard update.
     */
    static std::string getHandlerId(KeyHandlerType type);

private:
    bool executeDocumentKeyHandlers(const RootContextPtr& rootContext,  KeyHandlerType type, const Keyboard& keyboard);

};

} // namespace apl

#endif //_APL_KEY_MASTER_H
