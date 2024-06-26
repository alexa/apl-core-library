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

#ifndef _APL_TEXT_CHUNK_H
#define _APL_TEXT_CHUNK_H

#include "apl/scenegraph/common.h"
#include "apl/primitives/styledtext.h"
#include "apl/utils/userdata.h"

namespace apl {
namespace sg {

class TextChunk : public UserData<TextChunk> {
public:
    static TextChunkPtr create(const StyledText& styledText) {
        return std::make_shared<TextChunk>(styledText);
    }

    static TextChunkPtr create(const std::string& text) {
        return std::make_shared<TextChunk>(StyledText::createRaw(text));
    }

    explicit TextChunk(const StyledText& styledText) : mStyledText(styledText) {}

    const StyledText& styledText() const { return mStyledText; }

    std::size_t hash() const { return std::hash<std::string>{}(mStyledText.asString()); }

private:
    StyledText mStyledText;
};

}
} // namespace apl

#endif // _APL_TEXT_CHUNK_H
