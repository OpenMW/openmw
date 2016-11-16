//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_TRANSLOCATIONS_HPP
#define OPENMW_TRANSLOCATIONS_HPP

#include "../Types.hpp"

#define TRANSLOCATIONFUNCTIONS \
    {"GetPos",              TranslocationFunctions::GetPos},\
    {"GetPosX",             TranslocationFunctions::GetPosX},\
    {"GetPosY",             TranslocationFunctions::GetPosY},\
    {"GetPosZ",             TranslocationFunctions::GetPosZ},\
\
    {"GetAngle",            TranslocationFunctions::GetAngle},\
    {"GetAngleX",           TranslocationFunctions::GetAngleX},\
    {"GetAngleY",           TranslocationFunctions::GetAngleY},\
    {"GetAngleZ",           TranslocationFunctions::GetAngleZ},\
\
    {"SetPos",              TranslocationFunctions::SetPos},\
    {"SetAngle",            TranslocationFunctions::SetAngle},\
\
    {"GetCell",             TranslocationFunctions::GetCell},\
    {"SetCell",             TranslocationFunctions::SetCell},\
    {"SetExterior",         TranslocationFunctions::SetExterior},\
    {"GetExteriorX",        TranslocationFunctions::GetExteriorX},\
    {"GetExteriorY",        TranslocationFunctions::GetExteriorY},\
    {"IsInExterior",        TranslocationFunctions::IsInExterior},\
\
    {"SendPos",             TranslocationFunctions::SendPos},\
    {"SendCell",            TranslocationFunctions::SendCell}


class TranslocationFunctions
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

    static const char *GetCell(unsigned short pid) noexcept;
    static void SetCell(unsigned short pid, const char *name) noexcept;
    static void SetExterior(unsigned short pid, int x, int y) noexcept;
    static int GetExteriorX(unsigned short pid) noexcept;
    static int GetExteriorY(unsigned short pid) noexcept;
    static bool IsInExterior(unsigned short pid) noexcept;

    static void SendPos(unsigned short pid) noexcept;
    static void SendCell(unsigned short pid) noexcept;
};

#endif //OPENMW_TRANSLOCATIONS_HPP
