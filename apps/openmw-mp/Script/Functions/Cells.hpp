#ifndef OPENMW_CELLAPI_HPP
#define OPENMW_CELLAPI_HPP

#include "../Types.hpp"

#define CELLAPI \
    {"GetCell",             CellFunctions::GetCell},\
    {"SetCell",             CellFunctions::SetCell},\
    {"SetExterior",         CellFunctions::SetExterior},\
    {"GetExteriorX",        CellFunctions::GetExteriorX},\
    {"GetExteriorY",        CellFunctions::GetExteriorY},\
    {"IsInExterior",        CellFunctions::IsInExterior},\
\
    {"SendCell",            CellFunctions::SendCell}


class CellFunctions
{
public:
    static const char *GetCell(unsigned short pid) noexcept;
    static void SetCell(unsigned short pid, const char *name) noexcept;
    static void SetExterior(unsigned short pid, int x, int y) noexcept;
    static int GetExteriorX(unsigned short pid) noexcept;
    static int GetExteriorY(unsigned short pid) noexcept;
    static bool IsInExterior(unsigned short pid) noexcept;

    static void SendCell(unsigned short pid) noexcept;
};

#endif //OPENMW_CELLAPI_HPP
