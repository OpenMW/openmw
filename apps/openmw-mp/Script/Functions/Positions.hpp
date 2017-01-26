#ifndef OPENMW_POSITIONAPI_HPP
#define OPENMW_POSITIONAPI_HPP

#include "../Types.hpp"

#define POSITIONAPI \
    {"GetPos",              PositionFunctions::GetPos},\
    {"GetPosX",             PositionFunctions::GetPosX},\
    {"GetPosY",             PositionFunctions::GetPosY},\
    {"GetPosZ",             PositionFunctions::GetPosZ},\
\
    {"GetAngle",            PositionFunctions::GetAngle},\
    {"GetAngleX",           PositionFunctions::GetAngleX},\
    {"GetAngleY",           PositionFunctions::GetAngleY},\
    {"GetAngleZ",           PositionFunctions::GetAngleZ},\
\
    {"SetPos",              PositionFunctions::SetPos},\
    {"SetAngle",            PositionFunctions::SetAngle},\
\
    {"SendPos",             PositionFunctions::SendPos}


class PositionFunctions
{
public:
    static void GetPos(unsigned short pid, float *x, float *y, float *z) noexcept;
    static double GetPosX(unsigned short pid) noexcept;
    static double GetPosY(unsigned short pid) noexcept;
    static double GetPosZ(unsigned short pid) noexcept;

    static void GetAngle(unsigned short pid, float *x, float *y, float *z) noexcept;
    static double GetAngleX(unsigned short pid) noexcept;
    static double GetAngleY(unsigned short pid) noexcept;
    static double GetAngleZ(unsigned short pid) noexcept;

    static void SetPos(unsigned short pid, double x, double y, double z) noexcept;
    static void SetAngle(unsigned short pid, double x, double y, double z) noexcept;

    static void SendPos(unsigned short pid) noexcept;
};

#endif //OPENMW_POSITIONAPI_HPP
