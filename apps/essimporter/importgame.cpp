#include "importgame.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void GAME::load(ESM::ESMReader& esm)
    {
        esm.getSubNameIs("GMDT");
        esm.getSubHeader();
        bool hasSecundaPhase = esm.getSubSize() == 96;
        esm.getT(mGMDT.mCellName);
        esm.getT(mGMDT.mFogColour);
        esm.getT(mGMDT.mFogDensity);
        esm.getT(mGMDT.mCurrentWeather);
        esm.getT(mGMDT.mNextWeather);
        esm.getT(mGMDT.mWeatherTransition);
        esm.getT(mGMDT.mTimeOfNextTransition);
        esm.getT(mGMDT.mMasserPhase);
        if (hasSecundaPhase)
            esm.getT(mGMDT.mSecundaPhase);

        mGMDT.mWeatherTransition &= (0x000000ff);
        mGMDT.mSecundaPhase &= (0x000000ff);
        mGMDT.mMasserPhase &= (0x000000ff);
    }

}
