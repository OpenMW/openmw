#include "loadlevlist.hpp"

namespace ESM
{

void LeveledListBase::load(ESMReader &esm)
{
    esm.getHNT(flags, "DATA");
    esm.getHNT(chanceNone, "NNAM");

    if (esm.isNextSub("INDX"))
    {
        int len;
        esm.getHT(len);
        list.resize(len);
    }
    else
        return;

    // TODO: Merge with an existing lists here. This can be done
    // simply by adding the lists together, making sure that they are
    // sorted by level. A better way might be to exclude repeated
    // items. Also, some times we don't want to merge lists, just
    // overwrite. Figure out a way to give the user this option.

    for (size_t i = 0; i < list.size(); i++)
    {
        LevelItem &li = list[i];
        li.id = esm.getHNString(recName);
        esm.getHNT(li.level, "INTV");
    }
}
void LeveledListBase::save(ESMWriter &esm)
{
    esm.writeHNT("DATA", flags);
    esm.writeHNT("NNAM", chanceNone);
    esm.writeHNT<int>("INDX", list.size());
    
    for (std::vector<LevelItem>::iterator it = list.begin(); it != list.end(); ++it)
    {
        esm.writeHNString("INAM", it->id);
        esm.writeHNT("INTV", it->level);
    }
}

}
