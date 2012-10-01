#include "loadsoun.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Sound::load(ESMReader &esm)
{
    mSound = esm.getHNString("FNAM");
    esm.getHNT(mData, "DATA", 3);
    /*
     cout << "vol=" << (int)data.volume
     << " min=" << (int)data.minRange
     << " max=" << (int)data.maxRange
     << endl;
     */
}
void Sound::save(ESMWriter &esm)
{
    esm.writeHNCString("FNAM", mSound);
    esm.writeHNT("DATA", mData, 3);
}

}
