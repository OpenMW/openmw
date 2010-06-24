#ifndef _ESM_LEVLISTS_H
#define _ESM_LEVLISTS_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Leveled lists. Since these have identical layout, I only bothered
 * to implement it once.
 *
 * We should later implement the ability to merge leveled lists from
 * several files. 
 */

struct LeveledListBase
{
  enum Flags
    {
      AllLevels         = 0x01, // Calculate from all levels <= player
                                // level, not just the closest below
                                // player.
      Each              = 0x02  // Select a new item each time this
                                // list is instantiated, instead of
                                // giving several identical items
    };                          // (used when a container has more
                                // than one instance of one leveled
                                // list.)
  int flags;
  unsigned char chanceNone; // Chance that none are selected (0-255?)

  // Record name used to read references. Must be set before load() is
  // called.
  const char *recName;

  struct LevelItem
  {
    std::string id;
    short level;
  };

  std::vector<LevelItem> list;

  void load(ESMReader &esm)
  {
    esm.getHNT(flags, "DATA");
    esm.getHNT(chanceNone, "NNAM");

    if(esm.isNextSub("INDX"))
      {
        int len;
        esm.getHT(len);
        list.resize(len);
      }
    else return;

    // TODO: Merge with an existing lists here. This can be done
    // simply by adding the lists together, making sure that they are
    // sorted by level. A better way might be to exclude repeated
    // items. Also, some times we don't want to merge lists, just
    // overwrite. Figure out a way to give the user this option.
 
    for(size_t i=0; i<list.size(); i++)
      {
        LevelItem &li = list[i];
        li.id = esm.getHNString(recName);
        esm.getHNT(li.level, "INTV");
      }
  }
};

struct CreatureLevList : LeveledListBase
{ CreatureLevList() { recName = "CNAM"; } };

struct ItemLevList : LeveledListBase
{ ItemLevList() { recName = "INAM"; } };

}
#endif
