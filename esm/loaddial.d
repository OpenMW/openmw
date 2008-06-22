/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loaddial.d) is part of the OpenMW package.

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

module esm.loaddial;
import esm.imports;
import esm.loadinfo;

/*
 * Dialogue topic and journal entries. The acutal data is contained in
 * the following INFO records.
 */

// Keep a list of possible hyper links. This list is used when parsing
// text and finding references to topic names. Eg. if a character says
// "Would you like to join the mages guild?", we must be able to pick
// out the phrase "join the mages guild?" and make a hyperlink of
// it. Each link is indexed by their first word. The structure
// contains the rest of the phrase, so the phrase above would be
// indexed by "join" and contain the string "the mages guild?", for
// quick comparison with the text are currently parsing. It also
// contains a pointer to the corresponding dialogue struct. The lists
// are sorted by descending string length, in order to match the
// longest possible term first.

struct Hyperlink
{
  Dialogue *ptr;
  char[] rest;

  // Returns a < b if a.length > b.length.
  int opCmp(Hyperlink *h) {return h.rest.length - rest.length;}
}

alias RegionBuffer!(Hyperlink) HyperlinkArray;

// This is much nicer now that we use our own AA.
struct HyperlinkList
{
  // Make a case insensitive hash table of Hyperlink arrays.
  HashTable!(char[], HyperlinkArray, ESMRegionAlloc, CITextHash) list;

  void add(char[] topic, Dialogue* ptr)
  {
    // Only add dialogues
    if(ptr.type != Dialogue.Type.Topic) return;

    Hyperlink l;
    l.ptr = ptr;
    l.rest = topic;

    // Use the first word as the index
    topic = nextWord(l.rest);

    // Insert a new array, or get an already existing one
    HyperlinkArray ha = list.get(topic,
	// Create a new array
	delegate void (ref HyperlinkArray a)
	{ a = esmRegion.getBuffer!(Hyperlink)(0,1); }
	);

    // Finally, add it to the list
    ha ~= l;
  }

  Hyperlink[] getList(char[] word)
  {
    HyperlinkArray p;
    if(list.inList(word, p)) return p.array();
    return null;
  }

  void rehash(uint size)
  {
    list.rehash(size);
  }

  // We wouldn't need this if we only dealt with one file, since the
  // topics are already sorted in Morrowind.esm. However, other files
  // might add items out of order later, so we have to sort it. To
  // understand why this is needed, consider the following example:
  //
  // Morrowind.esm contains the topic 'join us'. When ever the text
  // ".. join us blahblah ..." is encountered, this match is
  // found. However, if a plugin adds the topic 'join us today', we
  // have to place this _before_ 'join us' in the list, or else it
  // will never be matched.
  void sort()
  {
    foreach(char[] s, HyperlinkArray l; list)
      {
	l.array().sort;
	/*
	writefln("%s: ", s, l.length);
	foreach(Hyperlink h; l.array())
	  writefln("  %s (%s)", h.rest, h.ptr.id);
	*/
      }
  }
}

// List of dialogue hyperlinks
HyperlinkList hyperlinks;

struct Dialogue
{
  enum Type
    {
      Topic		= 0,
      Voice		= 1,
      Greeting		= 2,
      Persuasion	= 3,
      Journal		= 4,
      Deleted		= -1
    }

  //Type type;
  DialogueType type;

  DialInfoList infoList;

  char[] id; // This is the 'dialogue topic' that the user actually
	     // sees.
  LoadState state;

  void load()
  {with(esFile){
    getSubNameIs("DATA");

    getSubHeader();
    int si = getSubSize();
    if(si == 1)
      {
	byte b;
	getByte(b);
	DialogueType t = cast(DialogueType)b;

	// Meet the new type, same as the old type
	if(t != this.type && state == LoadState.Previous)
	  fail("Type changed in dialogue " ~ id);

	this.type = t;
      }
    else if(si == 4)
      {
	// These are just markers, their values are not used.
	int i;
	getInt(i);
	//writefln("In file %s:", getFilename());
	//writefln("  WARNING: DIAL.DATA was size 4 and contains: ", i);
	i = getHNInt("DELE");
	//writefln("  DELE contains ", i);
	this.type = Type.Deleted;
      }
    else fail("Unknown sub record size " ~ toString(si));

    infoList.state = state;
    while(isNextHRec("INFO"))
      infoList.load(this.type);
    //skipRecord();
  }}
}

