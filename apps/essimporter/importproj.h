#ifndef OPENMW_ESSIMPORT_IMPORTPROJ_H
#define OPENMW_ESSIMPORT_IMPORTPROJ_H

#include <vector>
#include <components/esm/esmcommon.hpp>
#include <components/esm/util.hpp>

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

struct PROJ
{

#pragma pack(push)
#pragma pack(1)
    struct PNAM // 184 bytes
    {
        float mAttackStrength;
        float mSpeed;
        unsigned char mUnknown[4*2];
        float mFlightTime;
        int mSplmIndex; // reference to a SPLM record (0 for ballistic projectiles)
        unsigned char mUnknown2[4];
        ESM::Vector3 mVelocity;
        ESM::Vector3 mPosition;
        unsigned char mUnknown3[4*9];
        ESM::NAME32 mActorId; // indexed refID (with the exception of "PlayerSaveGame")
        ESM::NAME32 mArrowId;
        ESM::NAME32 mBowId;

        bool isMagic() const { return mSplmIndex != 0; }
    };
#pragma pack(pop)

    std::vector<PNAM> mProjectiles;

    void load(ESM::ESMReader& esm);
};

}

#endif
