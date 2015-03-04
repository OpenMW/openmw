
#include "player.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::Player::load (ESMReader &esm)
{
    mObject.mRef.loadId(esm, true);
    mObject.load (esm);

    mCellId.load (esm);

    esm.getHNT (mLastKnownExteriorPosition, "LKEP", 12);

    if (esm.isNextSub ("MARK"))
    {
        mHasMark = true;
        esm.getHT (mMarkedPosition, 24);
        mMarkedCell.load (esm);
    }
    else
        mHasMark = false;

    mAutoMove = 0;
    esm.getHNOT (mAutoMove, "AMOV");

    mBirthsign = esm.getHNString ("SIGN");

    mCurrentCrimeId = -1;
    esm.getHNOT (mCurrentCrimeId, "CURD");
    mPaidCrimeId = -1;
    esm.getHNOT (mPaidCrimeId, "PAYD");
}

void ESM::Player::save (ESMWriter &esm) const
{
    mObject.save (esm);

    mCellId.save (esm);

    esm.writeHNT ("LKEP", mLastKnownExteriorPosition, 12);

    if (mHasMark)
    {
        esm.writeHNT ("MARK", mMarkedPosition, 24);
        mMarkedCell.save (esm);
    }

    if (mAutoMove)
        esm.writeHNT ("AMOV", mAutoMove);

    esm.writeHNString ("SIGN", mBirthsign);

    esm.writeHNT ("CURD", mCurrentCrimeId);
    esm.writeHNT ("PAYD", mPaidCrimeId);
}
