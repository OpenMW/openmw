//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_WORLD_HPP
#define OPENMW_WORLD_HPP

#define WORLDFUNCTIONS \
    {"setHour",              WorldFunctions::setHour},\
    {"setMonth",             WorldFunctions::setMonth},\
    {"setDay",               WorldFunctions::setDay}

class WorldFunctions
{
public:
    static void setHour(unsigned short pid, double hour) noexcept;
    static void setMonth(unsigned short pid, int month) noexcept;
    static void setDay(unsigned short pid, int day) noexcept;
};


#endif //OPENMW_WORLD_HPP
