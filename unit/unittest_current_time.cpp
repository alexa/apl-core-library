/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "testeventloop.h"

using namespace apl;

class CurrentTimeTest : public DocumentWrapper {};

static const char *TIME =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${elapsedTime} ${localTime}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, Basic)
{
    // Thu Sep 05 2019 12:15:39  (LocalTime)
    const apl_time_t START_TIME = 1567685739476;
    config.localTime(START_TIME);

    loadDocument(TIME);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("0 1567685739476", component->getCalculated(kPropertyText).asString()));

    // Move forward one second
    root->updateTime(1000);
    ASSERT_TRUE(IsEqual("1000 1567685740476", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, component));

    // Now we'll adjust elapsed time AND local time
    root->updateTime(1001, START_TIME - 10);
    ASSERT_TRUE(IsEqual("1001 1567685739466", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(1001, root->currentTime()));
}


static const char *TIME_YEAR =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.year(localTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, Year)
{
    // Thu Sep 05 2019 12:15:39  (LocalTime)
    const apl_time_t START_TIME = 1567685739476;

    // Start in 1989
    config.localTime(START_TIME - 30 * 1000L * 3600 * 24 * 365);

    loadDocument(TIME_YEAR);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual("1989", component->getCalculated(kPropertyText).asString()));

    // Move forward approximately 30 years (advance both local and elapsed time)
    root->updateTime(30 * 1000L * 3600 * 24 * 365);
    ASSERT_TRUE(IsEqual("2019", component->getCalculated(kPropertyText).asString()));

    // Move forward another year
    root->updateTime(root->currentTime() + 1000L * 3600 * 24 * 365);
    ASSERT_TRUE(IsEqual("2020", component->getCalculated(kPropertyText).asString()));

    // Jump forward to 2024
    root->updateTime(root->currentTime() + 4 * 1000L * 3600 * 24 * 365.24);
    ASSERT_TRUE(IsEqual("2024", component->getCalculated(kPropertyText).asString()));

    // Jump to one milliseconds later
    root->updateTime(root->currentTime() + 1);  // Just a millisecond later
    ASSERT_TRUE(IsEqual("2024", component->getCalculated(kPropertyText).asString()));

    // Add another 100 years
    root->updateTime(root->currentTime() + 100 * 1000L * 3600 * 24 * 365);
    ASSERT_TRUE(IsEqual("2124", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_MONTH =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.month(localTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, Month)
{
    // Thu Sep 05 2019 12:15:39  (LocalTime)
    const apl_time_t START_TIME = 1567685739476;
    config.localTime(START_TIME);

    loadDocument(TIME_MONTH);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual("8", component->getCalculated(kPropertyText).asString()));

    root->updateTime(1000L * 3600 * 24 * 31);
    ASSERT_TRUE(IsEqual("9", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_DATE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.date(localTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, Date)
{
    // Thu Sep 05 2019 12:15:39  (LocalTime)
    const apl_time_t START_TIME = 1567685739476;
    config.localTime(START_TIME);

    loadDocument(TIME_DATE);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("5", component->getCalculated(kPropertyText).asString()));

    // Advance 24 hours
    root->updateTime(1000 * 3600 * 24);
    ASSERT_TRUE(IsEqual("6", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_UTC_DATE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.date(localTime) + ' ' + Time.date(utcTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, UTCDate)
{
    // Thu Sep 05 2019 15:39:17  (LocalTime)
    const apl_time_t START_TIME = 1567697957924;
    config.localTime(START_TIME).localTimeAdjustment(-11 * 3600 * 1000);  // -11 hours from UTC

    loadDocument(TIME_UTC_DATE);
    ASSERT_TRUE(component);

    // 11 hours behind UTC means that UTC is one day ahead (3:39 PM + 11 hours = 2:39 AM)
    ASSERT_TRUE(IsEqual("5 6", component->getCalculated(kPropertyText).asString()));

    // Move forward one day and verify everything updates
    root->updateTime(1000 * 3600 * 24);
    ASSERT_TRUE(IsEqual("6 7", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_WEEK_DAY =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.weekDay(localTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, WeekDay)
{
    // Thu Sep 05 2019 12:15:39  (LocalTime)
    const apl_time_t START_TIME = 1567685739476;
    config.localTime(START_TIME);

    loadDocument(TIME_WEEK_DAY);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("4", component->getCalculated(kPropertyText).asString()));

    root->updateTime(1000 * 3600 * 24);
    ASSERT_TRUE(IsEqual("5", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_UTC_WEEK_DAY =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.weekDay(localTime) + ' ' + Time.weekDay(utcTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, UTCWeekDay)
{
    // Thu Sep 05 2019 15:39:17  (LocalTime)
    const apl_time_t START_TIME = 1567697957924;
    config.localTime(START_TIME).localTimeAdjustment(-11 * 3600 * 1000);  // -11 hours from UTC

    loadDocument(TIME_UTC_WEEK_DAY);
    ASSERT_TRUE(component);

    // 11 hours behind UTC means that UTC is one day ahead (3:39 PM + 11 hours = 2:39 AM)
    ASSERT_TRUE(IsEqual("4 5", component->getCalculated(kPropertyText).asString()));

    // Move forward two days and verify everything updates
    root->updateTime(1000 * 3600 * 24 * 2);
    ASSERT_TRUE(IsEqual("6 0", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_HOURS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.hours(localTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, Hours)
{
    // Thu Sep 05 2019 12:15:39  (LocalTime)
    const apl_time_t START_TIME = 1567685739476;
    config.localTime(START_TIME);

    loadDocument(TIME_HOURS);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("12", component->getCalculated(kPropertyText).asString()));

    root->updateTime(1000 * 3600);
    ASSERT_TRUE(IsEqual("13", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_UTC_HOURS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.hours(localTime) + ' ' + Time.hours(utcTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, UTCHours)
{
    // Thu Sep 05 2019 15:39:17  (LocalTime)
    const apl_time_t START_TIME = 1567697957924;
    config.localTime(START_TIME).localTimeAdjustment(-11 * 3600 * 1000);  // -11 hours from UTC

    loadDocument(TIME_UTC_HOURS);
    ASSERT_TRUE(component);

    // 11 hours behind UTC means that UTC is one day ahead (3:39 PM + 11 hours = 2:39 AM)
    ASSERT_TRUE(IsEqual("15 2", component->getCalculated(kPropertyText).asString()));

    // Move forward two hours and verify everything updates
    root->updateTime(1000 * 3600 * 2);
    ASSERT_TRUE(IsEqual("17 4", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_MINUTES =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.minutes(localTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, Minutes)
{
    // Thu Sep 05 2019 12:15:39  (LocalTime)
    const apl_time_t START_TIME = 1567685739476;
    config.localTime(START_TIME);

    loadDocument(TIME_MINUTES);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("15", component->getCalculated(kPropertyText).asString()));

    root->updateTime( 1000 * 60);
    ASSERT_TRUE(IsEqual("16", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_UTC_MINUTES =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.minutes(localTime) + ' ' + Time.minutes(utcTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, UTCMinutes)
{
    // Thu Sep 05 2019 15:39:17  (LocalTime)
    const apl_time_t START_TIME = 1567697957924;
    config.localTime(START_TIME).localTimeAdjustment(-6.5 * 3600 * 1000);  // -6.5 hours from UTC

    loadDocument(TIME_UTC_MINUTES);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual(static_cast<double>(START_TIME), context->opt("localTime")));
    ASSERT_TRUE(IsEqual(static_cast<double>(START_TIME + 6.5 * 3600 * 1000), context->opt("utcTime")));

    // 6.5 hours behind UTC means that UTC is (3:39 PM + 6.5 hours = 10:09 AM)
    ASSERT_TRUE(IsEqual("39 9", component->getCalculated(kPropertyText).asString()));

    // Move forward 21 minutes and verify everything updates
    root->updateTime(1000 * 60 * 21);
    ASSERT_TRUE(IsEqual("0 30", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_SECONDS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.seconds(localTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, Seconds)
{
    // Thu Sep 05 2019 12:15:39  (LocalTime)
    const apl_time_t START_TIME = 1567685739476;
    config.localTime(START_TIME);

    loadDocument(TIME_SECONDS);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("39", component->getCalculated(kPropertyText).asString()));

    root->updateTime(1000);
    ASSERT_TRUE(IsEqual("40", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_UTC_SECONDS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.seconds(localTime) + ' ' + Time.seconds(utcTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, UTCSeconds)
{
    // Thu Sep 05 2019 15:39:17  (LocalTime)
    const apl_time_t START_TIME = 1567697957924;
    config.localTime(START_TIME).localTimeAdjustment(-6.5 * 3600 * 1000);  // -6.5 hours from UTC

    loadDocument(TIME_UTC_SECONDS);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual(static_cast<double>(START_TIME), context->opt("localTime")));
    ASSERT_TRUE(IsEqual(static_cast<double>(START_TIME + 6.5 * 3600 * 1000), context->opt("utcTime")));

    // 6.5 hours behind UTC means that UTC is (3:39 PM + 6.5 hours = 10:09 AM)
    ASSERT_TRUE(IsEqual("17 17", component->getCalculated(kPropertyText).asString()));

    // Move forward 21 seconds and verify everything updates
    root->updateTime(1000 * 21);
    ASSERT_TRUE(IsEqual("38 38", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_MILLISECONDS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.milliseconds(localTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, Milliseconds)
{
    // Thu Sep 05 2019 12:15:39  (LocalTime)
    const apl_time_t START_TIME = 1567685739476;
    config.localTime(START_TIME);

    loadDocument(TIME_MILLISECONDS);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("476", component->getCalculated(kPropertyText).asString()));

    root->updateTime(1);
    ASSERT_TRUE(IsEqual("477", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_UTC_MILLISECONDS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${Time.milliseconds(localTime) + ' ' + Time.milliseconds(utcTime)}\""
    "    }"
    "  }"
    "}";

TEST_F(CurrentTimeTest, UTCMilliseconds)
{
    // Thu Sep 05 2019 15:39:17  (LocalTime)
    const apl_time_t START_TIME = 1567697957924;
    config.localTime(START_TIME).localTimeAdjustment(-6.5 * 3600 * 1000);  // -6.5 hours from UTC

    loadDocument(TIME_UTC_MILLISECONDS);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual(static_cast<double>(START_TIME), context->opt("localTime")));
    ASSERT_TRUE(IsEqual(static_cast<double>(START_TIME + 6.5 * 3600 * 1000), context->opt("utcTime")));

    // 6.5 hours behind UTC means that UTC is (3:39 PM + 6.5 hours = 10:09 AM)
    ASSERT_TRUE(IsEqual("924 924", component->getCalculated(kPropertyText).asString()));

    // Move forward 92 milliseconds and verify everything updates
    root->updateTime(92);
    ASSERT_TRUE(IsEqual("16 16", component->getCalculated(kPropertyText).asString()));
}

static const char *TIME_FORMAT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"data\": ["
    "        \"h:m:s\","
    "        \"hh:mm:ss\","
    "        \"HH:mm:ss\","
    "        \"d/M/YY\","
    "        \"dd/MM/YYYY\""
    "      ],"
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"text\": \"${Time.format(data, localTime)} ${Time.format(data, utcTime)}\""
    "      }"
    "    }"
    "  }"
    "}";

static std::vector<std::string> TIME_FORMAT_ANSWERS = {
    "3:9:7 9:39:7",
    "03:09:07 09:39:07",
    "15:09:07 21:39:07",
    "5/9/19 5/9/19",
    "05/09/2019 05/09/2019"
};

TEST_F(CurrentTimeTest, Format)
{
    // Thu Sep 05 2019 15:09:07  (LocalTime)
    // Thu Sep 05 2019 21:39:07  (UTC)
    const apl_time_t START_TIME = 1567696147924;

    config.localTimeAdjustment(-6.5 * 3600 * 1000L);
    config.localTime(START_TIME);
    loadDocument(TIME_FORMAT);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual(static_cast<double>(START_TIME), context->opt("localTime")));
    ASSERT_TRUE(IsEqual(static_cast<double>(START_TIME + 6.5 * 3600 * 1000L), context->opt("utcTime")));

    ASSERT_EQ(TIME_FORMAT_ANSWERS.size(), component->getChildCount());

    for (int i = 0 ; i < TIME_FORMAT_ANSWERS.size() ; i++) {
        auto text = component->getChildAt(i);
        ASSERT_TRUE(IsEqual(TIME_FORMAT_ANSWERS.at(i), text->getCalculated(kPropertyText).asString())) << i;
    }
}
