//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_WORLD_HPP
#define OPENMW_WORLD_HPP

#define WORLDFUNCTIONS \
    {"SetHour",              WorldFunctions::SetHour},\
    {"SetMonth",             WorldFunctions::SetMonth},\
    {"SetDay",               WorldFunctions::SetDay}

class WorldFunctions
{
public:
    static void SetHour(unsigned short pid, double hour) noexcept;
    static void SetMonth(unsigned short pid, int month) noexcept;
    static void SetDay(unsigned short pid, int day) noexcept;
};


#endif //OPENMW_WORLD_HPP
