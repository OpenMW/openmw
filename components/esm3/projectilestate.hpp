#ifndef OPENMW_ESM_PROJECTILESTATE_H
#define OPENMW_ESM_PROJECTILESTATE_H

#include <osg/Quat>
#include <osg/Vec3f>

#include "components/esm/quaternion.hpp"
#include "components/esm/refid.hpp"
#include "components/esm/vector3.hpp"

#include "refnum.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, savegames only

    struct BaseProjectileState
    {
        RefId mId;

        Vector3 mPosition;
        Quaternion mOrientation;

        int32_t mActorId;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };

    struct MagicBoltState : public BaseProjectileState
    {
        RefId mSpellId;
        float mSpeed;
        RefNum mItem;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };

    struct ProjectileState : public BaseProjectileState
    {
        RefId mBowId;
        Vector3 mVelocity;
        float mAttackStrength;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };

}

#endif
