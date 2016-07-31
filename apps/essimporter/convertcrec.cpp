#include "convertcrec.hpp"

#include "convertinventory.hpp"

namespace ESSImport
{

    void convertCREC(const CREC &crec, ESM::CreatureState &state)
    {
        convertInventory(crec.mInventory, state.mInventory);
    }

}
