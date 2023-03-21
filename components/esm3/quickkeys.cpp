#include "quickkeys.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void QuickKeys::load(ESMReader& esm)
    {
        if (esm.isNextSub("KEY_"))
            esm.getSubHeader(); // no longer used, because sub-record hierachies do not work properly in esmreader

        while (esm.isNextSub("TYPE"))
        {
            QuickKey key;
            esm.getHT(key.mType);
            key.mId = esm.getHNRefId("ID__");

            mKeys.push_back(key);

            if (esm.isNextSub("KEY_"))
                esm.getSubHeader(); // no longer used, because sub-record hierachies do not work properly in esmreader
        }
    }

    void QuickKeys::save(ESMWriter& esm) const
    {
        for (const QuickKey& key : mKeys)
        {
            esm.writeHNT("TYPE", key.mType);
            esm.writeHNRefId("ID__", key.mId);
        }
    }

}
