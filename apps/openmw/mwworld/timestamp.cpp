#include "timestamp.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

#include <components/esm/defs.hpp>

#include "duration.hpp"

namespace MWWorld
{
    TimeStamp::TimeStamp (float hour, int day)
    : mHour (hour), mDay (day)
    {
        if (hour < 0 || hour >= 24)
            throw std::runtime_error("invalid time stamp hour: " + std::to_string(hour));
    }

    float TimeStamp::getHour() const
    {
        return mHour;
    }

    int TimeStamp::getDay() const
    {
        return mDay;
    }

    TimeStamp& TimeStamp::operator+= (double hours)
    {
        if (hours<0)
            throw std::runtime_error ("can't move time stamp backwards in time");

        const Duration duration = Duration::fromHours(mHour + hours);

        mHour = duration.getHours();
        mDay += duration.getDays();

        return *this;
    }

    bool operator== (const TimeStamp& left, const TimeStamp& right)
    {
        return left.getHour()==right.getHour() && left.getDay()==right.getDay();
    }

    bool operator!= (const TimeStamp& left, const TimeStamp& right)
    {
        return !(left==right);
    }

    bool operator< (const TimeStamp& left, const TimeStamp& right)
    {
        if (left.getDay()<right.getDay())
            return true;

        if (left.getDay()>right.getDay())
            return false;

        return left.getHour()<right.getHour();
    }

    bool operator<= (const TimeStamp& left, const TimeStamp& right)
    {
        return left<right || left==right;
    }

    bool operator> (const TimeStamp& left, const TimeStamp& right)
    {
        return !(left<=right);
    }

    bool operator>= (const TimeStamp& left, const TimeStamp& right)
    {
        return !(left<right);
    }

    TimeStamp operator+ (const TimeStamp& stamp, double hours)
    {
        return TimeStamp (stamp) += hours;
    }

    TimeStamp operator+ (double hours, const TimeStamp& stamp)
    {
        return TimeStamp (stamp) += hours;
    }

    double operator- (const TimeStamp& left, const TimeStamp& right)
    {
        if (left<right)
            return -(right-left);

        int days = left.getDay() - right.getDay();

        double hours = 0;

        if (left.getHour()<right.getHour())
        {
            hours = 24-right.getHour()+left.getHour();
            --days;
        }
        else
        {
            hours = left.getHour()-right.getHour();
        }

        return hours + 24*days;
    }

    ESM::TimeStamp TimeStamp::toEsm() const
    {
        ESM::TimeStamp ret;
        ret.mDay = mDay;
        ret.mHour = mHour;
        return ret;
    }

    TimeStamp::TimeStamp(const ESM::TimeStamp &esm)
    {
        mDay = esm.mDay;
        mHour = esm.mHour;
    }
}
