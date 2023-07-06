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

using namespace apl;

namespace apl {
bool
operator==(const SpeechMark& lhs, const SpeechMark& rhs) {
    return lhs.type == rhs.type && lhs.start == rhs.start && lhs.end == rhs.end &&
           lhs.time == rhs.time && lhs.value == rhs.value;
}
}

class SpeechMarkTest : public AudioTest {};

static const char *SIMPLE = R"apl(
{"time":0,"type":"sentence","start":0,"end":23,"value":"Mary had a little lamb."}
)apl";

TEST_F(SpeechMarkTest, Simple)
{
    auto result = parsePollySpeechMarks(SIMPLE, strlen(SIMPLE));
    ASSERT_EQ(1, result.size());
    auto mark = result.at(0);
    ASSERT_EQ(kSpeechMarkSentence, mark.type);
    ASSERT_EQ(0, mark.start);
    ASSERT_EQ(23, mark.end);
    ASSERT_EQ(0, mark.time);
    ASSERT_STREQ("Mary had a little lamb.", mark.value.c_str());
}

static const char * POLLY_EXAMPLE_1 = R"apl(
{"time":0,"type":"sentence","start":0,"end":23,"value":"Mary had a little lamb."}
{"time":6,"type":"word","start":0,"end":4,"value":"Mary"}
{"time":6,"type":"viseme","value":"p"}
{"time":73,"type":"viseme","value":"E"}
{"time":180,"type":"viseme","value":"r"}
{"time":292,"type":"viseme","value":"i"}
{"time":373,"type":"word","start":5,"end":8,"value":"had"}
{"time":373,"type":"viseme","value":"k"}
{"time":460,"type":"viseme","value":"a"}
{"time":521,"type":"viseme","value":"t"}
{"time":604,"type":"word","start":9,"end":10,"value":"a"}
{"time":604,"type":"viseme","value":"@"}
{"time":643,"type":"word","start":11,"end":17,"value":"little"}
{"time":643,"type":"viseme","value":"t"}
{"time":739,"type":"viseme","value":"i"}
{"time":769,"type":"viseme","value":"t"}
{"time":799,"type":"viseme","value":"t"}
{"time":882,"type":"word","start":18,"end":22,"value":"lamb"}
{"time":882,"type":"viseme","value":"t"}
{"time":964,"type":"viseme","value":"a"}
{"time":1082,"type":"viseme","value":"p"}
)apl";

static const std::vector<SpeechMark> POLLY_EXAMPLE_1_EXPECTED = {
    {kSpeechMarkSentence, 0, 23, 0, "Mary had a little lamb." },
    {kSpeechMarkWord, 0, 4, 6, "Mary" },
    {kSpeechMarkViseme, 0, 0, 6, "p" },
    {kSpeechMarkViseme, 0, 0, 73, "E" },
    {kSpeechMarkViseme, 0, 0, 180, "r" },
    {kSpeechMarkViseme, 0, 0, 292, "i" },
    {kSpeechMarkWord, 5, 8, 373, "had" },
    {kSpeechMarkViseme, 0, 0, 373, "k" },
    {kSpeechMarkViseme, 0, 0, 460, "a" },
    {kSpeechMarkViseme, 0, 0, 521, "t" },
    {kSpeechMarkWord, 9, 10, 604, "a" },
    {kSpeechMarkViseme, 0, 0, 604, "@" },
    {kSpeechMarkWord, 11, 17, 643, "little" },
    {kSpeechMarkViseme, 0, 0, 643, "t" },
    {kSpeechMarkViseme, 0, 0, 739, "i" },
    {kSpeechMarkViseme, 0, 0, 769, "t" },
    {kSpeechMarkViseme, 0, 0, 799, "t" },
    {kSpeechMarkWord, 18, 22, 882, "lamb" },
    {kSpeechMarkViseme, 0, 0, 882, "t" },
    {kSpeechMarkViseme, 0, 0, 964, "a" },
    {kSpeechMarkViseme, 0, 0, 1082, "p" },

};

// This test comes from https://docs.aws.amazon.com/polly/latest/dg/speechmarkexamples.html
TEST_F(SpeechMarkTest, PollyExample1)
{
    auto result = parsePollySpeechMarks(POLLY_EXAMPLE_1, strlen(POLLY_EXAMPLE_1));
    ASSERT_EQ(result, POLLY_EXAMPLE_1_EXPECTED);
}


static const char *POLLY_EXAMPLE_2 = R"apl(
{"time":0,"type":"sentence","start":31,"end":95,"value":"Mary had <break time=\"300ms\"/>a little <mark name=\"animal\"/>lamb"}
{"time":6,"type":"word","start":31,"end":35,"value":"Mary"}
{"time":325,"type":"word","start":36,"end":39,"value":"had"}
{"time":897,"type":"word","start":40,"end":61,"value":"<break time=\"300ms\"/>"}
{"time":1291,"type":"word","start":61,"end":62,"value":"a"}
{"time":1373,"type":"word","start":63,"end":69,"value":"little"}
{"time":1635,"type":"ssml","start":70,"end":91,"value":"animal"}
{"time":1635,"type":"word","start":91,"end":95,"value":"lamb"}
)apl";

