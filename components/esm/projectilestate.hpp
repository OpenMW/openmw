#ifndef OPENMW_ESM_PROJECTILESTATE_H
#define OPENMW_ESM_PROJECTILESTATE_H

#include <string>

#include <OgreVector3.h>
#include <OgreQuaternion.h>

#include "effectlist.hpp"

#include "util.hpp"

namespace ESM
{

    // format 0, savegames only

    struct BaseProjectileState
    {
        std::string mId;

        Vector3 mPosition;
        Quaternion mOrientation;

        int mActorId;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };

    struct MagicBoltState : public BaseProjectileState
    {
        std::string mSpellId;
        std::string mSourceName;
        ESM::EffectList mEffects;
        float mSpeed;
        bool mStack;
        std::string mSound;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };

    struct ProjectileState : public BaseProjectileState
    {
        std::string mBowId;
        Vector3 mVelocity;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };

}

#endif
