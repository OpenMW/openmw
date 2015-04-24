#include "importgame.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

void GAME::load(ESM::ESMReader &esm)
{
    esm.getSubNameIs("GMDT");
    esm.getSubHeader();
    if (esm.getSubSize() == 92)
    {
        esm.getExact(&mGMDT, 92);
        mGMDT.mSecundaPhase = 0;
    }
    else if (esm.getSubSize() == 96)
    {
        esm.getT(mGMDT);
    }
    else
        esm.fail("unexpected subrecord size for GAME.GMDT");

    mGMDT.mWeatherTransition &= (0x000000ff);
    mGMDT.mSecundaPhase &= (0x000000ff);
    mGMDT.mMasserPhase &= (0x000000ff);
}

}
