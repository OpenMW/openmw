//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_TRANSLOCATIONS_HPP
#define OPENMW_TRANSLOCATIONS_HPP

#include "../Types.hpp"

#define TRANSLOCATIONFUNCTIONS \
    {"getPos",              TranslocationFunctions::getPos},\
    {"getPosX",             TranslocationFunctions::getPosX},\
    {"getPosY",             TranslocationFunctions::getPosY},\
    {"getPosZ",             TranslocationFunctions::getPosZ},\
\
    {"getAngle",            TranslocationFunctions::getAngle},\
    {"getAngleX",           TranslocationFunctions::getAngleX},\
    {"getAngleY",           TranslocationFunctions::getAngleY},\
    {"getAngleZ",           TranslocationFunctions::getAngleZ},\
\
    {"setPos",              TranslocationFunctions::setPos},\
    {"setAngle",            TranslocationFunctions::setAngle},\
\
    {"getCell",             TranslocationFunctions::getCell},\
    {"setCell",             TranslocationFunctions::setCell},\
    {"setExterior",         TranslocationFunctions::setExterior},\
    {"getExteriorX",        TranslocationFunctions::getExteriorX},\
    {"getExteriorY",        TranslocationFunctions::getExteriorY},\
    {"isInExterior",        TranslocationFunctions::isInExterior},\
\
    {"sendPos",             TranslocationFunctions::sendPos},\
    {"sendCell",            TranslocationFunctions::sendCell}


class TranslocationFunctions
{
public:
    static void getPos(unsigned short pid, float *x, float *y, float *z) noexcept;
    static double getPosX(unsigned short pid) noexcept;
    static double getPosY(unsigned short pid) noexcept;
    static double getPosZ(unsigned short pid) noexcept;

    static void getAngle(unsigned short pid, float *x, float *y, float *z) noexcept;
    static double getAngleX(unsigned short pid) noexcept;
    static double getAngleY(unsigned short pid) noexcept;
    static double getAngleZ(unsigned short pid) noexcept;

    static void setPos(unsigned short pid, double x, double y, double z) noexcept;
    static void setAngle(unsigned short pid, double x, double y, double z) noexcept;

    static const char *getCell(unsigned short pid) noexcept;
    static void setCell(unsigned short pid, const char *name) noexcept;
    static void setExterior(unsigned short pid, int x, int y) noexcept;
    static int getExteriorX(unsigned short pid) noexcept;
    static int getExteriorY(unsigned short pid) noexcept;
    static bool isInExterior(unsigned short pid) noexcept;

    static void sendPos(unsigned short pid) noexcept;
    static void sendCell(unsigned short pid) noexcept;
};

#endif //OPENMW_TRANSLOCATIONS_HPP
