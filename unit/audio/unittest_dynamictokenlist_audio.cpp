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

#include "audiotest.h"

/**
 * The purpose of this include statement is to verify that apl/dataprovider.h includes
 * required file(s) that a consumer will need in order to use the datasource provider
 * functionality of APL.
 */
#include "apl/dynamicdata.h"

using namespace apl;

static const char* SOURCE_TYPE = "dynamicTokenList";
static const char* LIST_ID = "listId";
static const char* CORRELATION_TOKEN = "correlationToken";
static const char* PAGE_TOKEN = "pageToken";

class DynamicTokenListAudioTest : public AudioTest {
protected:
    ::testing::AssertionResult CheckFetchRequest(const std::string& listId,
                                                 const std::string& correlationToken,
                                                 const std::string& pageToken) {
        bool fetchCalled = root->hasEvent();
        auto event = root->popEvent();
        fetchCalled &= (event.getType() == kEventTypeDataSourceFetchRequest);

        if (!fetchCalled)
            return ::testing::AssertionFailure() << "Fetch was not called.";

        auto incomingType = event.getValue(kEventPropertyName).getString();
        if (SOURCE_TYPE != incomingType)
            return ::testing::AssertionFailure()
                   << "DataSource type is wrong. Expected: " << SOURCE_TYPE
                   << ", actual: " << incomingType;

        auto request = event.getValue(kEventPropertyValue);

        auto incomingListId = request.opt(LIST_ID, "");
        if (incomingListId != listId)
            return ::testing::AssertionFailure()
                   << "listId is wrong. Expected: " << listId << ", actual: " << incomingListId;

        auto incomingCorrelationToken = request.opt(CORRELATION_TOKEN, "");
        if (incomingCorrelationToken != correlationToken)
            return ::testing::AssertionFailure()
                   << "correlationToken is wrong. Expected: " << correlationToken
                   << ", actual: " << incomingCorrelationToken;

        auto incomingPageToken = request.opt(PAGE_TOKEN, "");
        if (incomingPageToken != pageToken)
            return ::testing::AssertionFailure() << "pageToken is wrong. Expected: " << pageToken
                                                 << ", actual: " << incomingPageToken;

        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    CheckErrors(std::vector<std::string> reasons) {
        auto errors = ds->getPendingErrors().getArray();

        if (errors.size() != reasons.size()) {
            return ::testing::AssertionFailure()
                << "Number of errors is wrong. Expected: " << reasons.size()
                << ", actual: " << errors.size();
        }

        for (int i = 0; i<errors.size(); i++) {
            auto actual = errors.at(i).get("reason").asString();
            auto expected = reasons.at(i);
            if (actual != expected) {
                return ::testing::AssertionFailure()
                    << "error " << i << " reason is wrong. Expected: " << expected
                    << ", actual: " << actual;
            }
        }

        return ::testing::AssertionSuccess();
    }

    static std::string createLazyLoad(int correlationToken, const std::string& pageToken,
                                      const std::string& nextPageToken, const std::string& items) {
        std::string ctString =
            correlationToken < 0
                ? ""
                : ("\"correlationToken\": \"" + std::to_string(correlationToken) + "\",");
        std::string nptString =
            nextPageToken == "" ? "" : ("\"nextPageToken\": \"" + nextPageToken + "\",");
        std::string ptString = "\"pageToken\": \"" + pageToken + "\",";
        return "{"
               "  \"presentationToken\": \"presentationToken\","
               "  \"listId\": \"vQdpOESlok\"," +
               ctString + ptString + nptString + "  \"items\": [" + items + "]"
               "}";
    }

    DynamicTokenListAudioTest() : AudioTest() {
        auto cnf = DynamicListConfiguration(SOURCE_TYPE)
                       .setFetchTimeout(100);
        ds = std::make_shared<DynamicTokenListDataSourceProvider>(cnf);
        config->dataSourceProvider("dynamicTokenList", ds);
    }

    void TearDown() override {
        // Check for unprocessed errors.
        ASSERT_TRUE(ds->getPendingErrors().empty());

        // Clean any pending timeouts. Tests will check them explicitly.
        if (root) {
            loop->advanceToEnd();
            while (root->hasEvent()) {
                root->popEvent();
            }
        }

        DocumentWrapper::TearDown();
    }

    std::shared_ptr<DynamicTokenListDataSourceProvider> ds;
};


static const char * BIT_BY_A_BIT_DEEP = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "parameters": ["dynamicSource"],
    "items": [
      {
        "onMount": [
          {
            "type": "Sequential",
            "commands": [
              {"componentId": "dynamicSequence", "minimumDwellTime": "200", "type": "SpeakItem"},
              {"delay": 500, "type": "Idle"},
              {"type": "ScrollToIndex", "componentId": "dynamicSequence", "index": 0, "align": "center"}
            ]
          }
        ],
        "type": "Container",
        "width": "100%",
        "height": "100%",
        "id": "root",
        "direction": "row",
        "items": [
          {
            "type": "Container",
            "grow": 1,
            "item": [
              {
                "type": "Pager",
                "id": "viewPager",
                "navigation": "none",
                "width": "100%",
                "grow": 1,
                "item": [
                  {
                    "type": "Sequence",
                    "id": "dynamicSequence",
                    "speech": "https://example.com/test.mp3",
                    "navigation": "none",
                    "scrollDirection": "vertical",
                    "numbered": true,
                    "data": "${dynamicSource}",
                    "item": [
                      {
                        "type": "Container",
                        "id": "container${data}",
                        "height": 150,
                        "width": "100%",
                        "data": "${data}",
                        "items": [
                          {
                            "type": "Container",
                            "paddingTop": "50dp",
                            "paddingBottom": "50dp",
                            "item": [{"type": "Text", "text": "${data}"}]
                          }
                        ]
                      }
                    ]
                  }
                ]
              }
            ]
          }
        ]
      }
    ]
  }
})";

