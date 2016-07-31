#include "cellid.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

const std::string ESM::CellId::sDefaultWorldspace = "sys::default";

void ESM::CellId::load (ESMReader &esm)
{
    mWorldspace = esm.getHNString ("SPAC");

    if (esm.isNextSub ("CIDX"))
    {
        esm.getHT (mIndex, 8);
        mPaged = true;
    }
    else
        mPaged = false;
}

void ESM::CellId::save (ESMWriter &esm) const
{
    esm.writeHNString ("SPAC", mWorldspace);

    if (mPaged)
        esm.writeHNT ("CIDX", mIndex, 8);
}

bool ESM::operator== (const CellId& left, const CellId& right)
{
    return left.mWorldspace==right.mWorldspace && left.mPaged==right.mPaged &&
        (!left.mPaged || (left.mIndex.mX==right.mIndex.mX && left.mIndex.mY==right.mIndex.mY));
}

bool ESM::operator!= (const CellId& left, const CellId& right)
{
    return !(left==right);
}

bool ESM::operator < (const CellId& left, const CellId& right)
{
    if (left.mPaged < right.mPaged)
        return true;
    if (left.mPaged > right.mPaged)
        return false;

    if (left.mPaged)
    {
        if (left.mIndex.mX < right.mIndex.mX)
            return true;
        if (left.mIndex.mX > right.mIndex.mX)
            return false;

        if (left.mIndex.mY < right.mIndex.mY)
            return true;
        if (left.mIndex.mY > right.mIndex.mY)
            return false;
    }

    return left.mWorldspace < right.mWorldspace;
}
