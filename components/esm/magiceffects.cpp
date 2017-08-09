#include "magiceffects.hpp"

#include "esmwriter.hpp"
#include "esmreader.hpp"

namespace ESM
{

void MagicEffects::save(ESMWriter &esm) const
{
    for (std::map<int, int>::const_iterator it = mEffects.begin(); it != mEffects.end(); ++it)
    {
        esm.writeHNT("EFID", it->first);
        esm.writeHNT("BASE", it->second);
    }
}

void MagicEffects::load(ESMReader &esm)
{
    while (esm.isNextSub("EFID"))
    {
        int id, base;
        esm.getHT(id);
        esm.getHNT(base, "BASE");
        mEffects.insert(std::make_pair(id, base));
    }
}

}
