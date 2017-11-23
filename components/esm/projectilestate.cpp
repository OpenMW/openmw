#include "projectilestate.hpp"

#include "esmwriter.hpp"
#include "esmreader.hpp"

namespace ESM
{

    void BaseProjectileState::save(ESMWriter &esm) const
    {
        esm.writeHNString ("ID__", mId);
        esm.writeHNT ("VEC3", mPosition);
        esm.writeHNT ("QUAT", mOrientation);
        esm.writeHNT ("ACTO", mActorId);
    }

    void BaseProjectileState::load(ESMReader &esm)
    {
        mId = esm.getHNString("ID__");
        esm.getHNT (mPosition, "VEC3");
        esm.getHNT (mOrientation, "QUAT");
        esm.getHNT (mActorId, "ACTO");
    }

    void MagicBoltState::save(ESMWriter &esm) const
    {
        BaseProjectileState::save(esm);

        esm.writeHNString ("SPEL", mSpellId);
        esm.writeHNT ("SPED", mSpeed);
    }

    void MagicBoltState::load(ESMReader &esm)
    {
        BaseProjectileState::load(esm);

        mSpellId = esm.getHNString("SPEL");
        if (esm.isNextSub("SRCN")) // for backwards compatibility
            esm.skipHSub();
        ESM::EffectList().load(esm); // for backwards compatibility
        esm.getHNT (mSpeed, "SPED");
        if (esm.isNextSub("STCK")) // for backwards compatibility
            esm.skipHSub();
        if (esm.isNextSub("SOUN")) // for backwards compatibility
            esm.skipHSub();
    }

    void ProjectileState::save(ESMWriter &esm) const
    {
        BaseProjectileState::save(esm);

        esm.writeHNString ("BOW_", mBowId);
        esm.writeHNT ("VEL_", mVelocity);
        esm.writeHNT ("STR_", mAttackStrength);
    }

    void ProjectileState::load(ESMReader &esm)
    {
        BaseProjectileState::load(esm);

        mBowId = esm.getHNString ("BOW_");
        esm.getHNT (mVelocity, "VEL_");

        mAttackStrength = 1.f;
        esm.getHNOT(mAttackStrength, "STR_");
    }

}
