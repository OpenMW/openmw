#ifndef COMPONENT_ESM_TES3_H
#define COMPONENT_ESM_TES3_H

#include <vector>

#include "components/esm/common.hpp"
#include "components/esm/esmcommon.hpp"
#include "components/esm3/formatversion.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct Data
    {
        ESM::ESMVersion version;
        int32_t type; // 0=esp, 1=esm, 32=ess (unused)
        std::string author; // Author's name
        std::string desc; // File description
        int32_t records; // Number of records
    };

    struct GMDT
    {
        float mCurrentHealth;
        float mMaximumHealth;
        float mHour;
        unsigned char unknown1[12];
        NAME64 mCurrentCell;
        unsigned char unknown2[4];
        NAME32 mPlayerName;
    };

    /// \brief File header record
    struct Header
    {
        // Defines another files (esm or esp) that this file depends upon.
        struct MasterData
        {
            std::string name;
            uint64_t size;
        };

        GMDT mGameData; // Used in .ess savegames only
        std::vector<unsigned char> mSCRD; // Used in .ess savegames only, unknown
        std::vector<unsigned char> mSCRS; // Used in .ess savegames only, screenshot

        Data mData;
        FormatVersion mFormatVersion;
        std::vector<MasterData> mMaster;

        void blank();

        void load(ESMReader& esm);
        void save(ESMWriter& esm);
    };

}

#endif
