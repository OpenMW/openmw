#include "cellref.hpp"

#include <components/debug/debuglog.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    namespace
    {
        template <bool load>
        void loadIdImpl(ESMReader& esm, bool wideRefNum, CellRef& cellRef)
        {
            // According to Hrnchamd, this does not belong to the actual ref. Instead, it is a marker indicating that
            // the following refs are part of a "temp refs" section. A temp ref is not being tracked by the moved references system.
            // Its only purpose is a performance optimization for "immovable" things. We don't need this, and it's problematic anyway,
            // because any item can theoretically be moved by a script.
            if (esm.isNextSub("NAM0"))
                esm.skipHSub();

            if constexpr (load)
            {
                cellRef.blank();
                cellRef.mRefNum.load (esm, wideRefNum);
                cellRef.mRefID = esm.getHNOString("NAME");

                if (cellRef.mRefID.empty())
                    Log(Debug::Warning) << "Warning: got CellRef with empty RefId in " << esm.getName() << " 0x" << std::hex << esm.getFileOffset();
            }
            else
            {
                RefNum {}.load(esm, wideRefNum);
                esm.skipHNOString("NAME");
            }
        }

        template <bool load>
        void loadDataImpl(ESMReader &esm, bool &isDeleted, CellRef& cellRef)
        {
            const auto getHStringOrSkip = [&] (std::string& value)
            {
                if constexpr (load)
                    value = esm.getHString();
                else
                    esm.skipHString();
            };

            const auto getHTOrSkip = [&] (auto& value)
            {
                if constexpr (load)
                    esm.getHT(value);
                else
                    esm.skipHT<std::decay_t<decltype(value)>>();
            };

            if constexpr (load)
                isDeleted = false;

            bool isLoaded = false;
            while (!isLoaded && esm.hasMoreSubs())
            {
                esm.getSubName();
                switch (esm.retSubName().toInt())
                {
                    case fourCC("UNAM"):
                        getHTOrSkip(cellRef.mReferenceBlocked);
                        break;
                    case fourCC("XSCL"):
                        getHTOrSkip(cellRef.mScale);
                        if constexpr (load)
                            cellRef.mScale = std::clamp(cellRef.mScale, 0.5f, 2.0f);
                        break;
                    case fourCC("ANAM"):
                        getHStringOrSkip(cellRef.mOwner);
                        break;
                    case fourCC("BNAM"):
                        getHStringOrSkip(cellRef.mGlobalVariable);
                        break;
                    case fourCC("XSOL"):
                        getHStringOrSkip(cellRef.mSoul);
                        break;
                    case fourCC("CNAM"):
                        getHStringOrSkip(cellRef.mFaction);
                        break;
                    case fourCC("INDX"):
                        getHTOrSkip(cellRef.mFactionRank);
                        break;
                    case fourCC("XCHG"):
                        getHTOrSkip(cellRef.mEnchantmentCharge);
                        break;
                    case fourCC("INTV"):
                        getHTOrSkip(cellRef.mChargeInt);
                        break;
                    case fourCC("NAM9"):
                        getHTOrSkip(cellRef.mGoldValue);
                        break;
                    case fourCC("DODT"):
                        getHTOrSkip(cellRef.mDoorDest);
                        if constexpr (load)
                            cellRef.mTeleport = true;
                        break;
                    case fourCC("DNAM"):
                        getHStringOrSkip(cellRef.mDestCell);
                        break;
                    case fourCC("FLTV"):
                        getHTOrSkip(cellRef.mLockLevel);
                        break;
                    case fourCC("KNAM"):
                        getHStringOrSkip(cellRef.mKey);
                        break;
                    case fourCC("TNAM"):
                        getHStringOrSkip(cellRef.mTrap);
                        break;
                    case fourCC("DATA"):
                        if constexpr (load)
                            esm.getHTSized<24>(cellRef.mPos);
                        else
                            esm.skipHTSized<24, decltype(cellRef.mPos)>();
                        break;
                    case fourCC("NAM0"):
                    {
                        esm.skipHSub();
                        break;
                    }
                    case SREC_DELE:
                        esm.skipHSub();
                        if constexpr (load)
                            isDeleted = true;
                        break;
                    default:
                        esm.cacheSubName();
                        isLoaded = true;
                        break;
                }
            }

            if constexpr (load)
            {
                if (cellRef.mLockLevel == 0 && !cellRef.mKey.empty())
                {
                    cellRef.mLockLevel = UnbreakableLock;
                    cellRef.mTrap.clear();
                }
            }
        }
    }