static const char* BIT_BY_A_BIT_DATA = R"({
  "dynamicSource": {
    "listId": "vQdpOESlok",
    "pageToken": "currentPageToken",
    "backwardPageToken": "backwardsPageToken1",
    "type": "testList",
    "forwardPageToken": "forwardPageToken1",
    "items": [100]
  }
})";

TEST_F(DynamicTokenListAudioTest, DeepProgressive) {
    factory->addFakeContent({
        {"https://example.com/test.mp3", 4950, 50, -1, {}}, // 50 ms initial delay, 5 second total
    });

    // Set different source, just to avoid config overrides
    auto source = std::make_shared<DynamicTokenListDataSourceProvider>();
    metrics.size(750, 750);
    config->dataSourceProvider("testList", source);

    loadDocument(BIT_BY_A_BIT_DEEP, BIT_BY_A_BIT_DATA);
    auto sequence = std::static_pointer_cast<CoreComponent>(root->findComponentById("dynamicSequence"));
    ASSERT_TRUE(sequence);
    ASSERT_EQ(1, sequence->getChildCount());

    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "101", "forwardPageToken1"));
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "102", "backwardsPageToken1"));

    ASSERT_TRUE(CheckPlayer("https://example.com/test.mp3", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(50);
    ASSERT_TRUE(CheckPlayer("https://example.com/test.mp3", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("https://example.com/test.mp3", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    auto checker = [sequence](int sIdx, int eIdx) {
        int shift = 0;
        for (int i = sIdx; i <= eIdx; i++) {
            auto c = sequence->getCoreChildAt(i);
            if (!c->getCalculated(kPropertyLaidOut).getBoolean()) return false;
            if (Object(Rect(0,shift*150,750,150)) !=c->getCalculated(kPropertyBounds)) return false;
            auto cc = c->getCoreChildAt(0);
            if (!cc->getCalculated(kPropertyLaidOut).getBoolean()) return false;
            if (Object(Rect(0,0,750,110)) != cc->getCalculated(kPropertyBounds)) return false;
            auto ccc = cc->getCoreChildAt(0);
            if (!ccc->getCalculated(kPropertyLaidOut).getBoolean()) return false;
            if (Object(Rect(0,50,750,10)) !=ccc->getCalculated(kPropertyBounds)) return false;
            shift++;
        }
        return true;
    };

    advanceTime(600);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(101, "forwardPageToken1", "forwardPageToken2", "101, 102, 103")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "103", "forwardPageToken2"));
    advanceTime(50);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(102, "backwardsPageToken1", "backwardsPageToken2", "97, 98, 99")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "104", "backwardsPageToken2"));

    ASSERT_EQ(7, sequence->getChildCount());
    ASSERT_EQ(Point(0, 450), sequence->scrollPosition());
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {0, 6}, true));
    ASSERT_TRUE(checker(0, 6));

    advanceTime(600);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(103, "forwardPageToken2", "forwardPageToken3", "104, 105, 106")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "105", "forwardPageToken3"));
    advanceTime(50);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(104, "backwardsPageToken2", "backwardsPageToken3", "94, 95, 96")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "106", "backwardsPageToken3"));

    ASSERT_EQ(13, sequence->getChildCount());
    ASSERT_EQ(Point(0, 900), sequence->scrollPosition());
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {0, 12}, true));
    ASSERT_TRUE(checker(0, 12));

    advanceTime(600);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(105, "forwardPageToken3", "forwardPageToken4", "107, 108, 109")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "107", "forwardPageToken4"));
    advanceTime(50);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(106, "backwardsPageToken3", "backwardsPageToken4", "91, 92, 93")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "108", "backwardsPageToken4"));

    ASSERT_EQ(19, sequence->getChildCount());
    ASSERT_EQ(Point(0, 900), sequence->scrollPosition());

    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {0, 2}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {3, 18}, true));
    ASSERT_TRUE(checker(3, 18));

    advanceTime(600);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(107, "forwardPageToken4", "forwardPageToken5", "110, 111, 112")));
    advanceTime(50);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "109", "forwardPageToken5"));
    advanceTime(50);
    ASSERT_TRUE(source->processUpdate(createLazyLoad(108, "backwardsPageToken4", "backwardsPageToken5", "88, 89, 90")));
    advanceTime(26);
    ASSERT_TRUE(CheckFetchRequest("vQdpOESlok", "110", "backwardsPageToken5"));

    ASSERT_EQ(25, sequence->getChildCount());
    ASSERT_EQ(Point(0, 900), sequence->scrollPosition());
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {0, 5}, false));
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {6, 22}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {23, 24}, false));
    ASSERT_TRUE(checker(6, 22));

    // The speak item has not finished yet.  Calculate how much longer it will take and move forward that far
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(5000 - root->currentTime());
    ASSERT_TRUE(CheckPlayer("https://example.com/test.mp3", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("https://example.com/test.mp3", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    ASSERT_EQ(Point(0, 900), sequence->scrollPosition());

    // The Idle command after speech is 500 ms
    advanceTime(500);

    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {0, 22}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {23, 24}, false));

    ASSERT_EQ(Point(0, 1800), sequence->scrollPosition());
    ASSERT_TRUE(checker(0, 22));

    // The scroll to index takes some time as well
    advanceTime(1000);

    ASSERT_EQ(Point(0, 0), sequence->scrollPosition()); // Current 0 index effectively
    ASSERT_TRUE(source->processUpdate(createLazyLoad(109, "forwardPageToken5", "", "113, 114, 115")));
    ASSERT_TRUE(source->processUpdate(createLazyLoad(110, "backwardsPageToken5", "", "85, 86, 87")));

    advanceTime(16);
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {0, 25}, true));
    ASSERT_TRUE(CheckChildrenLaidOut(sequence, {26, 28}, false));
    ASSERT_TRUE(checker(0, 25));
}
