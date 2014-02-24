
#include "creaturestats.hpp"

void ESM::CreatureStats::load (ESMReader &esm)
{
    for (int i=0; i<8; ++i)
        mAttributes[i].load (esm);

    for (int i=0; i<3; ++i)
        mDynamic[i].load (esm);
}

void ESM::CreatureStats::save (ESMWriter &esm) const
{
    for (int i=0; i<8; ++i)
        mAttributes[i].save (esm);

    for (int i=0; i<3; ++i)
        mDynamic[i].save (esm);
}