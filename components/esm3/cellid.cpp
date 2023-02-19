#include "cellid.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include <components/misc/algorithm.hpp>

namespace ESM
{

    const std::string CellId::sDefaultWorldspace = "sys::default";

    void CellId::load(ESMReader& esm)
    {
        mWorldspace = esm.getHNString("SPAC");

        if (esm.isNextSub("CIDX"))
        {
            esm.getHTSized<8>(mIndex);
            mPaged = true;
        }
        else
        {
            mPaged = false;
            mIndex.mX = 0;
            mIndex.mY = 0;
        }
    }

    void CellId::save(ESMWriter& esm) const
    {
        esm.writeHNString("SPAC", mWorldspace);

        if (mPaged)
            esm.writeHNT("CIDX", mIndex, 8);
    }

    ESM::RefId CellId::getCellRefId() const
    {
        if (mPaged)
        {
            return ESM::RefId::stringRefId("#" + std::to_string(mIndex.mX) + "," + std::to_string(mIndex.mY));
        }
        else
        {
            return ESM::RefId::stringRefId(mWorldspace);
        }
    }

    CellId CellId::extractFromRefId(const ESM::RefId& id)
    {
        // This is bad and that code should not be merged.
        const std::string& idString = id.getRefIdString();
        CellId out;
        if (idString[0] == '#' && idString.find(',')) // That is an exterior cell Id
        {
            int x, y;
            std::stringstream stringStream = std::stringstream(idString);
            char sharp = '#';
            char comma = ',';
            stringStream >> sharp >> x >> comma >> y;
            out.mPaged = true;
            out.mIndex = { x, y };
        }
        else
        {
            out.mPaged = false;
            out.mWorldspace = Misc::StringUtils::lowerCase(idString);
        }

        return out;
    }

    bool operator==(const CellId& left, const CellId& right)
    {
        return left.mWorldspace == right.mWorldspace && left.mPaged == right.mPaged
            && (!left.mPaged || (left.mIndex.mX == right.mIndex.mX && left.mIndex.mY == right.mIndex.mY));
    }

    bool operator!=(const CellId& left, const CellId& right)
    {
        return !(left == right);
    }

    bool operator<(const CellId& left, const CellId& right)
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

}
