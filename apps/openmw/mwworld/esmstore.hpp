#ifndef OPENMW_MWWORLD_STORE_H
#define OPENMW_MWWORLD_STORE_H

/*
  The ESM storage module.

  This is separate from the ESM loader module, located in esm/. It is
  also unaware of the cell loading and storage module.

  The advantage of this, as with all other modularizations, is that
  you can replace the storage method later without touching the
  loading code. Cutting down dependencies also help on the general
  maintainability.
 */

#include <components/esm/records.hpp>
#include "reclists.hpp"

namespace MWWorld 
{

  struct ESMStore
  {
    /* Lists all the list types. Mostly used for quick lookup on
       loading. The key is the record name (4 chars) parsed as a 32
       bit int. See esm/records.hpp for the complete list.
    */
    RecListList recLists;

    // Each individual list
    RecListT<ESM::Activator>         activators;
    RecListWithIDT<ESM::Potion>      potions;
    RecListT<ESM::Apparatus>         appas;
    RecListT<ESM::Armor>             armors;
    RecListT<ESM::BodyPart>          bodyParts;
    RecListWithIDT<ESM::Book>        books;
    RecListT<ESM::BirthSign>         birthSigns;
    RecListT<ESM::Class>             classes;
    RecListT<ESM::Clothing>          clothes;
    RecListT<ESM::LoadCNTC>          contChange;
    RecListT<ESM::Container>         containers;
    RecListWithIDT<ESM::Creature>    creatures;
    RecListT<ESM::LoadCREC>          creaChange;
    RecListCaseT<ESM::Dialogue>      dialogs;
    RecListT<ESM::Door>              doors;
    RecListT<ESM::Enchantment>       enchants;
    RecListT<ESM::Faction>           factions;
    RecListT<ESM::Global>            globals;
    RecListWithIDT<ESM::Ingredient>  ingreds;
    RecListT<ESM::CreatureLevList>   creatureLists;
    RecListT<ESM::ItemLevList>       itemLists;
    RecListT<ESM::Light>             lights;
    RecListT<ESM::Tool>              lockpicks;
    RecListT<ESM::Miscellaneous>     miscItems;
    RecListWithIDT<ESM::NPC>         npcs;
    RecListT<ESM::LoadNPCC>          npcChange;
    RecListT<ESM::Probe>             probes;
    RecListT<ESM::Race>              races;
    RecListT<ESM::Region>            regions;
    RecListT<ESM::Repair>            repairs;
    RecListT<ESM::SoundGenerator>    soundGens;
    RecListT<ESM::Sound>             sounds;
    RecListT<ESM::Spell>             spells;
    RecListT<ESM::StartScript>       startScripts;
    RecListT<ESM::Static>            statics;
    RecListT<ESM::Weapon>            weapons;

    // Lists that need special rules
    CellList                    cells;
    RecListWithIDT<ESM::GameSetting> gameSettings;
    LandList                    lands;
    LTexList                    landTexts;
    ScriptListT<ESM::Script>         scripts;
    IndexListT<ESM::MagicEffect>     magicEffects;
    IndexListT<ESM::Skill>           skills;
    //RecListT<ESM::Pathgrid>          pathgrids;
    PathgridList                pathgrids;

    // Special entry which is hardcoded and not loaded from an ESM
    IndexListT<ESM::Attribute>       attributes;

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
      recLists[ESM::REC_ACTI] = &activators;
      recLists[ESM::REC_ALCH] = &potions;
      recLists[ESM::REC_APPA] = &appas;
      recLists[ESM::REC_ARMO] = &armors;
      recLists[ESM::REC_BODY] = &bodyParts;
      recLists[ESM::REC_BOOK] = &books;
      recLists[ESM::REC_BSGN] = &birthSigns;
      recLists[ESM::REC_CELL] = &cells;
      recLists[ESM::REC_CLAS] = &classes;
      recLists[ESM::REC_CLOT] = &clothes;
      recLists[ESM::REC_CNTC] = &contChange;
      recLists[ESM::REC_CONT] = &containers;
      recLists[ESM::REC_CREA] = &creatures;
      recLists[ESM::REC_CREC] = &creaChange;
      recLists[ESM::REC_DIAL] = &dialogs;
      recLists[ESM::REC_DOOR] = &doors;
      recLists[ESM::REC_ENCH] = &enchants;
      recLists[ESM::REC_FACT] = &factions;
      recLists[ESM::REC_GLOB] = &globals;
      recLists[ESM::REC_GMST] = &gameSettings;
      recLists[ESM::REC_INGR] = &ingreds;
      recLists[ESM::REC_LAND] = &lands;
      recLists[ESM::REC_LEVC] = &creatureLists;
      recLists[ESM::REC_LEVI] = &itemLists;
      recLists[ESM::REC_LIGH] = &lights;
      recLists[ESM::REC_LOCK] = &lockpicks;
      recLists[ESM::REC_LTEX] = &landTexts;
      recLists[ESM::REC_MISC] = &miscItems;
      recLists[ESM::REC_NPC_] = &npcs;
      recLists[ESM::REC_NPCC] = &npcChange;
      recLists[ESM::REC_PGRD] = &pathgrids;
      recLists[ESM::REC_PROB] = &probes;
      recLists[ESM::REC_RACE] = &races;
      recLists[ESM::REC_REGN] = &regions;
      recLists[ESM::REC_REPA] = &repairs;
      recLists[ESM::REC_SCPT] = &scripts;
      recLists[ESM::REC_SNDG] = &soundGens;
      recLists[ESM::REC_SOUN] = &sounds;
      recLists[ESM::REC_SPEL] = &spells;
      recLists[ESM::REC_SSCR] = &startScripts;
      recLists[ESM::REC_STAT] = &statics;
      recLists[ESM::REC_WEAP] = &weapons;
    }

    void load(ESM::ESMReader &esm);
  };
}

#endif
