#ifndef _ESM_SPEL_H
#define _ESM_SPEL_H

#include "esm_reader.hpp"

namespace ESM {

/** A list of references to spells and spell effects. This is shared
    between the records BSGN, NPC and RACE.

    TODO: This should also be part of a shared definition file
 */
struct SpellList
{
  std::vector<std::string> list;

  void load(ESMReader &esm)
  {
    while(esm.isNextSub("NPCS"))
      list.push_back(esm.getHString());
  }
};

  /* TODO: Not completely ported yet - depends on EffectList, which
     should be defined in a shared definition file.

struct Spell
{
  enum SpellType
    {
      ST_Spell          = 0, // Normal spell, must be cast and costs mana
      ST_Ability        = 1, // Inert ability, always in effect
      ST_Blight         = 2, // Blight disease
      ST_Disease	= 3, // Common disease
      ST_Curse          = 4, // Curse (?)
      ST_Power          = 5  // Power, can use once a day
    };

  enum Flags
    {
      F_Autocalc        = 1, 
      F_PCStart         = 2,
      F_Always          = 4 // Casting always succeeds
    };

  struct SPDTstruct
  {
    int type;   // SpellType
    int cost;   // Mana cost
    int flags;  // Flags
  };

  SPDTstruct data;

  std::string name;

  EffectList effects;

  void load()
    {with(esFile){
      name = getHNOString("FNAM");
      getHNT(data, "SPDT", 12);

      effects = getRegion().getBuffer!(ENAMstruct)(0,1);

      while(isNextSub("ENAM"))
	{
	  effects.length = effects.length + 1;
	  readHExact(&effects.array[$-1], effects.array[$-1].sizeof);
	}
    }}

}
  */
}
#endif
