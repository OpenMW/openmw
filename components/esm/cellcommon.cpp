#include <components/esm/cellcommon.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm4/loadcell.hpp>

namespace ESM
{
    const ESM::CellCommon* CellVariant::getCommon() const
    {
        auto cell3 = getEsm3();
        if (cell3)
            return cell3;
        else
            return getEsm4();
    }
}
