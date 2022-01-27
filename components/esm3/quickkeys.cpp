#include "quickkeys.hpp"

#include "esmwriter.hpp"
#include "esmreader.hpp"

namespace ESM
{

    void QuickKeys::load(ESMReader &esm)
    {
        if (esm.isNextSub("KEY_"))
            esm.getSubHeader(); // no longer used, because sub-record hierachies do not work properly in esmreader

        while (esm.isNextSub("TYPE"))
        {
            int keyType;
            esm.getHT(keyType);
            std::string id;
            id = esm.getHNString("ID__");

            QuickKey key;
            key.mType = keyType;
            key.mId = id;

            mKeys.push_back(key);

            if (esm.isNextSub("KEY_"))
                esm.getSubHeader();  // no longer used, because sub-record hierachies do not work properly in esmreader
        }
    }

    void QuickKeys::save(ESMWriter &esm) const
    {
        for (std::vector<QuickKey>::const_iterator it = mKeys.begin(); it != mKeys.end(); ++it)
        {
            esm.writeHNT("TYPE", it->mType);
            esm.writeHNString("ID__", it->mId);
        }
    }


}
