#ifndef _ESM_WEAP_H
#define _ESM_WEAP_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Weapon definition
 */

struct Weapon
{
  enum Type
    {
      ShortBladeOneHand	= 0,
      LongBladeOneHand	= 1,
      LongBladeTwoHand	= 2,
      BluntOneHand	= 3,
      BluntTwoClose	= 4,
      BluntTwoWide	= 5,
      SpearTwoWide	= 6,
      AxeOneHand	= 7,
      AxeTwoHand	= 8,
      MarksmanBow	= 9,
      MarksmanCrossbow	= 10,
      MarksmanThrown	= 11,
      Arrow		= 12,
      Bolt		= 13
    };

  enum Flags
    {
      Magical	= 0x01,
      Silver	= 0x02
    };

#pragma pack(push)
#pragma pack(1)
  struct WPDTstruct
  {
    float weight;
    int value;
    short type;
    short health;
    float speed, reach;
    short enchant; // Enchantment points
    unsigned char chop[2], slash[2], thrust[2]; // Min and max
    int flags;
  }; // 32 bytes
#pragma pack(pop)

  WPDTstruct data;

  std::string name, model, icon, enchant, script;

  void load(ESMReader &esm)
  {
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "WPDT", 32);
    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
    enchant = esm.getHNOString("ENAM");
  }
};
}
#endif
