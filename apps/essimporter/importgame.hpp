#ifndef OPENMW_ESSIMPORT_GAME_H
#define OPENMW_ESSIMPORT_GAME_H

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    /// Weather data
    struct GAME
    {
        struct GMDT
        {
           char mCellName[64];
           int mFogColour;
           float mFogDensity;
           int mCurrentWeather, mNextWeather;
           int mWeatherTransition; // 0-100 transition between weathers, top 3 bytes may be garbage
           float mTimeOfNextTransition; // weather changes when gamehour == timeOfNextTransition
           int mMasserPhase, mSecundaPhase; // top 3 bytes may be garbage
        };

        GMDT mGMDT;

        void load(ESM::ESMReader& esm);
    };

}

#endif
