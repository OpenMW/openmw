#ifndef OPENMW_ESM_PROJECTILESTATE_H
#define OPENMW_ESM_PROJECTILESTATE_H

#include <string>

#include <OgreVector3.h>
#include <OgreQuaternion.h>

#include "effectlist.hpp"

namespace ESM
{

    // format 0, savegames only

    struct Quaternion
    {
        float mValues[4];

        Quaternion() {}
        Quaternion (Ogre::Quaternion q)
        {
            mValues[0] = q.w;
            mValues[1] = q.x;
            mValues[2] = q.y;
            mValues[3] = q.z;
        }

        operator Ogre::Quaternion () const
        {
            return Ogre::Quaternion(mValues[0], mValues[1], mValues[2], mValues[3]);
        }
    };

    struct Vector3
    {
        float mValues[3];

        Vector3() {}
        Vector3 (Ogre::Vector3 v)
        {
            mValues[0] = v.x;
            mValues[1] = v.y;
            mValues[2] = v.z;
        }

        operator Ogre::Vector3 () const
        {
            return Ogre::Vector3(&mValues[0]);
        }
    };

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