// TODO/FIXME: HACK suddenly needed with version 0.167 :(
// Haven't re-tested it with non-ancient compiler
typedef Dialogue.Type DialogueType;

/+
    // I don't remember when I commented out this code or what state
    // it is in. Probably highly experimental.
    // --------------

    // Loop through the info blocks in this dialogue, and update the
    // master as necessary.

    // TODO: Note that the dialogue system in Morrowind isn't very
    // robust. If several mods insert dialogues at exactly the same
    // place, the mods loaded last might overwrite the previous mods,
    // completely removing the previous entry even if the two entries
    // do not have the same id. This is because a change also
    // overwrites the previous and the next entry, in order to update
    // their "previous" and "next" fields. Furthermore, we might put
    // ourselves in a situation where the forward and backward chains
    // do not match, or in a situation where we update a deleted
    // info. For now I do nothing about it, but I will have to develop
    // a "conflict manager" later on. It SHOULD be possible to merge
    // these info lists automatically in most cases, but it
    // complicates the code.

    // Whoa, seems we have a case study already with just tribunal and
    // bloodmoon loaded! See comments below.

    foreach(char[] id, ref DialInfoLoad m; mod.infoList)
      {
	// Remove the response if it is marked as deleted.
	if(m.deleted)
	  {
	    if((id in master.infoList) == null)
	      writefln("Cannot delete info %s, does not exist", id);
	    else master.infoList.remove(id);
	  }
	else
	  // Just plain copy it in.
	  master.infoList[id] = m;
      }
  }

  // Here we have to fix inconsistencies. A good case example is the
  // dialogue "Apelles Matius" in trib/blood. Trib creates a
  // dialogue of a few items, bloodmoon adds another. But since the
  // two are independent, the list in bloodmoon does not change the
  // one in trib but rather creates a new one. In other words, we
  // will have to deal with the possibility of several "independent"
  // lists within each topic. We can do this by looking for several
  // start points (ie. infos with prev="") and just latch them onto
  // each other. I'm not sure it gives the correct result,
  // though. For example, which list comes first would be rather
  // arbitrarily decided by the order we traverse the infoList AA. I
  // will just have to assume that they truly are "independent".

  // There still seems to be a problem though. Bloodmoon overwrites
  // some stuff added by Tribunal, see "Boots of the Apostle" for an
  // example. Looks like the editor handles it just fine... We need
  // to make sure that all the items in our AA are put into the
  // list, and in the right place too. We obviously cannot fully
  // trust the 'next' and 'prev' fields, but they are the only
  // guidance we have. Deal with it later!

  // At this point we assume "master" to contain the final dialogue
  // list, so at this point we can set it in stone.
  infoList.length = master.infoList.length;

  // Find the first entry
  DialInfoLoad* starts[]; // starting points for linked lists
  DialInfoLoad *current;
  foreach(char[] id, ref DialInfoLoad l; master.infoList)
    if(l.prev == "") starts ~= &l;

  foreach(int num, ref DialInfo m; infoList)
    {
      if(current == null)
	{
	  if(starts.length == 0)
	    {
	      writefln("Error: No starting points!");
	      infoList.length = num;
	      break;
	    }
	  // Pick the next starting point
	  current = starts[0];
	  starts = starts[1..$];
	}
      m.copy(*current, this);

      if((*current).next == "")
	current = null;
      else
	{
	  current = (*current).next in master.infoList;
	  if(current == null)
	    {
	      writefln("Error in dialouge info lookup!");
	      break;
	    }
	}
    }
  if(infoList.length != master.infoList.length)
    writefln("Dialogue list lengths do not match, %d != %d",
	     infoList.length, master.infoList.length);
  }
}
+/
