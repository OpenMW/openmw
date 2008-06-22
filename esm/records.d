/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (records.d) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */
module esm.records;

public
{
  import monster.util.aa;
  import util.regions;
  
  import core.memory;
  import core.resource;

  import esm.filereader;
  import esm.defs;
  import esm.listkeeper;

  import std.stdio; // Remove later
}

public import
  esm.loadacti, esm.loaddoor, esm.loadglob, esm.loadscpt, esm.loadsoun, esm.loadgmst,
  esm.loadfact, esm.loadstat, esm.loadspel, esm.loadalch, esm.loadappa, esm.loadarmo,
  esm.loadbody, esm.loadench, esm.loadbook, esm.loadbsgn, esm.loadltex, esm.loadmgef,
  esm.loadweap, esm.loadlocks,esm.loadcell, esm.loadregn, esm.loadligh, esm.loadskil,
  esm.loadsndg, esm.loadrace, esm.loadmisc, esm.loadclot, esm.loadingr, esm.loadclas,
  esm.loadcont, esm.loadcrea, esm.loadnpc,  esm.loaddial, esm.loadinfo, esm.loadsscr,
  esm.loadlevlist;

void loadRecord(char[] recName)
{
  switch(recName)
    {
    case "ACTI": activators.load(); break;
    case "DOOR": doors.load(); break;
    case "GLOB": globals.load(); break;
    case "SCPT": scripts.load(); break;
    case "SOUN": sounds.load(); break;
    case "GMST": gameSettings.load(); break;
    case "FACT": factions.load(); break;
    case "STAT": statics.load(); break; 
    case "SPEL": spells.load(); break;
    case "ALCH": potions.load(); break;
    case "APPA": appas.load(); break;
    case "ARMO": armors.load(); break;
    case "BODY": bodyParts.load(); break;
    case "ENCH": enchants.load(); break;
    case "BOOK": books.load(); break;
    case "BSGN": birthSigns.load(); break;
    case "LTEX": landTextures.load(); break;
    case "MGEF": effects.load(); break;
    case "WEAP": weapons.load(); break;
    case "REPA": repairs.load(); break;
    case "LOCK": lockpicks.load(); break;
    case "PROB": probes.load(); break;
    case "CELL": cells.load(); break;
    case "REGN": regions.load(); break;
    case "LIGH": lights.load(); break;
    case "SKIL": skills.load(); break;
    case "SNDG": soundGens.load(); break;
    case "RACE": races.load(); break;
    case "MISC": miscItems.load(); break;
    case "CLOT": clothes.load(); break;
    case "INGR": ingreds.load(); break;
    case "CLAS": classes.load(); break;
    case "CONT": containers.load(); break;
    case "CREA": creatures.load(); break;
    case "LEVI": itemLists.load(); break;
    case "LEVC": creatureLists.load(); break;
    case "NPC_": npcs.load(); break;
    case "DIAL": dialogues.load(); break;
    case "SSCR": startScripts.load(); break;
  /*

      // Tribunal / Bloodmoon only
    case "SSCR": loadSSCR.load(); break;

  */
      // For save games:
      // case "NPCC": loadNPCC;
      // case "CNTC": loadCNTC;
      // case "CREC": loadCREC;

      // These should never be looked up
    case "TES3":
    case "INFO":
    case "LAND":
    case "PGRD":
      esFile.fail("Misplaced record " ~ recName);
    default:
      esFile.fail("Unknown record type " ~ recName);
    }
  //*/
}

// Um, this has to be in this file for some reason.
ListID!(Dialogue) dialogues;

struct ItemT
{
  Item item;
  ItemBase *i;

  T* getType(T, ItemType Type)()
  {
    return item.getType!(T, Type)();
  }

  alias getType!(Potion, ItemType.Potion) getPotion;
  alias getType!(Apparatus, ItemType.Apparatus) getApparatus;
  alias getType!(Armor, ItemType.Armor) getArmor;
  alias getType!(Weapon, ItemType.Weapon) getWeapon;
  alias getType!(Book, ItemType.Book) getBook;
  alias getType!(Clothing, ItemType.Clothing) getClothing;
  alias getType!(Light, ItemType.Light) getLight;
  alias getType!(Ingredient, ItemType.Ingredient) getIngredient;
  alias getType!(Tool, ItemType.Pick) getPick;
  alias getType!(Tool, ItemType.Probe) getProbe;
  alias getType!(Tool, ItemType.Repair) getRepair;
  alias getType!(Misc, ItemType.Misc) getMisc;
  alias getType!(LeveledItems, ItemType.ItemList) getItemList;
  alias getType!(Creature, ItemType.Creature) getCreature;
  alias getType!(LeveledCreatures, ItemType.CreatureList) getCreatureList;
  alias getType!(NPC, ItemType.NPC) getNPC;
  alias getType!(Door, ItemType.Door) getDoor;
  alias getType!(Activator, ItemType.Activator) getActivator;
  alias getType!(Static, ItemType.Static) getStatic;
  alias getType!(Container, ItemType.Container) getContainer;

  static ItemT opCall(Item it)
  {
    ItemT itm;
    itm.item = it;
    itm.i = it.i;
    return itm;
  }
}

void endFiles()
{
  foreach(ListKeeper l; recordLists)
    l.endFile();

  items.endFile();
}

void initializeLists()
{
  recordLists = null;

  // Initialize all the lists here. The sizes have been chosen big
  // enough to hold the main ESM files and a large number of mods
  // without rehashing.

  activators = new ListID!(Activator)(1400);
  doors = new ListID!(Door)(300);
  globals = new ListID!(Global)(300);
  scripts = new ScriptList(1800);
  sounds = new ListID!(Sound)(1000);
  gameSettings = new ListID!(GameSetting)(1600);
  factions = new ListID!(Faction)(30);
  statics = new ListID!(Static)(4000);
  spells = new ListID!(Spell)(1300);
  potions = new ListID!(Potion)(300);
  appas = new ListID!(Apparatus)(30);
  armors = new ListID!(Armor)(500);
  bodyParts = new ListID!(BodyPart)(2300);
  enchants = new ListID!(Enchantment)(1000);
  books = new ListID!(Book)(700);
  birthSigns = new ListID!(BirthSign)(30);
  landTextures = new LandTextureList;
  effects = new MagicEffectList;
  weapons = new ListID!(Weapon)(700);
  lockpicks = new ListID!(Tool)(10);
  probes = new ListID!(Tool)(10);
  repairs = new ListID!(Tool)(10);
  cells = new CellList;
  regions = new ListID!(Region)(20);
  lights = new ListID!(Light)(1000);
  skills = new SkillList;
  soundGens = new ListID!(SoundGenerator)(500);
  races = new ListID!(Race)(100);
  miscItems = new ListID!(Misc)(700);
  clothes = new ListID!(Clothing)(700);
  ingreds = new ListID!(Ingredient)(200);
  classes = new ListID!(Class)(100);
  containers = new ListID!(Container)(1200);
  creatures = new ListID!(Creature)(800);
  itemLists = new ListID!(LeveledItems)(600);
  creatureLists = new ListID!(LeveledCreatures)(400);
  npcs = new ListID!(NPC)(3500);
  dialogues = new ListID!(Dialogue)(3000);
  startScripts.init();

  hyperlinks.rehash(1600);

  items.rehash(5500);
  actors.rehash(5000);
  cellRefs.rehash(17000);
}
