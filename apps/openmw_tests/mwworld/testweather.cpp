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

            // Days 2 and 9626, 11:57
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 2 + 11.0f + 56.0f / 60.0f);
            timeStampAfter += (24.0f * 2 + 11.0f + 58.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9626 + 11.0f + 56.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9626 + 11.0f + 58.0f / 60.0f);

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

            // Days 5 and 9629, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 4 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 5 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9628 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9629 + 0.0f + 1.0f / 60.0f);

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

            // Days 8 and 9632, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 7 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 8 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9631 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9632 + 0.0f + 1.0f / 60.0f);

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

            // Days 11 and 9635, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 10 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 11 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9634 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9635 + 0.0f + 1.0f / 60.0f);

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

            // Days 14 and 9638, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 13 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 14 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9637 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9638 + 0.0f + 1.0f / 60.0f);

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

            // Days 17 and 9641, 2:57
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 17 + 2.0f + 56.0f / 60.0f);
            timeStampAfter += (24.0f * 17 + 2.0f + 58.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9641 + 2.0f + 56.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9641 + 2.0f + 58.0f / 60.0f);

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

            // Days 20 and 9644, 5:57
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 20 + 5.0f + 56.0f / 60.0f);
            timeStampAfter += (24.0f * 20 + 5.0f + 58.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9644 + 5.0f + 56.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9644 + 5.0f + 58.0f / 60.0f);

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

            // Days 23 and 9647, 8:57
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 23 + 8.0f + 56.0f / 60.0f);
            timeStampAfter += (24.0f * 23 + 8.0f + 58.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9647 + 8.0f + 56.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9647 + 8.0f + 58.0f / 60.0f);

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

            // Days 2 and 9626, 14:19
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 2 + 14.0f + 18.0f / 60.0f);
            timeStampAfter += (24.0f * 2 + 14.0f + 20.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9626 + 14.0f + 18.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9626 + 14.0f + 20.0f / 60.0f);

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

            // Days 5 and 9629, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 4 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 5 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9628 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9629 + 0.0f + 1.0f / 60.0f);

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

            // Days 8 and 9632, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 7 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 8 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9631 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9632 + 0.0f + 1.0f / 60.0f);

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

            // Days 11 and 9635, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 10 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 11 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9634 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9635 + 0.0f + 1.0f / 60.0f);

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

            // Days 14 and 9638, 0:00
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 13 + 23.0f + 59.0f / 60.0f);
            timeStampAfter += (24.0f * 14 + 0.0f + 1.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9637 + 23.0f + 59.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9638 + 0.0f + 1.0f / 60.0f);

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

            // Days 17 and 9641, 3:31
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 17 + 3.0f + 30.0f / 60.0f);
            timeStampAfter += (24.0f * 17 + 3.0f + 32.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9641 + 3.0f + 30.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9641 + 3.0f + 32.0f / 60.0f);

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

            // Days 20 and 9644, 7:07
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 20 + 7.0f + 6.0f / 60.0f);
            timeStampAfter += (24.0f * 20 + 7.0f + 8.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9644 + 7.0f + 6.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9644 + 7.0f + 8.0f / 60.0f);

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

            // Days 23 and 9647, 10:43
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0f * 23 + 10.0f + 42.0f / 60.0f);
            timeStampAfter += (24.0f * 23 + 10.0f + 44.0f / 60.0f);
            timeStampBeforePostLoop += (24.0f * 9647 + 10.0f + 42.0f / 60.0f);
            timeStampAfterPostLoop += (24.0f * 9647 + 10.0f + 44.0f / 60.0f);

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

        // 24-DAY LOOP

        TEST(MWWorldWeatherTest, moonShouldApplySkippedIncrementsAfterFirstLoop)
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

        TEST(MWWorldWeatherTest, moonWithoutAHandoverShouldNotJumpAtTheEndOfTheLoop)
        {
            float dailyIncrement = 0.4f;
            float speed = 0.15f;
            float fadeEndAngle = 10.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 5.0f;
            float fadeInStart = 14.0f;
            float fadeInFinish = 15.0f;
            float fadeOutStart = 7.0f;
            float fadeOutFinish = 10.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Days 24 and 48, 23:59. Visible at midnight on the last day of the cycle, so no handover.
            TimeStamp timeStampBefore, timeStampAfter, timeStampBeforePostLoop, timeStampAfterPostLoop;
            timeStampBefore += (24.0 * 24 + 23.0 + 59.0 / 60.0);
            timeStampAfter += (24.0 * 25);
            timeStampBeforePostLoop += (24.0 * 48 + 23.0 + 59.0 / 60.0);
            timeStampAfterPostLoop += (24.0 * 49);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            MWRender::MoonState beforeState = moon.calculateState(timeStampBefore);
            MWRender::MoonState afterState = moon.calculateState(timeStampAfter);
            MWRender::MoonState beforeStatePostLoop = moon.calculateState(timeStampBeforePostLoop);
            MWRender::MoonState afterStatePostLoop = moon.calculateState(timeStampAfterPostLoop);

            // Under minimum speed, so 7.83 degrees an hour.
            float oneMinuteOfRotation = 180.0f / 23.0f / 60.0f;

            EXPECT_NEAR(afterState.mRotationFromHorizon, beforeState.mRotationFromHorizon, oneMinuteOfRotation);
            EXPECT_NEAR(
                afterStatePostLoop.mRotationFromHorizon, beforeStatePostLoop.mRotationFromHorizon, oneMinuteOfRotation);

            EXPECT_GT(beforeState.mMoonAlpha, 0.0f);
            EXPECT_GT(afterState.mMoonAlpha, 0.0f);
            EXPECT_GT(beforeStatePostLoop.mMoonAlpha, 0.0f);
            EXPECT_GT(afterStatePostLoop.mMoonAlpha, 0.0f);
        }

        // FIXED RISE HOURS

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

            // Days 3 and 27, 17:03 (a daily increment of 12 hours doesn't advance the moon rise hour)
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

            // Days 4 and 28, 5:57 (a daily increment of 12 hours doesn't advance the moon set hour)
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

        // NEGATIVE INCREMENT RISE HOURS

        TEST(MWWorldWeatherTest, moonWithNegativeNonZeroIncrementRisesEarlierEachDay)
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

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            // The moon appears an hour earlier for three consecutive days
            const float appearanceTimes[] = { 2.0f + 17.0f / 60.0f, 1.0f + 17.0f / 60.0f, 0.0f + 17.0f / 60.0f };
            int day = 9;
            for (float hour : appearanceTimes)
            {
                TimeStamp before, after;
                before += (24.0f * day + hour - 1.5f / 60.0f);
                after += (24.0f * day + hour + 1.5f / 60.0f);

                EXPECT_LE(moon.calculateState(before).mMoonAlpha, 0.0f) << "day " << day;
                EXPECT_GT(moon.calculateState(after).mMoonAlpha, 0.0f) << "day " << day;
                ++day;
            }

            TimeStamp beforePostLoop, afterPostLoop;
            beforePostLoop += (24.0f * 33 + 2.0f + 15.5f / 60.0f);
            afterPostLoop += (24.0f * 33 + 2.0f + 18.5f / 60.0f);

            EXPECT_LE(moon.calculateState(beforePostLoop).mMoonAlpha, 0.0f);
            EXPECT_GT(moon.calculateState(afterPostLoop).mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWithNegativeNonZeroIncrementKeepsRisingWhenItsRiseHourCrossesMidnight)
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

            // Days 12 and 36, 0:17
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

        // RISE HOURS THAT SKIP A DAY

        TEST(MWWorldWeatherTest, moonWithLargeIncrementSkipsTheDayItsRiseHourReachesMidnight)
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

            TimeStamp dayThreeMoonSetting, dayThreeMoonDark, dayFourMoonDark, dayFourMoonVisible, dayFourMoonSetting,
                dayFourMoonDarkAgain, dayFiveMoonNight;

            // Day 3's moon rose at 08:00 and sets at 01:58.
            dayThreeMoonSetting += (24.0f * 4 + 1.0f + 57.0f / 60.0f);
            dayThreeMoonDark += (24.0f * 4 + 1.0f + 59.0f / 60.0f);

            // Day 4's moon rose at 16:00 and becomes visible at 21:03.
            dayFourMoonDark += (24.0f * 4 + 21.0f + 2.0f / 60.0f);
            dayFourMoonVisible += (24.0f * 4 + 21.0f + 4.0f / 60.0f);

            // Day 4's moon sets, but the handover is delayed until 17:57, so no new rise.
            dayFourMoonSetting += (24.0f * 5 + 9.0f + 56.0f / 60.0f);
            dayFourMoonDarkAgain += (24.0f * 5 + 9.0f + 58.0f / 60.0f);

            // Day 5's moon remains dark after the handover because it's past the new visible range.
            dayFiveMoonNight += (24.0f * 5 + 21.0f + 4.0f / 60.0f);

            TimeStamp dayThreeMoonSettingPostLoop, dayThreeMoonDarkPostLoop, dayFourMoonDarkPostLoop,
                dayFourMoonVisiblePostLoop, dayFourMoonSettingPostLoop, dayFourMoonDarkAgainPostLoop,
                dayFiveMoonNightPostLoop;
            dayThreeMoonSettingPostLoop += (24.0f * 28 + 1.0f + 57.0f / 60.0f);
            dayThreeMoonDarkPostLoop += (24.0f * 28 + 1.0f + 59.0f / 60.0f);
            dayFourMoonDarkPostLoop += (24.0f * 28 + 21.0f + 2.0f / 60.0f);
            dayFourMoonVisiblePostLoop += (24.0f * 28 + 21.0f + 4.0f / 60.0f);
            dayFourMoonSettingPostLoop += (24.0f * 29 + 9.0f + 56.0f / 60.0f);
            dayFourMoonDarkAgainPostLoop += (24.0f * 29 + 9.0f + 58.0f / 60.0f);
            dayFiveMoonNightPostLoop += (24.0f * 29 + 21.0f + 4.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            EXPECT_GT(moon.calculateState(dayThreeMoonSetting).mMoonAlpha, 0.0f);
            EXPECT_LE(moon.calculateState(dayThreeMoonDark).mMoonAlpha, 0.0f);
            EXPECT_LE(moon.calculateState(dayFourMoonDark).mMoonAlpha, 0.0f);
            EXPECT_GT(moon.calculateState(dayFourMoonVisible).mMoonAlpha, 0.0f);
            EXPECT_GT(moon.calculateState(dayFourMoonSetting).mMoonAlpha, 0.0f);
            EXPECT_LE(moon.calculateState(dayFourMoonDarkAgain).mMoonAlpha, 0.0f);
            EXPECT_LE(moon.calculateState(dayFiveMoonNight).mMoonAlpha, 0.0f);

            EXPECT_GT(moon.calculateState(dayThreeMoonSettingPostLoop).mMoonAlpha, 0.0f);
            EXPECT_LE(moon.calculateState(dayThreeMoonDarkPostLoop).mMoonAlpha, 0.0f);
            EXPECT_LE(moon.calculateState(dayFourMoonDarkPostLoop).mMoonAlpha, 0.0f);
            EXPECT_GT(moon.calculateState(dayFourMoonVisiblePostLoop).mMoonAlpha, 0.0f);
            EXPECT_GT(moon.calculateState(dayFourMoonSettingPostLoop).mMoonAlpha, 0.0f);
            EXPECT_LE(moon.calculateState(dayFourMoonDarkAgainPostLoop).mMoonAlpha, 0.0f);
            EXPECT_LE(moon.calculateState(dayFiveMoonNightPostLoop).mMoonAlpha, 0.0f);
        }

        // RISE HOUR HANDOVER

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

            // Days 5 and 17, 7:00. This moon's handover should be at 2:59, so it wouldn't become visible at this point
            // unless the moon has correctly switched to Day 5's rise hour.
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

            // Days 6 and 18, 11:57. The fade at 9:59 is after sunrise, so it isn't enough for handover. The moon keeps
            // yesterday's rise hour until it sets.
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

            // Days 5 and 29, 2:59. The moon phase should change at the same moment as the rise hour.
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

        TEST(MWWorldWeatherTest, moonCrossesTheSkyAtAMultipleOfDayLength)
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

            // Days 2 and 14, 14:13. A 14 hour day makes the moon slower than a 12 hour day.
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

            // Days 3 and 15, 10:00. The fade at 6:59 is before the sunrise, so the handover occurs.
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

        // SPEED LIMITS

        TEST(MWWorldWeatherTest, moonSlowerThanTheMinimumCrossesTheSkyAtTheMinimumRate)
        {
            float dailyIncrement = 10.0f;
            float speed = 0.1f;
            float fadeEndAngle = 40.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 0.5f;
            float fadeInStart = 10.0f;
            float fadeInFinish = 11.0f;
            float fadeOutStart = 6.0f;
            float fadeOutFinish = 7.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 35.0f;

            // Day 2, 17:04. The moon needs minimum speed to become visible in the evening.
            TimeStamp timeStampBefore, timeStampAfter;
            timeStampBefore += (24.0f * 2 + 17.0f + 2.0f / 60.0f);
            timeStampAfter += (24.0f * 2 + 17.0f + 5.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            EXPECT_LE(moon.calculateState(timeStampBefore).mMoonAlpha, 0.0f);
            EXPECT_GT(moon.calculateState(timeStampAfter).mMoonAlpha, 0.0f);
        }

        TEST(MWWorldWeatherTest, moonWithNegativeSpeedNeverRises)
        {
            float dailyIncrement = 1.2f;
            float speed = -15.0f;
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

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            // A moon with negative non-zero speed never rises throughout the day.
            for (int minute = 0; minute < 24 * 60; ++minute)
            {
                TimeStamp time;
                time += (24.0f * 3 + minute / 60.0f);
                EXPECT_LE(moon.calculateState(time).mMoonAlpha, 0.0f) << "minute " << minute;
            }
        }

        TEST(MWWorldWeatherTest, moonWithZeroSpeedRisesAtMinimumButNegativeZeroSpeedDoesNot)
        {
            float dailyIncrement = 1.0f;
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

            // Day 3, 02:00. Morrowind treats -0.0 speed differently than 0.0
            TimeStamp time;
            time += (24.0f * 3 + 2.0f);

            MWWorld::MoonModel noSpeed
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, 0.0f,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);
            MWWorld::MoonModel negativeZeroSpeed
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, -0.0f,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            EXPECT_GT(noSpeed.calculateState(time).mMoonAlpha, 0.0f);
            EXPECT_LE(negativeZeroSpeed.calculateState(time).mMoonAlpha, 0.0f);
        }

        // FADE ANGLES

        TEST(MWWorldWeatherTest, moonWithEarlyFadeAngleGreaterThanFadeEndAngleIsNeverInvisible)
        {
            float dailyIncrement = 1.2f;
            float speed = 0.6f;
            float fadeEndAngle = 1.0f;
            float fadeStartAngle = 50.0f;
            float moonShadowEarlyFadeAngle = 10.0f;
            float fadeInStart = 0.0f;
            float fadeInFinish = 0.0f;
            float fadeOutStart = 0.0f;
            float fadeOutFinish = 0.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 50.0f;

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            // An early fade angle > fade end angle leaves no unlit angle, so this moon is never fully transparent.
            for (int minute = 0; minute < 24 * 60; ++minute)
            {
                TimeStamp time;
                time += (24.0f * 3 + minute / 60.0f);
                EXPECT_GT(moon.calculateState(time).mMoonAlpha, 0.0f) << "minute " << minute;
            }
        }

        TEST(MWWorldWeatherTest, moonAtTheHorizonIsInvisibleWithEarlyFadeAngleEqualToFadeEndAngle)
        {
            float dailyIncrement = 0.0f;
            float speed = 0.0f;
            float fadeEndAngle = 0.0f;
            float fadeStartAngle = 0.0f;
            float moonShadowEarlyFadeAngle = 0.0f;
            float fadeInStart = 0.0f;
            float fadeInFinish = 0.0f;
            float fadeOutStart = 0.0f;
            float fadeOutFinish = 0.0f;
            float sunriseHour = 6.0f;
            float sunsetHour = 18.0f;
            float axisOffset = 0.0f;

            // Day 3, 12:00 and 23:30. With every angle at zero the moon can still reach invisibility past the horizon.
            TimeStamp midCrossing, pastTheHorizon;
            midCrossing += (24.0f * 3 + 12.0f);
            pastTheHorizon += (24.0f * 3 + 23.0f + 30.0f / 60.0f);

            MWWorld::MoonModel moon
                = MWWorld::MoonModel(fadeInStart, fadeInFinish, fadeOutStart, fadeOutFinish, axisOffset, speed,
                    dailyIncrement, fadeStartAngle, fadeEndAngle, moonShadowEarlyFadeAngle, sunriseHour, sunsetHour);

            EXPECT_GT(moon.calculateState(midCrossing).mMoonAlpha, 0.0f);
            EXPECT_LE(moon.calculateState(pastTheHorizon).mMoonAlpha, 0.0f);
        }
    }
}
