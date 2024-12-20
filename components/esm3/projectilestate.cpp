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
        esm.writeFormId(mCaster, true, "ACTO");
    }

    void BaseProjectileState::load(ESMReader& esm)
    {
        mId = esm.getHNRefId("ID__");
        esm.getHNT("VEC3", mPosition.mValues);
        esm.getHNT("QUAT", mOrientation.mValues);
        if (esm.getFormatVersion() <= MaxActorIdSaveGameFormatVersion)
        {
            mCaster.mIndex = -1;
            esm.getHNT(mCaster.mIndex, "ACTO");
        }
        else
            mCaster = esm.getFormId(true, "ACTO");
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
        esm.getHNT(mSpeed, "SPED");
        if (esm.peekNextSub("ITEM"))
            mItem = esm.getFormId(true, "ITEM");
        if (esm.isNextSub("SLOT")) // for backwards compatibility
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
        esm.getHNT("VEL_", mVelocity.mValues);

        mAttackStrength = 1.f;
        esm.getHNOT(mAttackStrength, "STR_");
    }

}