void RefNum::load(ESMReader& esm, bool wide, NAME tag)
{
    if (wide)
        esm.getHNTSized<8>(*this, tag);
    else
        esm.getHNT(mIndex, tag);
}

void RefNum::save(ESMWriter &esm, bool wide, NAME tag) const
{
    if (wide)
        esm.writeHNT (tag, *this, 8);
    else
    {
        if (isSet() && !hasContentFile())
            Log(Debug::Error) << "Generated RefNum can not be saved in 32bit format";
        int refNum = (mIndex & 0xffffff) | ((hasContentFile() ? mContentFile : 0xff)<<24);
        esm.writeHNT (tag, refNum, 4);
    }
}

void CellRef::load (ESMReader& esm, bool &isDeleted, bool wideRefNum)
{
    loadId(esm, wideRefNum);
    loadData(esm, isDeleted);
}

void CellRef::loadId (ESMReader& esm, bool wideRefNum)
{
    loadIdImpl<true>(esm, wideRefNum, *this);
}

void CellRef::loadData(ESMReader &esm, bool &isDeleted)
{
    loadDataImpl<true>(esm, isDeleted, *this);
}

void CellRef::save (ESMWriter &esm, bool wideRefNum, bool inInventory, bool isDeleted) const
{
    mRefNum.save (esm, wideRefNum);

    esm.writeHNCString("NAME", mRefID);

    if (isDeleted) {
        esm.writeHNString("DELE", "", 3);
        return;
    }

    if (mScale != 1.0) {
        esm.writeHNT("XSCL", std::clamp(mScale, 0.5f, 2.0f));
    }

    if (!inInventory)
        esm.writeHNOCString("ANAM", mOwner);

    esm.writeHNOCString("BNAM", mGlobalVariable);
    esm.writeHNOCString("XSOL", mSoul);

    if (!inInventory)
    {
        esm.writeHNOCString("CNAM", mFaction);
        if (mFactionRank != -2)
        {
            esm.writeHNT("INDX", mFactionRank);
        }
    }

    if (mEnchantmentCharge != -1)
        esm.writeHNT("XCHG", mEnchantmentCharge);

    if (mChargeInt != -1)
        esm.writeHNT("INTV", mChargeInt);

    if (mGoldValue > 1)
        esm.writeHNT("NAM9", mGoldValue);

    if (!inInventory && mTeleport)
    {
        esm.writeHNT("DODT", mDoorDest);
        esm.writeHNOCString("DNAM", mDestCell);
    }

    if (!inInventory && mLockLevel != 0) {
        esm.writeHNT("FLTV", mLockLevel);
    }

    if (!inInventory)
    {
        esm.writeHNOCString ("KNAM", mKey);
        esm.writeHNOCString ("TNAM", mTrap);
    }

    if (mReferenceBlocked != -1)
        esm.writeHNT("UNAM", mReferenceBlocked);

    if (!inInventory)
        esm.writeHNT("DATA", mPos, 24);
}

void CellRef::blank()
{
    mRefNum.unset();
    mRefID.clear();
    mScale = 1;
    mOwner.clear();
    mGlobalVariable.clear();
    mSoul.clear();
    mFaction.clear();
    mFactionRank = -2;
    mChargeInt = -1;
    mChargeIntRemainder = 0.0f;
    mEnchantmentCharge = -1;
    mGoldValue = 1;
    mDestCell.clear();
    mLockLevel = 0;
    mKey.clear();
    mTrap.clear();
    mReferenceBlocked = -1;
    mTeleport = false;

    for (int i=0; i<3; ++i)
    {
        mDoorDest.pos[i] = 0;
        mDoorDest.rot[i] = 0;
        mPos.pos[i] = 0;
        mPos.rot[i] = 0;
    }
}

void skipLoadCellRef(ESMReader& esm, bool wideRefNum)
{
    CellRef cellRef;
    loadIdImpl<false>(esm, wideRefNum, cellRef);
    bool isDeleted;
    loadDataImpl<false>(esm, isDeleted, cellRef);
}

}
