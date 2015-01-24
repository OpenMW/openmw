#include "importgame.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

void GAME::load(ESM::ESMReader &esm)
{
    esm.getHNT(mGMDT, "GMDT");
    mGMDT.mWeatherTransition &= (0x000000ff);
    mGMDT.mSecundaPhase &= (0x000000ff);
    mGMDT.mMasserPhase &= (0x000000ff);
}

}
