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

#ifndef _APL_EDIT_TEXT_COMPONENT_H
#define _APL_EDIT_TEXT_COMPONENT_H

#include <utility>

#include "actionablecomponent.h"

namespace apl {

class EditTextComponent : public ActionableComponent {

public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);

    EditTextComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypeEditText; };

    void assignProperties(const ComponentPropDefSet& propDefSet) override;

    void update(UpdateType type, float value) override;

    void update(UpdateType type, const std::string& value) override;

    Object getValue() const override;

    bool isCharacterValid(const wchar_t wc) const override;

    void parseValidCharactersProperty();

protected:
    const ComponentPropDefSet& propDefSet() const override;
    const EventPropertyMap& eventPropertyMap() const override;
    PointerCaptureStatus processPointerEvent(const PointerEvent& event, apl_time_t timestamp) override;
    void executeOnFocus() override;

private:
    CharacterRangesPtr mCharacterRangesPtr;
};

} // namespace apl

#endif //_APL_EDIT_TEXT_COMPONENT_H
