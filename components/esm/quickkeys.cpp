#include "quickkeys.hpp"

#include "esmwriter.hpp"
#include "esmreader.hpp"

namespace ESM
{

    void QuickKeys::load(ESMReader &esm)
    {
        while (esm.isNextSub("KEY_"))
        {
            esm.getSubHeader();
            int keyType;
            esm.getHNT(keyType, "TYPE");
            std::string id;
            id = esm.getHNString("ID__");

            QuickKey key;
            key.mType = keyType;
            key.mId = id;

            mKeys.push_back(key);
        }
    }

    void QuickKeys::save(ESMWriter &esm) const
    {
        const std::string recKey = "KEY_";

        for (std::vector<QuickKey>::const_iterator it = mKeys.begin(); it != mKeys.end(); ++it)
        {
            esm.startSubRecord(recKey);

            esm.writeHNT("TYPE", it->mType);
            esm.writeHNString("ID__", it->mId);

            esm.endRecord(recKey);
        }
    }


}
