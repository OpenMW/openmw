#ifndef _GAME_ESM_STORE_H
#define _GAME_ESM_STORE_H

/*
  The ESM storage module.

  This is separate from the ESM loader module, located in esm/. It is
  also unaware of the cell loading and storage module.

  The advantage of this, as with all other modularizations, is that
  you can replace the storage method later without touching the
  loading code. Cutting down dependencies also help on the general
  maintainability.
 */

#include "components/esm/records.hpp"
#include "reclists.hpp"

namespace ESMS
{
  using namespace ESM;

  struct ESMStore
  {
    /* Lists all the list types. Mostly used for quick lookup on
       loading. The key is the record name (4 chars) parsed as a 32
       bit int. See esm/records.hpp for the complete list.
    */
    RecListList recLists;

    // Each individual list
    RecListT<Activator>         activators;
    RecListWithIDT<Potion>      potions;
    RecListT<Apparatus>         appas;
    RecListT<Armor>             armors;
    RecListT<BodyPart>          bodyParts;
    RecListT<Book>              books;
    RecListT<BirthSign>         birthSigns;
    RecListT<Class>             classes;
    RecListT<Clothing>          clothes;
    RecListT<LoadCNTC>          contChange;
    RecListT<Container>         containers;
    RecListWithIDT<Creature>    creatures;
    RecListT<LoadCREC>          creaChange;
    RecListCaseT<Dialogue>      dialogs;
    RecListT<Door>              doors;
    RecListT<Enchantment>       enchants;
    RecListT<Faction>           factions;
    RecListT<Global>            globals;
    RecListWithIDT<Ingredient>  ingreds;
    RecListT<CreatureLevList>   creatureLists;
    RecListT<ItemLevList>       itemLists;
    RecListT<Light>             lights;
    RecListT<Tool>              lockpicks;
    RecListT<Miscellaneous>     miscItems;
    RecListWithIDT<NPC>         npcs;
    RecListT<LoadNPCC>          npcChange;
    RecListT<Probe>             probes;
    RecListT<Race>              races;
    RecListT<Region>            regions;
    RecListT<Repair>            repairs;
    RecListT<SoundGenerator>    soundGens;
    RecListT<Sound>             sounds;
    RecListT<Spell>             spells;
    RecListT<StartScript>       startScripts;
    RecListT<Static>            statics;
    RecListT<Weapon>            weapons;

    // Lists that need special rules
    CellList                    cells;
    RecIDListT<GameSetting>     gameSettings;
    LandList                    lands;
    LTexList                    landTexts;
    ScriptListT<Script>         scripts;
    IndexListT<MagicEffect>     magicEffects;
    IndexListT<Skill>           skills;
    //RecListT<Pathgrid>          pathgrids;
    PathgridList                pathgrids;

    // Special entry which is hardcoded and not loaded from an ESM
    IndexListT<Attribute>       attributes;

    // Lookup of all IDs. Makes looking up references faster. Just
    // maps the id name to the record type.
    typedef std::map<std::string, int> AllMap;
    AllMap all;

    // Look up the given ID in 'all'. Returns 0 if not found.
    int find(const std::string &id) const
    {
      AllMap::const_iterator it = all.find(id);
      if(it == all.end()) return 0;
      return it->second;
    }

    ESMStore()
    {
      recLists[REC_ACTI] = &activators;
      recLists[REC_ALCH] = &potions;
      recLists[REC_APPA] = &appas;
      recLists[REC_ARMO] = &armors;
      recLists[REC_BODY] = &bodyParts;
      recLists[REC_BOOK] = &books;
      recLists[REC_BSGN] = &birthSigns;
      recLists[REC_CELL] = &cells;
      recLists[REC_CLAS] = &classes;
      recLists[REC_CLOT] = &clothes;
      recLists[REC_CNTC] = &contChange;
      recLists[REC_CONT] = &containers;
      recLists[REC_CREA] = &creatures;
      recLists[REC_CREC] = &creaChange;
      recLists[REC_DIAL] = &dialogs;
      recLists[REC_DOOR] = &doors;
      recLists[REC_ENCH] = &enchants;
      recLists[REC_FACT] = &factions;
      recLists[REC_GLOB] = &globals;
      recLists[REC_GMST] = &gameSettings;
      recLists[REC_INGR] = &ingreds;
      recLists[REC_LAND] = &lands;
      recLists[REC_LEVC] = &creatureLists;
      recLists[REC_LEVI] = &itemLists;
      recLists[REC_LIGH] = &lights;
      recLists[REC_LOCK] = &lockpicks;
      recLists[REC_LTEX] = &landTexts;
      recLists[REC_MISC] = &miscItems;
      recLists[REC_NPC_] = &npcs;
      recLists[REC_NPCC] = &npcChange;
      recLists[REC_PGRD] = &pathgrids;
      recLists[REC_PROB] = &probes;
      recLists[REC_RACE] = &races;
      recLists[REC_REGN] = &regions;
      recLists[REC_REPA] = &repairs;
      recLists[REC_SCPT] = &scripts;
      recLists[REC_SNDG] = &soundGens;
      recLists[REC_SOUN] = &sounds;
      recLists[REC_SPEL] = &spells;
      recLists[REC_SSCR] = &startScripts;
      recLists[REC_STAT] = &statics;
      recLists[REC_WEAP] = &weapons;
    }

    void load(ESMReader &esm);
  };
}

#endif
