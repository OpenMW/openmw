/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadinfo.d) is part of the OpenMW package.

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

module esm.loadinfo;

import esm.imports;
import esm.loaddial;
import esm.loadrace;
import esm.loadclas;
import esm.loadfact;

/*
 * Dialogue information. These always follow a DIAL record and form a
 * linked list
 */

struct DialInfo
{
  enum Gender : byte
    {
      Male = 0,
      Female = 1,
      NA = -1
    }

  align(1) struct DATAstruct
  {
    uint unknown1;
    uint disposition;
    byte rank;     // Rank of NPC
    Gender gender; // 0 - male, 1 - female, 0xff - none
    byte PCrank;   // Player rank
    byte unknown2;

    static assert(DATAstruct.sizeof==12);
  }
  DATAstruct data;

  // The rules for whether or not we will select this dialog item.
  struct SelectStruct
  {
    char[] selectRule; // This has a complicated format
    union { float f; int i; }
    VarType type;
  }

  // Id of prevous and next in linked list
  char[] id, prev, next;

  // Various references used in determining when to select this item.
  Item actor;
  Race *race;
  Class *cls;
  Faction* npcFaction, pcFaction;
  bool factionLess; // ONLY select this item the NPC is not part of any faction.
  char[] cell; // Use this to compare with current cell name
  RegionBuffer!(SelectStruct) selects; // Selection rules

  SoundIndex sound; // Sound to play when this is selected.
  char[] response; // The text content of this info item.

  // TODO: This should probably be compiled at load time?
  char[] resultScript; // Result script (uncompiled) - is run if this
		       // dialog item is selected.

  // Journal quest indices (introduced with the quest system in tribunal)
  enum Quest
    {
      None, Name, Finished, Restart
    }

  bool deleted;

  Quest questStatus;

  void load(DialogueType tp)
  {with(esFile){
    id = getHNString("INAM");
    prev = getHNString("PNAM");
    next = getHNString("NNAM");

    // Not present if deleted
    if(isNextSub("DATA"))
      readHExact(&data, data.sizeof);

    // What follows is terrible spaghetti code, but I like it! ;-) In
    // all seriousness, it's an attempt to make things faster, since
    // INFO is by far the most common record type.

    // subName is a slice of the original, so it changes when new sub
    // names are read.
    getSubName();
    char[] subName = retSubName();

    if(subName == "ONAM")
      {
	actor = actors.lookup(tmpHString());
	if(isEmptyOrGetName) return;
      } else actor.i = null;
    if(subName == "RNAM")
      {
	race = cast(Race*)races.lookup(tmpHString());
	if(isEmptyOrGetName) return;
      } else race = null;
    if(subName == "CNAM")
      {
	cls = cast(Class*)classes.lookup(tmpHString());
	if(isEmptyOrGetName) return;
      } else cls = null;

    npcFaction = null;
    factionLess = false;
    if(subName == "FNAM")
      {
	char[] tmp = tmpHString();
	if(tmp == "FFFF") factionLess = true;
	else npcFaction = cast(Faction*)factions.lookup(tmp);

	if(isEmptyOrGetName) return;
      }
    if(subName == "ANAM")
      {
	cell = getHString();
	if(isEmptyOrGetName) return;
      } else cell = null;
    if(subName == "DNAM")
      {
	pcFaction = cast(Faction*)factions.lookup(tmpHString());
	if(isEmptyOrGetName) return;
      } else pcFaction = null;
    if(subName == "SNAM")
      {
	sound = resources.lookupSound(tmpHString());
	if(isEmptyOrGetName) return;
      } else sound = SoundIndex.init;
    if(subName == "NAME")
      {
	response = getHString();
	if(isEmptyOrGetName) return;
      } else response = null;

    selects = null;
    while(subName == "SCVR")
      {
	if(selects is null) selects = esmRegion.getBuffer!(SelectStruct)(1,1);
	else selects.length = selects.length + 1;
	with(selects.array()[$-1])
	  {
	    selectRule = getHString();
	    isEmptyOrGetName();
	    if(subName == "INTV") type = VarType.Int;
	    else if(subName == "FLTV") type = VarType.Float;
	    else 
	      fail("INFO.SCVR must precede INTV or FLTV, not "
		   ~ subName);
	    readHExact(&i, 4);
	  }
	if(isEmptyOrGetName) return;
      }

    if(subName == "BNAM")
      {
	resultScript = getHString();
	if(isEmptyOrGetName) return;
      } else resultScript = null;

    deleted = false;
    questStatus = Quest.None;

    // Figure out how we check the owner's type
    if(tp == Dialogue.Type.Journal)
      {
	if(subName == "QSTN") questStatus = Quest.Name;
	else if(subName == "QSTF") questStatus = Quest.Finished;
	else if(subName == "QSTR") questStatus = Quest.Restart;

	if(questStatus != Quest.None) getHByte();
      }
    else if(subName == "DELE") {getHInt(); deleted = true;}
    else fail("Don't know what to do with " ~ subName ~ " here.");
  }}
}

/*
  // We only need to put each item in ONE list. For if your NPC
  // matches this response, then it must match ALL criteria, thus it
  // will have to look up itself in all the lists. I think the order
  // is well optimized in making the lists as small as possible.
  if(this.actor.index != -1) actorDial[this.actor][parent]++;
  else if(cell != "") cellDial[cell][parent]++;
  else if(this.Class != -1) classDial[this.Class][parent]++;
  else if(this.npcFaction != -1)
    factionDial[this.npcFaction][parent]++;
  else if(this.race != -1) raceDial[this.race][parent]++;
  else allDial[parent]++; // Lists dialogues that might
  // apply to all npcs.
  */

struct DialInfoList
{
  LoadState state;

  DialInfo d;

  void load(DialogueType type)
  {
    d.load(type);
  }
}

// List of dialogue topics (and greetings, voices, etc.) that
// reference other objects. Eg. raceDial is indexed by the indices of
// all races referenced. The value of raceDial is a new AA, which is
// basically used as a map (the int value is just a count and isn't
// used for anything important.) The indices (or elements of the map)
// are the dialogues that reference the given race. I use an AA
// instead of a list or array, since each dialogue can be added lots
// of times.

/*
int allDial[Dialogue*];
int classDial[int][Dialogue*];
int factionDial[int][Dialogue*];
int actorDial[Item][Dialogue*];
// If I look up cells on cell load, I don't have to resolve these
// names into anything!
int cellDial[char[]][Dialogue*];
int raceDial[int][Dialogue*];
*/
