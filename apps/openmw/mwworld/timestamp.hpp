#ifndef GAME_MWWORLD_TIMESTAMP_H
#define GAME_MWWORLD_TIMESTAMP_H

namespace MWWorld
{
    /// \brief In-game time stamp
    ///
    /// This class is based on the global variables GameHour and DaysPassed.
    class TimeStamp
    {
            float mHour;
            int mDay;

        public:

            explicit TimeStamp (float hour = 0, int day = 0);
            ///< \oaram hour [0, 23)
            /// \param day >=0

            float getHour() const;

            int getDay() const;

            TimeStamp& operator+= (double hours);
            ///< \param hours >=0
    };

    bool operator== (const TimeStamp& left, const TimeStamp& right);
    bool operator!= (const TimeStamp& left, const TimeStamp& right);

    bool operator< (const TimeStamp& left, const TimeStamp& right);
    bool operator<= (const TimeStamp& left, const TimeStamp& right);

    bool operator> (const TimeStamp& left, const TimeStamp& right);
    bool operator>= (const TimeStamp& left, const TimeStamp& right);

    TimeStamp operator+ (const TimeStamp& stamp, double hours);
    TimeStamp operator+ (double hours, const TimeStamp& stamp);

    double operator- (const TimeStamp& left, const TimeStamp& right);
    ///< Returns the difference between \a left and \a right in in-game hours.
}

#endif
