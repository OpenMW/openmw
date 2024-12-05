#include "quickkeys.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void QuickKeys::load(ESMReader& esm)
    {
        while (esm.isNextSub("TYPE"))
        {
            QuickKey key;
            esm.getHT(key.mType);
            key.mId = esm.getHNRefId("ID__");

            mKeys.push_back(key);
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
