#include "importgame.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

void GAME::load(ESM::ESMReader &esm)
{
    esm.getHNT(mGMDT, "GMDT");
}

}
