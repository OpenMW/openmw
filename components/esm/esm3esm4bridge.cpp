#include <components/esm/esm3esm4bridge.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm4/loadcell.hpp>

namespace ESM
{
    const ESM4::Cell& CellVariant::getEsm4() const
    {
        auto cell4 = std::get<const ESM4::Cell*>(mVariant);
        return *cell4;
    }

    const ESM::Cell& CellVariant::getEsm3() const
    {
        auto cell = std::get<const ESM::Cell*>(mVariant);
        return *cell;
    }
}
