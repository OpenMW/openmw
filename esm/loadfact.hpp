#ifndef _ESM_FACT_H
#define _ESM_FACT_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Faction definitions
 */

// Requirements for each rank
struct RankData
{
  int attribute1, attribute2; // Attribute level

  int skill1, skill2;	  // Skill level (faction skills given in
		          // skillID below.) You need one skill at
		          // level 'skill1' and two skills at level
		          // 'skill2' to advance to this rank.

  int factReaction;	  // Reaction from faction members
};

struct Faction
{
  std::string id, name;

  struct FADTstruct
  {
    // Which attributes we like
    int	attribute1, attribute2;

    RankData rankData[10];

    int skillID[6]; // IDs of skills this faction require
    int unknown;    // Always -1?
    int isHidden;   // 1 - hidden from player
  }; // 240 bytes

  FADTstruct data;

  struct Reaction
  {
    std::string faction;
    int reaction;
  };

  std::vector<Reaction> reactions;

  // Name of faction ranks (may be empty for NPC factions)
  std::string ranks[10];

  void load(ESMReader &esm)
  {
    name = esm.getHNString("FNAM");

    // Read rank names. These are optional.
    int i = 0;
    while(esm.isNextSub("RNAM") && i<10) ranks[i++] = esm.getHString();

    // Main data struct
    esm.getHNT(data, "FADT", 240);

    if(data.isHidden > 1) esm.fail("Unknown flag!");

    // Read faction response values
    while(esm.hasMoreSubs())
      {
        Reaction r;
        r.faction = esm.getHNString("ANAM");
        esm.getHNT(r.reaction, "INTV");
        reactions.push_back(r);
      }
  }
};
}
#endif
