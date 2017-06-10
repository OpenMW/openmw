#ifndef OPENMW_POSITIONAPI_HPP
#define OPENMW_POSITIONAPI_HPP

#include "../Types.hpp"

#define POSITIONAPI \
    {"GetPos",              PositionFunctions::GetPos},\
    {"GetPosX",             PositionFunctions::GetPosX},\
    {"GetPosY",             PositionFunctions::GetPosY},\
    {"GetPosZ",             PositionFunctions::GetPosZ},\
    \
    {"GetRot",              PositionFunctions::GetRot},\
    {"GetRotX",             PositionFunctions::GetRotX},\
    {"GetRotZ",             PositionFunctions::GetRotZ},\
    \
    {"SetPos",              PositionFunctions::SetPos},\
    {"SetRot",              PositionFunctions::SetRot},\
    \
    {"SendPos",             PositionFunctions::SendPos}


class PositionFunctions
{
public:
    static void GetPos(unsigned short pid, float *x, float *y, float *z) noexcept;
    static double GetPosX(unsigned short pid) noexcept;
    static double GetPosY(unsigned short pid) noexcept;
    static double GetPosZ(unsigned short pid) noexcept;

    static void GetRot(unsigned short pid, float *x, float *y, float *z) noexcept;
    static double GetRotX(unsigned short pid) noexcept;
    static double GetRotZ(unsigned short pid) noexcept;

    static void SetPos(unsigned short pid, double x, double y, double z) noexcept;
    static void SetRot(unsigned short pid, double x, double z) noexcept;

    static void SendPos(unsigned short pid) noexcept;
};

#endif //OPENMW_POSITIONAPI_HPP
