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

using namespace apl;

class ImageTest : public DocumentWrapper {
public:
    class TestMediaObject : public MediaObject {
    public:
        TestMediaObject(std::string url, MediaObject::State state)
            : mURL(std::move(url)),
              mState(state)
        {}

        std::string url() const override { return mURL; }
        State state() const override { return mState; }
        EventMediaType type() const override { return apl::kEventMediaTypeImage; }
        Size size() const override { return {10,10}; }
        int errorCode() const override { return mErrorCode; }
        std::string errorDescription() const override { return mErrorDescription; }
        const HeaderArray& headers() const override { return mHeaders; }
        GraphicContentPtr graphic() override { return mGraphic; }

        CallbackID addCallback(MediaObjectCallback callback) override { return -1; }

        void removeCallback(CallbackID callbackId) override {}

        std::string mURL;
        State mState;
        std::map<CallbackID, MediaObjectCallback> mCallbacks;
        int mErrorCode = 0;
        std::string mErrorDescription;
        HeaderArray mHeaders;
        GraphicContentPtr mGraphic;
    };

    class TestMediaManager : public MediaManager {
    public:
        MediaObjectPtr request(const std::string& url, EventMediaType type, const HeaderArray& headers) override {
            if (mUrls.count(url)) {
                return std::make_shared<TestMediaObject>(url, MediaObject::State::kReady);
            } else {
                return std::make_shared<TestMediaObject>(url, MediaObject::State::kError);
            }

        }

        void add(const std::string& url) {
            mUrls.emplace(url);
        }

    private:
        std::set<std::string> mUrls;
    };
};

static const char *IMAGE_SETVALUE = R"apl({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items": [
        {
          "type": "Image",
          "id": "img1",
          "source": "https://images.amazon.com/image/foo.png",
          "align": "center",
          "borderRadius": 5,
          "overlayGradient": {
            "colorRange": [ "blue","red" ]
          },
          "scale": "fill"
        },
        {
          "type": "Image",
          "id": "img2",
          "source": "https://images.amazon.com/image/bar.png",
          "overlayGradient": {
            "colorRange": [ "green", "gray" ]
          }
        }
      ]
    }
  }
}
)apl";

// Test for image component align/borderRadius/overlayGradient/scale properties for dynamic
TEST_F(ImageTest, ImageProperties) {
    loadDocument(IMAGE_SETVALUE);
    ASSERT_TRUE(component);
    auto mediaSourceUrl = Object("https://images.amazon.com/image/foo.png");
    auto img1 = CoreComponent::cast(context->findComponentById("img1"));
    ASSERT_TRUE(img1);
    ASSERT_EQ(kComponentTypeImage, img1->getType());
    ASSERT_TRUE(CheckProperties(img1, {
                                          {kPropertyAlign, kImageAlignCenter },
                                          {kPropertyScale, kImageScaleFill },
                                          {kPropertyBorderRadius, Dimension(5) },
                                          {kPropertySource, mediaSourceUrl },
                                      }));

    auto grad1 = img1->getCalculated(kPropertyOverlayGradient);
    ASSERT_TRUE(grad1.is<Gradient>());
    ASSERT_EQ(Object(Color(Color::BLUE)), grad1.get<Gradient>().getProperty(kGradientPropertyColorRange).at(0));
    ASSERT_EQ(Object(Color(Color::RED)), grad1.get<Gradient>().getProperty(kGradientPropertyColorRange).at(1));

    // Set aline property of img
    img1->setProperty(kPropertyAlign, "left");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(img1, kPropertyAlign, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, img1));
    root->clearDirty();

    ASSERT_EQ(kImageAlignLeft, img1->getCalculated(kPropertyAlign).getInteger());

    // Set borderRadius property of img
    img1->setProperty(kPropertyBorderRadius, 10);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(img1, kPropertyBorderRadius, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, img1));
    root->clearDirty();

    ASSERT_EQ(Object(Dimension(10)), img1->getCalculated(kPropertyBorderRadius));

    // Set scale property of img
    img1->setProperty(kPropertyScale, "best-fill");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(img1, kPropertyScale, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, img1));
    root->clearDirty();

    ASSERT_EQ(kImageScaleBestFill, img1->getCalculated(kPropertyScale).getInteger());

    // Set overlayGradient property of img
    auto img2 = CoreComponent::cast(context->findComponentById("img2"));
    auto grad2 = img2->getCalculated(kPropertyOverlayGradient);

    img1->setProperty(kPropertyOverlayGradient, Object(grad2));

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(img1, kPropertyOverlayGradient, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, img1));
    root->clearDirty();

    grad1 = img1->getCalculated(kPropertyOverlayGradient);
    ASSERT_TRUE(grad1.is<Gradient>());
    ASSERT_EQ(Object(Color(Color::GREEN)), grad1.get<Gradient>().getProperty(kGradientPropertyColorRange).at(0));
    ASSERT_EQ(Object(Color(Color::GRAY)), grad1.get<Gradient>().getProperty(kGradientPropertyColorRange).at(1));
}

