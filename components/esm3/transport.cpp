#include "transport.hpp"

#include <components/debug/debuglog.hpp>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

namespace ESM
{

    void Transport::add(ESMReader &esm)
    {
        if (esm.retSubName().toInt() == fourCC("DODT"))
        {
            Dest dodt;
            esm.getHExact(&dodt.mPos, 24);
            mList.push_back(dodt);
        }
        else if (esm.retSubName().toInt() == fourCC("DNAM"))
        {
            const std::string name = esm.getHString();
            if (mList.empty())
                Log(Debug::Warning) << "Encountered DNAM record without DODT record, skipped.";
            else
                mList.back().mCellName = name;
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
