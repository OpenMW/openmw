#ifndef COMPONENT_ESM_TES3_H
#define COMPONENT_ESM_TES3_H

#include <vector>

#include "components/esm/esmcommon.hpp"
#include "components/esm3/formatversion.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

#pragma pack(push)
#pragma pack(1)

    struct Data
    {
        /* File format version. This is actually a float, the supported
            versions are 1.2 and 1.3. These correspond to:
            1.2 = 0x3f99999a and 1.3 = 0x3fa66666
        */
        uint32_t version;
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

#pragma pack(pop)

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