static const char *IMAGE_DATA_URL = R"apl(
{
  "type": "APL",
  "version": "2024.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "wrap": "wrap",
      "width": "100%",
      "height": "100%",
      "data": [
        { "source": "data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyulvON", "test": 0 },
        { "source": "data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyulvO=", "test": 1 },
        { "source": "data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyulv==", "test": 2 },
        { "source": "data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyul===", "test": 3 },
        { "source": "data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAA_AMAAwAAAC8IyPqcvt3-CcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyulvON", "test": 4 },
        { "source": "data:video/mp4;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyulvON", "test": 5 },
        { "source": "data:,A%20brief%20note", "test": 6 },
        { "source": "data:image/png;,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyuvUUlvON", "test": 7 },
        { "source": "data:image/png;charset=iso-8859-7;potatoes=yes;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCiLc7C0qwyGHhSWpjQu5yq+CYsapyuv", "test": 8 },
        { "source": "data:image/png;base64,R0lGODdhMAAwAPAAAAAPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyulvON", "test": 9 }
      ],
      "items": {
        "type": "Image",
        "width": 100,
        "height": 100,
        "source": "${data.source}",
        "onLoad": {
          "type": "SendEvent",
          "sequencer": "SEND_EVENTER",
          "arguments": [ "SUCCESS", "${data.test}" ]
        },
        "onFail": {
          "type": "SendEvent",
          "sequencer": "SEND_EVENTER",
          "arguments": [ "FAIL", "${data.test}" ]
        }
      }
    }
  }
})apl";

TEST_F(ImageTest, DataUrlValidation) {
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureManageMediaRequests);
    auto mm = std::make_shared<TestMediaManager>();
    mm->add("data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyulvON");
    mm->add("data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyulvO=");
    mm->add("data:image/png;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCcDkiLc7C0qwyGHhSWpjQu5yq+CYsapyulv==");
    mm->add("data:image/png;charset=iso-8859-7;potatoes=yes;base64,R0lGODdhMAAwAPAAAAAAAP///ywAAAAAMAAwAAAC8IyPqcvt3wCiLc7C0qwyGHhSWpjQu5yq+CYsapyuv");

    config->mediaManager(mm);

    loadDocument(IMAGE_DATA_URL);
    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckSendEvent(root, "SUCCESS", 0));
    ASSERT_TRUE(CheckSendEvent(root, "SUCCESS", 1));
    ASSERT_TRUE(CheckSendEvent(root, "SUCCESS", 2));
    ASSERT_TRUE(CheckSendEvent(root, "FAIL", 3));
    ASSERT_TRUE(CheckSendEvent(root, "FAIL", 4));
    ASSERT_TRUE(CheckSendEvent(root, "FAIL", 5));
    ASSERT_TRUE(CheckSendEvent(root, "FAIL", 6));
    ASSERT_TRUE(CheckSendEvent(root, "FAIL", 7));
    ASSERT_TRUE(CheckSendEvent(root, "SUCCESS", 8));
    ASSERT_TRUE(CheckSendEvent(root, "FAIL", 9));

    session->checkAndClear();
}
