#ifndef OPENMW_ESSIMPORT_IMPORTPROJ_H
#define OPENMW_ESSIMPORT_IMPORTPROJ_H

#include <components/esm/esmcommon.hpp>
#include <components/esm/vector3.hpp>

#include <cstdint>
#include <vector>

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    struct PROJ
    {

        struct PNAM // 184 bytes
        {
            float mAttackStrength;
            float mSpeed;
            unsigned char mUnknown[4 * 2];
            float mFlightTime;
            int32_t mSplmIndex; // reference to a SPLM record (0 for ballistic projectiles)
            unsigned char mUnknown2[4];
            ESM::Vector3 mVelocity;
            ESM::Vector3 mPosition;
            unsigned char mUnknown3[4 * 9];
            ESM::NAME32 mActorId; // indexed refID (with the exception of "PlayerSaveGame")
            ESM::NAME32 mArrowId;
            ESM::NAME32 mBowId;

            bool isMagic() const { return mSplmIndex != 0; }
        };

        std::vector<PNAM> mProjectiles;

        void load(ESM::ESMReader& esm);
    };

}

#endif
