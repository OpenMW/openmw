#include "importscri.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void SCRI::load(ESM::ESMReader &esm)
    {
        mScript = esm.getHNOString("SCRI");

        int numShorts = 0, numLongs = 0, numFloats = 0;
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
            for (int i=0; i<numShorts; ++i)
            {
                short val;
                esm.getT(val);
                mShorts.push_back(val);
            }
        }
        // I haven't seen Longs in a save file yet, but SLLD would make sense for the name
        // TODO: test this
        if (esm.isNextSub("SLLD"))
        {
            esm.getSubHeader();
            for (int i=0; i<numLongs; ++i)
            {
                int val;
                esm.getT(val);
                mLongs.push_back(val);
            }
        }
        if (esm.isNextSub("SLFD"))
        {
            esm.getSubHeader();
            for (int i=0; i<numFloats; ++i)
            {
                float val;
                esm.getT(val);
                mFloats.push_back(val);
            }
        }
    }

}
