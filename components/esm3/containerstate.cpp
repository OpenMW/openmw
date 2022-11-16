#include "containerstate.hpp"

#include <components/esm3/inventorystate.hpp>
#include <components/esm3/objectstate.hpp>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    void ContainerState::load(ESMReader& esm)
    {
        ObjectState::load(esm);

        mInventory.load(esm);
    }

    void ContainerState::save(ESMWriter& esm, bool inInventory) const
    {
        ObjectState::save(esm, inInventory);

        mInventory.save(esm);
    }

}
