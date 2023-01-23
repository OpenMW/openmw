#include <components/esm/esm3esm4bridge.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm4/loadcell.hpp>

namespace ESM
{
    const ESM::CellCommon* CellVariant::getCommon() const
    {
        if (isEsm4())
            return &getEsm4();
        else
            return &getEsm3();
    }

    const ESM4::Cell& CellVariant::getEsm4() const
    {
        auto cell4 = std::get<0>(mVariant);
        if (!cell4)
            throw std::runtime_error("invalid variant acess");
        return *cell4;
    }

    const ESM::Cell& CellVariant::getEsm3() const
    {
        auto cell = std::get<1>(mVariant);
        if (!cell)
            throw std::runtime_error("invalid variant acess");
        return *cell;
    }
}
