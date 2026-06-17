#include "transport.hpp"

#include <components/debug/debuglog.hpp>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

namespace ESM
{

    void Transport::add(ESMReader& esm)
    {
        if (esm.retSubName().toInt() == fourCC("DODT"))
        {
            Dest dodt;
            esm.getSubComposite(dodt.mPos);
            mList.push_back(std::move(dodt));
        }
        else if (esm.retSubName().toInt() == fourCC("DNAM"))
        {
            std::string name = esm.getHString();
            if (mList.empty())
                Log(Debug::Warning) << "Encountered DNAM record without DODT record, skipped.";
            else
                mList.back().mCellName = std::move(name);
        }
    }

    void Transport::save(ESMWriter& esm) const
    {
        for (const Dest& dest : mList)
        {
            esm.writeNamedComposite("DODT", dest.mPos);
            esm.writeHNOCString("DNAM", dest.mCellName);
        }
    }

}