static const std::vector<SpeechMark> POLLY_EXAMPLE_2_EXPECTED = {
    {kSpeechMarkSentence, 31, 95, 0,
     "Mary had <break time=\"300ms\"/>a little <mark name=\"animal\"/>lamb"},
    {kSpeechMarkWord, 31, 35, 6, "Mary"},
    {kSpeechMarkWord, 36, 39, 325, "had"},
    {kSpeechMarkWord, 40, 61, 897, "<break time=\"300ms\"/>"},
    {kSpeechMarkWord, 61, 62, 1291, "a"},
    {kSpeechMarkWord, 63, 69, 1373, "little"},
    {kSpeechMarkSSML, 70, 91, 1635, "animal"},
    {kSpeechMarkWord, 91, 95, 1635, "lamb"},
};

// This test comes from https://docs.aws.amazon.com/polly/latest/dg/speechmarkexamples.html
TEST_F(SpeechMarkTest, PollyExample2)
{
    auto result = parsePollySpeechMarks(POLLY_EXAMPLE_2, strlen(POLLY_EXAMPLE_2));
    ASSERT_EQ(result, POLLY_EXAMPLE_2_EXPECTED);
}


static const char *TURTLES_1 = R"apl([{"time":0,"type":"word","start":32,"end":54,"value":"<break time=\"250ms\" />"}])apl";
static const char *TURTLES_2 = R"apl([{"time":250,"type":"sentence","start":109,"end":171,"value":"Box turtles are North American turtles of the genus Terrapene."},{"time":262,"type":"word","start":109,"end":112,"value":"Box"},{"time":262,"type":"viseme","value":"p"},{"time":500,"type":"viseme","value":"a"},{"time":562,"type":"viseme","value":"k"},{"time":625,"type":"viseme","value":"s"}])apl";

static const std::vector<SpeechMark> TURTLES_EXPECTED = {
    {kSpeechMarkWord, 32, 54, 0, "<break time=\"250ms\" />"},
    {kSpeechMarkSentence, 109, 171, 250, "Box turtles are North American turtles of the genus Terrapene." },
    {kSpeechMarkWord, 109, 112, 262, "Box" },
    {kSpeechMarkViseme, 0, 0, 262, "p" },
    {kSpeechMarkViseme, 0, 0, 500, "a" },
    {kSpeechMarkViseme, 0, 0, 562, "k" },
    {kSpeechMarkViseme, 0, 0, 625, "s" },
};

// This example is copied out of an MP3 file
TEST_F(SpeechMarkTest, PollyTurtles)
{
    auto result1 = parsePollySpeechMarks(TURTLES_1, strlen(TURTLES_1));
    auto result2 = parsePollySpeechMarks(TURTLES_2, strlen(TURTLES_2));

    result1.insert(result1.end(), result2.begin(), result2.end());
    ASSERT_EQ(result1, TURTLES_EXPECTED);
}

static const char * POLLY_EXAMPLE_BAD = R"apl(
{"time":0,"type":"sentence","start":0,"end":23,"value":"Mary had a little lamb."}
{"time":6,"type":"word","start":0,"end":4,"value":"Mary"}
{"time":6,"type":"viseme","value":"p"}
{"time":73,"type":"viseme","value":"E"}
{"time":180,"type":"viseme","value":"r"}
{"time":292,"type":"viseme","value":"i"
{"time":373,"type":"word","start":5,"end":8,"value":"had"}
{"time":373,"type":"viseme","value":"k"}
{"time":460,"type":"viseme","value":"a"}
{"time":521,"type":"viseme","value":"t"}
{"time":604,"type":"word","start":9,"end":10,"value":"a"}
{"time":604,"type":"viseme","value":"@"}
{"time":643,"type":"word","start":11,"end":17,"value":"little"}
{"time":643,"type":"viseme","value":"t"}
{"time":739,"type":"viseme","value":"i"}
{"time":769,"type":"viseme","value":"t"}
{"time":799,"type":"viseme","value":"t"}
{"time":882,"type":"word","start":18,"end":22,"value":"lamb"}
{"time":882,"type":"viseme","value":"t"}
{"time":964,"type":"viseme","value":"a"}
{"time":1082,"type":"viseme","value":"p"}
)apl";

TEST_F(SpeechMarkTest, PollyExampleBad)
{
    auto result = parsePollySpeechMarks(POLLY_EXAMPLE_BAD, strlen(POLLY_EXAMPLE_BAD));
    ASSERT_EQ(result.size(), 6);
}