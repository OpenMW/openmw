#include "projectilestate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void BaseProjectileState::save(ESMWriter& esm) const
    {
        esm.writeHNRefId("ID__", mId);
        esm.writeHNT("VEC3", mPosition);
        esm.writeHNT("QUAT", mOrientation);
        esm.writeHNT("ACTO", mActorId);
    }

    void BaseProjectileState::load(ESMReader& esm)
    {
        mId = esm.getHNRefId("ID__");
        esm.getHNT(mPosition, "VEC3");
        esm.getHNT(mOrientation, "QUAT");
        esm.getHNT(mActorId, "ACTO");
    }

    void MagicBoltState::save(ESMWriter& esm) const
    {
        BaseProjectileState::save(esm);

        esm.writeHNRefId("SPEL", mSpellId);
        esm.writeHNT("SPED", mSpeed);
        if (mItem.isSet())
            esm.writeFormId(mItem, true, "ITEM");
    }

    void MagicBoltState::load(ESMReader& esm)
    {
        BaseProjectileState::load(esm);

        mSpellId = esm.getHNRefId("SPEL");
        if (esm.isNextSub("SRCN")) // for backwards compatibility
            esm.skipHSub();
        EffectList().load(esm); // for backwards compatibility
        esm.getHNT(mSpeed, "SPED");
        if (esm.peekNextSub("ITEM"))
            mItem = esm.getFormId(true, "ITEM");
        if (esm.isNextSub("SLOT")) // for backwards compatibility
            esm.skipHSub();
        if (esm.isNextSub("STCK")) // for backwards compatibility
            esm.skipHSub();
        if (esm.isNextSub("SOUN")) // for backwards compatibility
            esm.skipHSub();
    }

    void ProjectileState::save(ESMWriter& esm) const
    {
        BaseProjectileState::save(esm);

        esm.writeHNRefId("BOW_", mBowId);
        esm.writeHNT("VEL_", mVelocity);
        esm.writeHNT("STR_", mAttackStrength);
    }

    void ProjectileState::load(ESMReader& esm)
    {
        BaseProjectileState::load(esm);

        mBowId = esm.getHNRefId("BOW_");
        esm.getHNT(mVelocity, "VEL_");

        mAttackStrength = 1.f;
        esm.getHNOT(mAttackStrength, "STR_");
    }

}
