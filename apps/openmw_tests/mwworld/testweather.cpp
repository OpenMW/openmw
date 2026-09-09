#include <gtest/gtest.h>

#include <cmath>

#include "apps/openmw/mwworld/timestamp.hpp"
#include "apps/openmw/mwworld/weather.hpp"

namespace MWWorld
{
    namespace
    {
        TEST(MWWorldWeatherTest, moonPhasesHaveMwscriptCompatibleValues)
        {
            using Phase = MWRender::MoonState::Phase;

            EXPECT_EQ(MWRender::MoonState::phaseToInt(Phase::New), 0);
            EXPECT_EQ(MWRender::MoonState::phaseToInt(Phase::WaxingCrescent), 1);
            EXPECT_EQ(MWRender::MoonState::phaseToInt(Phase::WaningCrescent), 1);
            EXPECT_EQ(MWRender::MoonState::phaseToInt(Phase::FirstQuarter), 2);
            EXPECT_EQ(MWRender::MoonState::phaseToInt(Phase::ThirdQuarter), 2);
            EXPECT_EQ(MWRender::MoonState::phaseToInt(Phase::WaxingGibbous), 3);
            EXPECT_EQ(MWRender::MoonState::phaseToInt(Phase::WaningGibbous), 3);
            EXPECT_EQ(MWRender::MoonState::phaseToInt(Phase::Full), 4);
        }

        // MASSER PHASES

