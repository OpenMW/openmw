#include "spelllist.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM {

void SpellList::load(ESMReader &esm)
{
    while (esm.isNextSub("NPCS")) {
        mList.push_back(esm.getHString());
    }
}

void SpellList::save(ESMWriter &esm) const
{
    for (std::vector<std::string>::const_iterator it = mList.begin(); it != mList.end(); ++it) {
        esm.writeHNString("NPCS", *it, 32);
    }
}

}
