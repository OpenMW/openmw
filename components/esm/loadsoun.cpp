#include "loadsoun.hpp"

namespace ESM
{

void Sound::load(ESMReader &esm)
{
    sound = esm.getHNString("FNAM");
    esm.getHNT(data, "DATA", 3);
    /*
     cout << "vol=" << (int)data.volume
     << " min=" << (int)data.minRange
     << " max=" << (int)data.maxRange
     << endl;
     */
}

}
