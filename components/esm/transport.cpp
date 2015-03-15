#include "transport.hpp"

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>

namespace ESM
{

    void Transport::add(ESMReader &esm)
    {
        if (esm.retSubName().val == ESM::FourCC<'D','O','D','T'>::value)
        {
            Dest dodt;
            esm.getHExact(&dodt.mPos, 24);
            mList.push_back(dodt);
        }
        else if (esm.retSubName().val == ESM::FourCC<'D','N','A','M'>::value)
        {
            mList.back().mCellName = esm.getHString();
        }
    }

    void Transport::save(ESMWriter &esm) const
    {
        typedef std::vector<Dest>::const_iterator DestIter;
        for (DestIter it = mList.begin(); it != mList.end(); ++it)
        {
            esm.writeHNT("DODT", it->mPos, sizeof(it->mPos));
            esm.writeHNOCString("DNAM", it->mCellName);
        }
    }

}
