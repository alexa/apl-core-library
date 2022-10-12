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

#include "apl/apl_config.h"
#include "apl/component/actionablecomponent.h"
#ifdef SCENEGRAPH
#include "apl/scenegraph/textproperties.h"
#include "apl/scenegraph/edittext.h"
#include "apl/utils/principal_ptr.h"
#endif // SCENEGRAPH

namespace apl {

class EditTextComponent : public ActionableComponent {

public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);

    EditTextComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypeEditText; };

    void assignProperties(const ComponentPropDefSet& propDefSet) override;
    void preLayoutProcessing(bool useDirtyFlag) override;

    void update(UpdateType type, float value) override;

    void update(UpdateType type, const std::string& value) override;

    Object getValue() const override;

    bool isCharacterValid(const wchar_t wc) const override;

protected:
    const ComponentPropDefSet& propDefSet() const override;
    const EventPropertyMap& eventPropertyMap() const override;
    PointerCaptureStatus processPointerEvent(const PointerEvent& event, apl_time_t timestamp) override;
    void executeOnBlur() override;
    void executeOnFocus() override;

#ifdef SCENEGRAPH
    // Common scene graph handling
    sg::LayerPtr constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph) override;
    bool updateSceneGraphInternal(sg::SceneGraphUpdates& sceneGraph) override;

private:
    YGSize measureEditText(MeasureRequest&& request);
    float baselineText(float width, float height);

    bool ensureEditTextBox();
    bool ensureEditConfig();
    bool ensureEditTextProperties();
    bool ensureHintLayout();

private:
    MeasureRequest mLastMeasureRequest;

    // This is created once with the scene graph and used to communicate with the view host
    principal_ptr<sg::EditText, &sg::EditText::release> mEditText;

    // These configure the edit control.  They are nulled when an internal value changes and
    // re-created during the scene graph update (or measure pass, for edit text box)
    sg::EditTextBoxPtr mEditTextBox;
    sg::TextPropertiesPtr mEditTextProperties;
    sg::EditTextConfigPtr mEditTextConfig;

    // These configure the hint display.  They are nulled when an internal values changes and
    // re-created during the scene graph update
    sg::TextLayoutPtr mHintLayout;
    sg::TextChunkPtr mHintText;
    sg::TextPropertiesPtr mHintTextProperties;
#endif // SCENEGRAPH
};

} // namespace apl

#endif //_APL_EDIT_TEXT_COMPONENT_H
