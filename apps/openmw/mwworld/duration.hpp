#ifndef GAME_MWWORLD_DURATION_H
#define GAME_MWWORLD_DURATION_H

#include <cmath>
#include <stdexcept>

namespace MWWorld
{
    inline const double maxFloatHour = static_cast<double>(std::nextafter(24.0f, 0.0f));

    class Duration
    {
    public:
        static Duration fromHours(double hours)
        {
            if (hours < 0)
                throw std::runtime_error("Negative hours is not supported Duration");

            return Duration(
                static_cast<int>(hours / 24), static_cast<float>(std::min(std::fmod(hours, 24), maxFloatHour)));
        }

        int getDays() const { return mDays; }

        float getHours() const { return mHours; }

    private:
        int mDays;
        float mHours;

        explicit Duration(int days, float hours)
            : mDays(days)
            , mHours(hours)
        {
        }
    };
}

#endif
