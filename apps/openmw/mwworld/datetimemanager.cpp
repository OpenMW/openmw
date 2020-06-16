#include "datetimemanager.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "esmstore.hpp"
#include "globals.hpp"
#include "timestamp.hpp"

namespace
{
    static int getDaysPerMonth(int month)
    {
        switch (month)
        {
            case 0: return 31;
            case 1: return 28;
            case 2: return 31;
            case 3: return 30;
            case 4: return 31;
            case 5: return 30;
            case 6: return 31;
            case 7: return 31;
            case 8: return 30;
            case 9: return 31;
            case 10: return 30;
            case 11: return 31;
        }

        throw std::runtime_error ("month out of range");
    }
}

namespace MWWorld
{
    void DateTimeManager::setup(Globals& globalVariables)
    {
        mGameHour = globalVariables["gamehour"].getFloat();
        mDaysPassed = globalVariables["dayspassed"].getInteger();
        mDay = globalVariables["day"].getInteger();
        mMonth = globalVariables["month"].getInteger();
        mYear = globalVariables["year"].getInteger();
        mTimeScale = globalVariables["timescale"].getFloat();
    }

    void DateTimeManager::setHour(double hour)
    {
        if (hour < 0)
            hour = 0;

        int days = static_cast<int>(hour / 24);
        hour = std::fmod(hour, 24);
        mGameHour = static_cast<float>(hour);

        if (days > 0)
            setDay(days + mDay);
    }

    void DateTimeManager::setDay(int day)
    {
        if (day < 1)
            day = 1;

        int month = mMonth;
        while (true)
        {
            int days = getDaysPerMonth(month);
            if (day <= days)
                break;

            if (month < 11)
            {
                ++month;
            }
            else
            {
                month = 0;
                mYear++;
            }

            day -= days;
        }

        mDay = day;
        mMonth = month;
    }

    TimeStamp DateTimeManager::getTimeStamp() const
    {
        return TimeStamp(mGameHour, mDaysPassed);
    }

    float DateTimeManager::getTimeScaleFactor() const
    {
        return mTimeScale;
    }

    ESM::EpochTimeStamp DateTimeManager::getEpochTimeStamp() const
    {
        ESM::EpochTimeStamp timeStamp;
        timeStamp.mGameHour = mGameHour;
        timeStamp.mDay = mDay;
        timeStamp.mMonth = mMonth;
        timeStamp.mYear = mYear;
        return timeStamp;
    }

    void DateTimeManager::setMonth(int month)
    {
        if (month < 0)
            month = 0;

        int years = month / 12;
        month = month % 12;

        int days = getDaysPerMonth(month);
        if (mDay > days)
            mDay = days;

        mMonth = month;

        if (years > 0)
            mYear += years;
    }

    void DateTimeManager::advanceTime(double hours, Globals& globalVariables)
    {
        hours += mGameHour;
        setHour(hours);

        int days = static_cast<int>(hours / 24);
        if (days > 0)
            mDaysPassed += days;

        globalVariables["gamehour"].setFloat(mGameHour);
        globalVariables["dayspassed"].setInteger(mDaysPassed);
        globalVariables["day"].setInteger(mDay);
        globalVariables["month"].setInteger(mMonth);
        globalVariables["year"].setInteger(mYear);
    }

    std::string DateTimeManager::getMonthName(int month) const
    {
        if (month == -1)
            month = mMonth;

        const int months = 12;
        if (month < 0 || month >= months)
            return std::string();

        static const char *monthNames[months] =
        {
            "sMonthMorningstar", "sMonthSunsdawn", "sMonthFirstseed", "sMonthRainshand",
            "sMonthSecondseed", "sMonthMidyear", "sMonthSunsheight", "sMonthLastseed",
            "sMonthHeartfire", "sMonthFrostfall", "sMonthSunsdusk", "sMonthEveningstar"
        };

        const ESM::GameSetting *setting = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(monthNames[month]);
        return setting->mValue.getString();
    }

    bool DateTimeManager::updateGlobalFloat(const std::string& name, float value)
    {
        if (name=="gamehour")
        {
            setHour(value);
            return true;
        }
        else if (name=="day")
        {
            setDay(static_cast<int>(value));
            return true;
        }
        else if (name=="month")
        {
            setMonth(static_cast<int>(value));
            return true;
        }
        else if (name=="year")
        {
            mYear = static_cast<int>(value);
        }
        else if (name=="timescale")
        {
            mTimeScale = value;
        }
        else if (name=="dayspassed")
        {
            mDaysPassed = static_cast<int>(value);
        }

        return false;
    }

    bool DateTimeManager::updateGlobalInt(const std::string& name, int value)
    {
        if (name=="gamehour")
        {
            setHour(static_cast<float>(value));
            return true;
        }
        else if (name=="day")
        {
            setDay(value);
            return true;
        }
        else if (name=="month")
        {
            setMonth(value);
            return true;
        }
        else if (name=="year")
        {
            mYear = value;
        }
        else if (name=="timescale")
        {
            mTimeScale = static_cast<float>(value);
        }
        else if (name=="dayspassed")
        {
            mDaysPassed = value;
        }

        return false;
    }
}
