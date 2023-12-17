#include "importscri.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void SCRI::load(ESM::ESMReader& esm)
    {
        mScript = esm.getHNOString("SCRI");

        int32_t numShorts = 0, numLongs = 0, numFloats = 0;
        if (esm.isNextSub("SLCS"))
        {
            esm.getSubHeader();
            esm.getT(numShorts);
            esm.getT(numLongs);
            esm.getT(numFloats);
        }

        if (esm.isNextSub("SLSD"))
        {
            esm.getSubHeader();
            for (int i = 0; i < numShorts; ++i)
            {
                int16_t val;
                esm.getT(val);
                mShorts.push_back(val);
            }
        }
        // I haven't seen Longs in a save file yet, but SLLD would make sense for the name
        // TODO: test this
        if (esm.isNextSub("SLLD"))
        {
            esm.getSubHeader();
            for (int i = 0; i < numLongs; ++i)
            {
                int32_t val;
                esm.getT(val);
                mLongs.push_back(val);
            }
        }
        if (esm.isNextSub("SLFD"))
        {
            esm.getSubHeader();
            for (int i = 0; i < numFloats; ++i)
            {
                float val;
                esm.getT(val);
                mFloats.push_back(val);
            }
        }
    }

}
