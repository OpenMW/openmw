#include "spelllist.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM {

void SpellList::add(ESMReader &esm)
{
    mList.push_back(esm.getHString());
}

void SpellList::save(ESMWriter &esm) const
{
    for (std::vector<std::string>::const_iterator it = mList.begin(); it != mList.end(); ++it) {
        esm.writeHNString("NPCS", *it, 32);
    }
}

bool SpellList::exists(const std::string &spell) const
{
    for (std::vector<std::string>::const_iterator it = mList.begin(); it != mList.end(); ++it)
        if (Misc::StringUtils::ciEqual(*it, spell))
            return true;
    return false;
}

}
