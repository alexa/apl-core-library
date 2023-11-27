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

#include "../testeventloop.h"
#include "test_sg.h"
#include "apl/scenegraph/builder.h"
#include "apl/media/mediaobject.h"
#include "../media/testmediaplayerfactory.h"

using namespace apl;

class SGVideoTest : public DocumentWrapper {
public:
    SGVideoTest() : DocumentWrapper() {
        config->enableExperimentalFeature(RootConfig::kExperimentalFeatureManageMediaRequests);
        auto mediaPlayerFactory = std::make_shared<TestMediaPlayerFactory>();
        config->mediaPlayerFactory(mediaPlayerFactory);
    }
};

static const char * BASIC_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "width": 100,
          "height": 100,
          "type": "Video",
          "source": "http://fake.url"
        }
      }
    }
)apl";

TEST_F(SGVideoTest, LayerCharacteristicTest) {
    metrics.size(300, 300);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 100})
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .content(IsVideoNode()
                             .url("http://fake.url")
                             .scale(apl::kVideoScaleBestFit)
                             .target(Rect{0,0,100,100}))
                ));
}