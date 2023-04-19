#include "cell.hpp"

#include <sstream>

void CSMWorld::Cell::load(ESM::ESMReader& esm, bool& isDeleted)
{
    ESM::Cell::load(esm, isDeleted, false);

    mId = ESM::RefId::stringRefId(ESM::Cell::mId.toString());
}
