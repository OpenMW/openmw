#include <map>
#include <iostream>
#include "esm_store.hpp"

using namespace ESM;
using namespace std;

struct RecList
{
  virtual void load(ESMReader &esm) = 0;
  virtual int getSize() = 0;
  virtual const string& getName() = 0;
};

/* Lists all the list types. Mostly used for quick lookup on
   loading. The index is the record name (4 chars) parsed as a 32 bit
   int. Eg. REC_ACTI, REC_BOOK etc. See esm/records.hpp for the
   complete list.
 */
typedef map<int,RecList*> RecListList;
RecListList recLists;

template <int RecID>
struct RecListIDT : RecList
{
  // Store ourselves in the list of all lists
  RecListIDT() { recLists[RecID] = this; }
};

template <int RecID, typename X>
struct RecListT : RecListIDT<RecID>
{
  typedef map<string,X> MapType;

  MapType list;
  string listName;

  RecListT(const string &name) : listName(name) {}

  void load(ESMReader &esm)
  {
    string id = esm.getHNString("NAME");

    X &ref = list[id];
    ref.load(esm);
  }

  int getSize() { return list.size(); }

  const string& getName() { return listName; }
};

// The only difference to the above is a slight change to the load()
// function. We might merge these together later, and store the id in
// all the structs.
template <int RecID, typename X>
struct RecIDListT : RecListIDT<RecID>
{
  typedef map<string,X> MapType;

  MapType list;
  string listName;

  RecIDListT(const string &name) : listName(name) {}

  void load(ESMReader &esm)
  {
    string id = esm.getHNString("NAME");

    X &ref = list[id];
    ref.id = id;
    ref.load(esm);
  }

  int getSize() { return list.size(); }

  const string& getName() { return listName; }
};

// Cells aren't simply indexed by name. Exterior cells are treated
// separately.
struct CellList : RecListIDT<REC_CELL>
{
  // Just count them for now
  int count;
  string listName;

  CellList() { listName = "Cells"; }

  void load(ESMReader &esm)
  {
    count++;
    esm.skipRecord();
  }

  int getSize() { return count; }

  const string& getName() { return listName; }
};

/* We need special lists for:

   Cells (partially done)
   Magic effects
   Skills
   Dialog / Info combo
   Scripts
   Land
   Path grids
   Land textures
 */

RecListT<REC_ACTI, Activator>   activators      ("Activators");
RecListT<REC_ALCH, Potion>      potions         ("Potions");
RecListT<REC_APPA, Apparatus>   appas           ("Apparatuses");
RecListT<REC_ARMO, Armor>       armors          ("Armors");
RecListT<REC_BODY, BodyPart>    bodyParts       ("Body parts");
RecListT<REC_BOOK, Book>        books           ("Books");
RecListT<REC_BSGN, BirthSign>   birthSigns      ("Birth signs");
CellList                        cells;
RecListT<REC_CLAS, Class>       classes         ("Classes");
RecListT<REC_CLOT, Clothing>    clothes         ("Clothes");
RecListT<REC_CNTC, LoadCNTC>    contChange      ("Container changes");
RecListT<REC_CONT, Container>   containers      ("Containers");
RecListT<REC_CREA, Creature>    creatures       ("Creatures");
RecListT<REC_CREC, LoadCREC>    creaChange      ("Creature changes");
RecListT<REC_DIAL, Dialogue>    dialogs         ("Dialogues");
RecListT<REC_DOOR, Door>        doors           ("Doors");
RecListT<REC_ENCH, Enchantment> enchants        ("Enchantments");
RecListT<REC_FACT, Faction>     factions        ("Factions");
RecListT<REC_GLOB, Global>      globals         ("Globals");
RecIDListT<REC_GMST,GameSetting>gameSettings    ("Game settings");
//RecListT<REC_INFO, DialInfo>    dialInfos       ("Dialog entries");
RecListT<REC_INGR, Ingredient>  ingreds         ("Ingredients");
//RecListT<REC_LAND, Land>        lands           ("Land data");
RecListT<REC_LEVC, CreatureLevList> creatureLists("Creature leveled lists");
RecListT<REC_LEVI, ItemLevList> itemLists       ("Item leveled lists");
RecListT<REC_LIGH, Light>       lights          ("Lights");
RecListT<REC_LOCK, Tool>        lockpicks       ("Lockpicks");
//RecListT<REC_LTEX, LandTexture> landTexts       ("Land textures");
//RecListT<REC_MGEF, MagicEffect> magicEffects    ("Magic effects");
RecListT<REC_MISC, Misc>        miscItems       ("Misc items");
RecListT<REC_NPC_, NPC>         npcs            ("NPCs");
RecListT<REC_NPCC, LoadNPCC>    npcChange       ("NPC changes");
//RecListT<REC_PGRD, PathGrid>    pathgrids       ("Path grids");
RecListT<REC_PROB, Tool>        probes          ("Probes");
RecListT<REC_RACE, Race>        races           ("Races");
RecListT<REC_REGN, Region>      regions         ("Regions");
RecListT<REC_REPA, Tool>        repairs         ("Repair items");
//RecListT<REC_SCPT, Script>      scripts         ("Scripts");
//RecListT<REC_SKIL, Skill>       skills          ("Skills");
RecListT<REC_SNDG, SoundGenerator> soundGens    ("Sound generators");
RecListT<REC_SOUN, Sound>       sounds          ("Sounds");
RecListT<REC_SPEL, Spell>       spells          ("Spells");
RecListT<REC_SSCR, StartScript> startScripts    ("Start scripts");
RecListT<REC_STAT, Static>      statics         ("Statics");
RecListT<REC_WEAP, Weapon>      weapons         ("Weapons");

void storeESM(ESMReader &esm)
{
  // Loop through all records
  while(esm.hasMoreRecs())
    {
      NAME n = esm.getRecName();
      esm.getRecHeader();

      // Look up the record type.
      RecListList::iterator it = recLists.find(n.val);

      if(it == recLists.end())
        {
          // Not found (this would be an error later)
          esm.skipRecord();
          continue;
        }

      // Load it
      it->second->load(esm);
    }

  cout << "\n" << recLists.size() << " record types:\n";
  for(RecListList::iterator it = recLists.begin(); it != recLists.end(); it++)
    cout << "  " << it->second->getName() << ": " << it->second->getSize() << endl;
}
