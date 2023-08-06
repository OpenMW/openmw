#ifndef GAME_MWWORLD_DATETIMEMANAGER_H
#define GAME_MWWORLD_DATETIMEMANAGER_H

#include <string_view>

#include "globalvariablename.hpp"

namespace ESM
{
    struct EpochTimeStamp;
}

namespace MWWorld
{
    class Globals;
    class TimeStamp;
    class World;

    class DateTimeManager
    {
    public:
        // Game time.
        // Note that game time generally goes faster than the simulation time.
        std::string_view getMonthName(int month = -1) const; // -1: current month
        TimeStamp getTimeStamp() const;
        ESM::EpochTimeStamp getEpochTimeStamp() const;
        double getGameTime() const { return (static_cast<double>(mDaysPassed) * 24 + mGameHour) * 3600.0; }
        float getGameTimeScale() const { return mGameTimeScale; }
        void setGameTimeScale(float scale); // game time to simulation time ratio

        // Simulation time (the number of seconds passed from the beginning of the game).
        double getSimulationTime() const { return mSimulationTime; }
        void setSimulationTime(double t) { mSimulationTime = t; }
        float getSimulationTimeScale() const { return mSimulationTimeScale; }
        void setSimulationTimeScale(float scale); // simulation time to real time ratio

    private:
        friend class World;
        void setup(Globals& globalVariables);
        bool updateGlobalInt(GlobalVariableName name, int value);
        bool updateGlobalFloat(GlobalVariableName name, float value);
        void advanceTime(double hours, Globals& globalVariables);

        void setHour(double hour);
        void setDay(int day);
        void setMonth(int month);

        int mDaysPassed = 0;
        int mDay = 0;
        int mMonth = 0;
        int mYear = 0;
        float mGameHour = 0.f;
        float mGameTimeScale = 0.f;
        float mSimulationTimeScale = 1.0;
        double mSimulationTime = 0.0;
    };
}

#endif
