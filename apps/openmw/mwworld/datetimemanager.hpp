#ifndef GAME_MWWORLD_DATETIMEMANAGER_H
#define GAME_MWWORLD_DATETIMEMANAGER_H

#include <string>

namespace ESM
{
    struct EpochTimeStamp;
}

namespace MWWorld
{
    class Globals;
    class TimeStamp;

    class DateTimeManager
    {
        int mDaysPassed = 0;
        int mDay = 0;
        int mMonth = 0;
        int mYear = 0;
        float mGameHour = 0.f;
        float mTimeScale = 0.f;

        void setHour(double hour);
        void setDay(int day);
        void setMonth(int month);

    public:
        std::string getMonthName(int month) const;
        TimeStamp getTimeStamp() const;
        ESM::EpochTimeStamp getEpochTimeStamp() const;
        float getTimeScaleFactor() const;

        void advanceTime(double hours, Globals& globalVariables);

        void setup(Globals& globalVariables);
        bool updateGlobalInt(const std::string& name, int value);
        bool updateGlobalFloat(const std::string& name, float value);
    };
}

#endif