        TEST(MWWorldWeatherTest, masserPhasesFullToWaningGibbousAtCorrectTimes)
        {
            float dailyIncrement = 1.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 2 and 26, 11:57
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 2 + 11.0f + 56.0f / 60.0f);
            timeStampAfter += (24.0f * 2 + 11.0f + 58.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 26 + 11.0f + 56.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 26 + 11.0f + 58.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(0));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(1));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(0));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(1));
        }

        TEST(MWWorldWeatherTest, masserPhasesWaningGibbousToThirdQuarterAtCorrectTimes)
        {
            float dailyIncrement = 1.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 5 and 29, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 4 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 5 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 28 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 29 + 0.0f + 1.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(1));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(2));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(1));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(2));
        }

        TEST(MWWorldWeatherTest, masserPhasesThirdQuarterToWaningCrescentAtCorrectTimes)
        {
            float dailyIncrement = 1.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 8 and 32, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 7 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 8 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 31 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 32 + 0.0f + 1.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(2));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(3));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(2));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(3));
        }

        TEST(MWWorldWeatherTest, masserPhasesWaningCrescentToNewAtCorrectTimes)
        {
            float dailyIncrement = 1.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 11 and 35, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 10 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 11 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 34 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 35 + 0.0f + 1.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(3));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(4));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(3));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(4));
        }

        TEST(MWWorldWeatherTest, masserPhasesNewToWaxingCrescentAtCorrectTimes)
        {
            float dailyIncrement = 1.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 14 and 38, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 13 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 14 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 37 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 38 + 0.0f + 1.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(4));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(5));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(4));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(5));
        }

        TEST(MWWorldWeatherTest, masserPhasesWaxingCrescentToFirstQuarterAtCorrectTimes)
        {
            float dailyIncrement = 1.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 17 and 41, 2:57
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 17 + 2.0f + 56.0f / 60.0f);
            timeStampAfter += (24.0f * 17 + 2.0f + 58.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 41 + 2.0f + 56.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 41 + 2.0f + 58.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(5));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(6));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(5));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(6));
        }

        TEST(MWWorldWeatherTest, masserPhasesFirstQuarterToWaxingGibbousAtCorrectTimes)
        {
            float dailyIncrement = 1.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 20 and 44, 5:57
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 20 + 5.0f + 56.0f / 60.0f);
            timeStampAfter += (24.0f * 20 + 5.0f + 58.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 44 + 5.0f + 56.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 44 + 5.0f + 58.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(6));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(7));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(6));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(7));
        }

        TEST(MWWorldWeatherTest, masserPhasesWaxingGibbousToFullAtCorrectTimes)
        {
            float dailyIncrement = 1.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 23 and 47, 8:57
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 23 + 8.0f + 56.0f / 60.0f);
            timeStampAfter += (24.0f * 23 + 8.0f + 58.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 47 + 8.0f + 56.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 47 + 8.0f + 58.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(7));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(0));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(7));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(0));
        }

        // SECUNDA PHASES

        TEST(MWWorldWeatherTest, secundaPhasesFullToWaningGibbousAtCorrectTimes)
        {
            float dailyIncrement = 1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 2 and 26, 14:19
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 2 + 14.0f + 18.0f / 60.0f);
            timeStampAfter += (24.0f * 2 + 14.0f + 20.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 26 + 14.0f + 18.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 26 + 14.0f + 20.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(0));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(1));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(0));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(1));
        }

        TEST(MWWorldWeatherTest, secundaPhasesWaningGibbousToThirdQuarterAtCorrectTimes)
        {
            float dailyIncrement = 1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 5 and 29, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 4 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 5 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 28 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 29 + 0.0f + 1.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(1));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(2));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(1));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(2));
        }

        TEST(MWWorldWeatherTest, secundaPhasesThirdQuarterToWaningCrescentAtCorrectTimes)
        {
            float dailyIncrement = 1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 8 and 32, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 7 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 8 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 31 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 32 + 0.0f + 1.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(2));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(3));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(2));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(3));
        }

        TEST(MWWorldWeatherTest, secundaPhasesWaningCrescentToNewAtCorrectTimes)
        {
            float dailyIncrement = 1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 11 and 35, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 10 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 11 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 34 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 35 + 0.0f + 1.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(3));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(4));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(3));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(4));
        }

        TEST(MWWorldWeatherTest, secundaPhasesNewToWaxingCrescentAtCorrectTimes)
        {
            float dailyIncrement = 1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 14 and 38, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 13 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 14 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 37 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 38 + 0.0f + 1.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(4));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(5));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(4));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(5));
        }

        TEST(MWWorldWeatherTest, secundaPhasesWaxingCrescentToFirstQuarterAtCorrectTimes)
        {
            float dailyIncrement = 1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 17 and 41, 3:31
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 17 + 3.0f + 30.0f / 60.0f);
            timeStampAfter += (24.0f * 17 + 3.0f + 32.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 41 + 3.0f + 30.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 41 + 3.0f + 32.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(5));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(6));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(5));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(6));
        }

        TEST(MWWorldWeatherTest, secundaPhasesFirstQuarterToWaxingGibbousAtCorrectTimes)
        {
            float dailyIncrement = 1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 20 and 44, 7:07
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 20 + 7.0f + 6.0f / 60.0f);
            timeStampAfter += (24.0f * 20 + 7.0f + 8.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 44 + 7.0f + 6.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 44 + 7.0f + 8.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(6));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(7));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(6));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(7));
        }

        TEST(MWWorldWeatherTest, secundaPhasesWaxingGibbousToFullAtCorrectTimes)
        {
            float dailyIncrement = 1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 23 and 47, 10:43
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 23 + 10.0f + 42.0f / 60.0f);
            timeStampAfter += (24.0f * 23 + 10.0f + 44.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 47 + 10.0f + 42.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 47 + 10.0f + 44.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(7));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(0));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(7));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(0));
        }

        // OFFSETS

        TEST(MWWorldWeatherTest, secundaShouldApplyIncrementOffsetAfterFirstLoop)
        {
            float dailyIncrement = 1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 8 and 32, 3:16
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 8 + 3.0f + 15.0f / 60.0f);
            timeStampAfter += (24.0f * 8 + 3.0f + 17.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 32 + 3.0f + 15.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 32 + 3.0f + 17.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_LE(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_LE(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWithLowIncrementShouldApplyIncrementOffsetAfterCycle)
        {
            float dailyIncrement = 0.9f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 0.0f;
            float fadeInFinish = 0.0f;
            float fadeOutStart = 0.0f;
            float fadeOutFinish = 0.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 7 and 31, 1:44
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 7 + 1.0f + 43.0f / 60.0f);
            timeStampAfter += (24.0f * 7 + 1.0f + 45.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 31 + 1.0f + 43.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 31 + 1.0f + 45.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_LE(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_LE(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, masserShouldApplyIncrementOffsetAfterCycle)
        {
            float dailyIncrement = 1.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 0.0f;
            float fadeInFinish = 0.0f;
            float fadeOutStart = 0.0f;
            float fadeOutFinish = 0.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 4 and 28, 1:02
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 4 + 1.0f + 1.0f / 60.0f);
            timeStampAfter += (24.0f * 4 + 1.0f + 3.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 28 + 1.0f + 1.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 28 + 1.0f + 3.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_LE(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_LE(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, secundaShouldApplyIncrementOffsetAfterCycle)
        {
            float dailyIncrement = 1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 0.0f;
            float fadeInFinish = 0.0f;
            float fadeOutStart = 0.0f;
            float fadeOutFinish = 0.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 3 and 27, 2:04
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 3 + 2.0f + 3.0f / 60.0f);
            timeStampAfter += (24.0f * 3 + 2.0f + 5.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 27 + 2.0f + 3.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 27 + 2.0f + 5.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_LE(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_LE(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWithIncreasedSpeedShouldApplyIncrementOffsetAfterCycle)
        {
            float dailyIncrement = 1.2f;
            float speed = 1.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 0.0f;
            float fadeInFinish = 0.0f;
            float fadeOutStart = 0.0f;
            float fadeOutFinish = 0.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 4 and 28, 1:13
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 4 + 1.0f + 12.0f / 60.0f);
            timeStampAfter += (24.0f * 4 + 1.0f + 14.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 28 + 1.0f + 12.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 28 + 1.0f + 14.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_LE(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_LE(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        // HIGH AND NEGATIVE INCREMENTS

        TEST(MWWorldWeatherTest, moonWithHalfDayIncrementShouldRiseAtTheSameHourEveryDay)
        {
            float dailyIncrement = 12.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 3 and 27, 17:03 (a daily increment of 12 hours leaves the moon rise hour where it is)
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 3 + 17.0f + 2.0f / 60.0f);
            timeStampAfter += (24.0f * 3 + 17.0f + 4.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 27 + 17.0f + 2.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 27 + 17.0f + 4.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_LE(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_LE(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWithHalfDayIncrementShouldSetAtTheSameHourEveryDay)
        {
            float dailyIncrement = 12.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 4 and 28, 5:57
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 4 + 5.0f + 56.0f / 60.0f);
            timeStampAfter += (24.0f * 4 + 5.0f + 58.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 28 + 5.0f + 56.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 28 + 5.0f + 58.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_GT(beforeState.mMoonAlpha, 0.0f);
            EXPECT_LE(afterState.mMoonAlpha, 0.0f);
            EXPECT_GT(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_LE(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWithNegativeIncrementShouldRiseEarlierEachDay)
        {
            float dailyIncrement = -1.0f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 9 and 33, 2:17 (the moon rose an hour before midnight)
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 9 + 2.0f + 16.0f / 60.0f);
            timeStampAfter += (24.0f * 9 + 2.0f + 18.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 33 + 2.0f + 16.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 33 + 2.0f + 18.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_LE(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_LE(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWithNegativeIncrementShouldApplyCycleOffsetAfterCrossingMidnight)
        {
            float dailyIncrement = -1.0f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 12 and 36, 0:17 (the increment is skipped once the moon becomes visible before midnight)
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 12 + 0.0f + 16.0f / 60.0f);
            timeStampAfter += (24.0f * 12 + 0.0f + 18.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 36 + 0.0f + 16.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 36 + 0.0f + 18.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_LE(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_LE(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWithNegativeIncrementPhasesWaxingCrescentToFirstQuarterAtCorrectTimes)
        {
            float dailyIncrement = -1.0f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 17 and 41, 8:43
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 17 + 8.0f + 42.0f / 60.0f);
            timeStampAfter += (24.0f * 17 + 8.0f + 45.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 41 + 8.0f + 42.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 41 + 8.0f + 45.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(5));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(6));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(5));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(6));
        }

        // LARGE AND FRACTIONAL NEGATIVE INCREMENTS

        TEST(MWWorldWeatherTest, moonWithLargeIncrementShouldRiseLaterEachDay)
        {
            float dailyIncrement = 8.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 4 and 28, 21:03 (the moon rose at 16:00, eight hours after the previous day)
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 4 + 21.0f + 2.0f / 60.0f);
            timeStampAfter += (24.0f * 4 + 21.0f + 4.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 28 + 21.0f + 2.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 28 + 21.0f + 4.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_LE(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_LE(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWithLargeIncrementShouldNotRiseOnEveryDay)
        {
            float dailyIncrement = 8.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 5 and 29, 9:57 (the rise hour reached 24, so the moon only sets on this day)
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 5 + 9.0f + 56.0f / 60.0f);
            timeStampAfter += (24.0f * 5 + 9.0f + 58.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 29 + 9.0f + 56.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 29 + 9.0f + 58.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_GT(beforeState.mMoonAlpha, 0.0f);
            EXPECT_LE(afterState.mMoonAlpha, 0.0f);
            EXPECT_GT(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_LE(afterStatePostLoop.mMoonAlpha, 0.0f);

            TimeStamp timeStampEvening, timeStampEveningPostLoop;
            timeStampEvening += (24.0f * 5 + 21.0f + 4.0f / 60.0f);
            timeStampEveningPostLoop += (24.0f * 29 + 21.0f + 4.0f / 60.0f);

            EXPECT_LE(moon.calculateState(timeStampEvening).mMoonAlpha, 0.0f);
            EXPECT_LE(moon.calculateState(timeStampEveningPostLoop).mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWithNegativeIncrementShouldDeferTheDailyUpdatePastMidnight)
        {
            float dailyIncrement = -1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 7 and 31, 0:53 (the increment that would make the moon rise before midnight waits)
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 7 + 0.0f + 52.0f / 60.0f);
            timeStampAfter += (24.0f * 7 + 0.0f + 54.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 31 + 0.0f + 52.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 31 + 0.0f + 54.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_LE(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_LE(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWithNegativeIncrementShouldApplyTheDeferredUpdateWhenTheMoonFadesOut)
        {
            float dailyIncrement = -1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 6 and 7 (and 30 and 31), 14:05: the moon reappears after fading in on the day the
            // increment is applied at midnight, but not on the day it is deferred to the fade out.
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 6 + 14.0f + 5.0f / 60.0f);
            timeStampAfter += (24.0f * 7 + 14.0f + 5.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 30 + 14.0f + 5.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 31 + 14.0f + 5.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_GT(beforeState.mMoonAlpha, 0.0f);
            EXPECT_LE(afterState.mMoonAlpha, 0.0f);
            EXPECT_GT(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_LE(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWithNegativeIncrementPhasesThirdQuarterToWaningCrescentAtCorrectTimes)
        {
            float dailyIncrement = -1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 30.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 8 and 32, 16:45
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 8 + 16.0f + 43.0f / 60.0f);
            timeStampAfter += (24.0f * 8 + 16.0f + 47.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 32 + 16.0f + 43.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 32 + 16.0f + 47.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(2));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(3));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(2));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(3));
        }

        // HANDOVER OF THE DAY'S RISE HOUR

        TEST(MWWorldWeatherTest, moonHandsOverBehindTheDaytimeFadeBeforeSunrise)
        {
            float dailyIncrement = 10.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 7.0f;
            float fadeInFinish = 8.0f;
            float fadeOutStart = 2.0f;
            float fadeOutFinish = 3.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 5 and 17, 7:00. The moon is out of sight from 2:59, so the day's rise hour takes over there and the
            // moon is well up when the fade lets it show again.
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 5 + 6.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 5 + 7.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 17 + 6.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 17 + 7.0f + 1.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_LE(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_LE(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWaitsToHandOverUntilItSetsAfterSunrise)
        {
            float dailyIncrement = 10.0f;
            float speed = 0.5f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 11.0f;
            float fadeInFinish = 12.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            // Days 6 and 18, 11:57. The fade at 9:59 is after sunrise, so it isn't cover enough. The moon keeps
            // yesterday's rise hour and sets on it.
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 6 + 11.0f + 56.0f / 60.0f);
            timeStampAfter += (24.0f * 6 + 11.0f + 58.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 18 + 11.0f + 56.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 18 + 11.0f + 58.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_GT(beforeState.mMoonAlpha, 0.0f);
            EXPECT_LE(afterState.mMoonAlpha, 0.0f);
            EXPECT_GT(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_LE(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonPhaseTurnsOverWithTheRiseHour)
        {
            float dailyIncrement = 10.0f;
            float speed = 1.0f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 4.0f;
            float fadeInFinish = 5.0f;
            float fadeOutStart = 2.0f;
            float fadeOutFinish = 3.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 5 and 29, 2:59
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 5 + 2.0f + 57.0f / 60.0f);
            timeStampAfter += (24.0f * 5 + 3.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 29 + 2.0f + 57.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 29 + 3.0f + 1.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_EQ(beforeState.mPhase, static_cast<MWRender::MoonState::Phase>(1));
            EXPECT_EQ(afterState.mPhase, static_cast<MWRender::MoonState::Phase>(2));
            EXPECT_EQ(beforeStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(1));
            EXPECT_EQ(afterStatePostLoop.mPhase, static_cast<MWRender::MoonState::Phase>(2));
        }

        // DAY LENGTH

        TEST(MWWorldWeatherTest, moonCrossesTheSkyAtAMultipleOfTheSunsSpeed)
        {
            float dailyIncrement = 10.0f;
            float speed = 5.0f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 13.0f;
            float fadeInFinish = 14.0f;
            float fadeOutStart = 11.0f;
            float fadeOutFinish = 12.0f;
            float axisOffset = 50.0f;
            float sunriseHour = 5.0f;
            float sunsetHour = 19.0f;

            // Days 2 and 14, 14:13. A 14 hour day makes the sun, and so the moon, slower than it is on a 12 hour one.
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 2 + 14.0f + 11.0f / 60.0f);
            timeStampAfter += (24.0f * 2 + 14.0f + 15.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 14 + 14.0f + 11.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 14 + 14.0f + 15.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_GT(beforeState.mMoonAlpha, 0.0f);
            EXPECT_LE(afterState.mMoonAlpha, 0.0f);
            EXPECT_GT(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_LE(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonHandsOverBehindTheDaytimeFadeBeforeALaterSunrise)
        {
            float dailyIncrement = 10.0f;
            float speed = 0.3f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 10.0f;
            float fadeInFinish = 11.0f;
            float fadeOutStart = 6.0f;
            float fadeOutFinish = 7.0f;
            float axisOffset = 35.0f;
            float sunriseHour = 10.0f;
            float sunsetHour = 18.0f;

            // Days 3 and 15, 10:00. The fade at 6:59 is before this sunrise, so it is cover enough for the day's rise
            // hour to take over there.
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 3 + 9.0f + 58.0f / 60.0f);
            timeStampAfter += (24.0f * 3 + 10.0f + 2.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 15 + 9.0f + 58.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 15 + 10.0f + 2.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            EXPECT_LE(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_LE(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }
    }
}
