/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (listkeeper.d) is part of the OpenMW package.

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

module esm.listkeeper;

import monster.util.aa;

import core.memory;

import esm.filereader;
import esm.defs;

import std.stdio;

// Item types, used in the lookup table for inventory items, creature
// lists and leveled lists. We also use it for all types of references
// that can exist in cells.
enum ItemType
  {
    // Items
    None = 0, Potion, Apparatus, Armor, Weapon, Book, Clothing,
    Light, Ingredient, Pick, Probe, Repair, Misc, ItemList,

    // Used for creature lists
    Creature, CreatureList, NPC,

    // Other cell references
    Door, Activator, Static, Container//, SoundGen
  }

abstract class ListKeeper
{
  int listIndex;

  new(uint size)
    { 
      return esmRegion.allocate(size).ptr;
    }

  delete(void *p) { assert(0); }

  this()
    {
      // Store our index for later use
      listIndex = recordLists.length;

      // Add the class to the global list
      recordLists ~= this;
    }

  // Load a record from a master or plugin file
  void load();

  // Looks up a reference. If it does not exist it is assumed to be a
  // forward reference within a file, and is inserted.
  void* lookup(char[] s);

  // Tell the loader that current file has ended, so it can do things
  // like check that all referenced objects have been loaded.
  void endFile();

  // Number of inserted elements
  uint length();

  void addToList(ref ItemBaseList l, ItemType t) { assert(0); }
}

ListKeeper recordLists[];

// Keep the list of Type structures for records where the first
// subrecord is an id string called NAME. This id is used for
// lookup. Although almost all lookups match in case, there are a few
// sounds that don't, so we treat these id lookups as generally case
// insensitive. This hasn't posed any problems so far.
class ListID(Type) : ListKeeper
{
  HashTable!(char[], Type, ESMRegionAlloc, CITextHash) names;

  this(uint size)
    {
      names = names.init;
      if(size) names.rehash(size);
    }

  // Reads the id for this header. Override if the id is not simply
  // getHNString("NAME")
  char[] getID()
    {
      return esFile.getHNString("NAME");
    }

  // Load a record from a master of plugin file
  void load()
    {
      assert(esFile.getFileType == FileType.Esm ||
	     esFile.getFileType == FileType.Esp);

      // Get the identifier of this record
      char[] id = getID();

      // Get pointer to a new or existing object.
      Type *p;
      if(names.insertEdit(id, p))
	// A new item was inserted
	{
	  p.state = LoadState.Unloaded;
	  p.id = id;
	  p.load();
	  p.state = LoadState.Loaded;
	}
      else
	// Item already existed, either from a previous file or as a
	// forward reference from this file. Load on top of it. The
	// LoadState tells the struct whether it contains loaded data.
	{
          /*
	  if(p.state == LoadState.Loaded)
	    // Make a special case for this, perhaps, or just ignore it.
	    writefln("WARNING: Duplicate record in file %s: '%s'",
		     esFile.getFilename(), id);
          */

	  assert(icmp(p.id, id) == 0);
	  p.load();
	  p.state = LoadState.Loaded;
	}
    }

  // Looks up a reference. If it does not exist it is assumed to be a
  // forward reference within a file, and is inserted.
  void* lookup(char[] id)
    {
      if(!id.length) return null; // Empty reference

      Type *p = names.lookup(id);
      // Is the value in the list?
      if(!p)
	// No, assume it is a forward reference.
	{
	  // Since the lookup name is stored in an internal buffer in
	  // esFile, we have to copy it.
	  id = esmRegion.copy(id);

	  // To avoid copying the string on every lookup, we have to
	  // insert in a separate step. But a double lookup isn't
	  // really THAT expensive. Besides, my tests show that this
	  // is used in less than 10% of the cases.
	  names.insertEdit(id, p);
	  p.id = id;
	  p.state = LoadState.Unloaded;
	}
      return cast(void*)p;
    }

  // Check that all referenced objects are actually loaded.
  void endFile()
  in
  {
    // We can skip this in release builds
    names.validate();
  }
  body
  {
    foreach(char[] id, ref Type t; names)
      // Current file is now counted as done
      if(t.state == LoadState.Loaded) t.state = LoadState.Previous;
      else if(t.state == LoadState.Unloaded)
	//writefln("WARNING: Unloaded reference " ~ id);
	esFile.fail("Unloaded reference " ~ id);
  }

  // Number of inserted elements
  uint length() {return names.length;}

  // Add the names in this list to an ItemList
  void addToList(ref ItemBaseList l, ItemType t)
    {
      foreach(char[] id, ref Type s; names)
	l.insert(id, &s, t);
    }
}

// A pointer to an item
struct ItemBase
{
  ItemType type;
  void *p;
}

struct ItemBaseList
{
  HashTable!(char[],ItemBase,ESMRegionAlloc) list;

  void addList(ItemBaseList l)
  {
    foreach(char[] id, ItemBase b; l.list)
      insert(id, b.p, b.type);
  }

  void addList(ListKeeper source, ItemType type)
  {
    source.addToList(*this, type);
  }

  void insert(char[] id, void* p, ItemType type)
  {
    ItemBase *b;
    if(!list.insertEdit(id, b))
      {
	//writefln("Replacing item ", id);
	if(b.type != ItemType.None)
	  esFile.fail("Replaced valid item: " ~ id);
      }
    //else writefln("Inserting new item ", id);

    b.type = type;
    b.p = p;
  }

  // Called at the end to check that all referenced items have been resolved
  void endMerge()
  {
    foreach(char[] id, ref ItemBase t; list)
      // Current file is now counted as done
      if(t.type == ItemType.None)
	// TODO: Don't use esFile.fail for this
	esFile.fail("ItemBaseList: Unresolved forward reference: " ~ id);
  }

  // Look up an item, return a pointer to the ItemBase representing
  // it. If it does not exist, it is inserted.
  ItemBase *lookup(char[] id)
  {
    if(!id.length) return null; // Empty reference
    ItemBase *b = list.lookup(id);
    // Is the value in the list?
    if(!b)
      // No, assume it is a forward reference.
      {
	// Since the lookup name is stored in an internal buffer in
	// esFile, we have to copy it.
	id = esmRegion.copy(id);

	// To avoid copying the string on every lookup, we have to
	// insert in a separate step. But a double lookup isn't
	// really THAT expensive.
	list.insertEdit(id, b);

	b.p = null;
	b.type = ItemType.None;
      }
    return b;
  }
}

// An item. Contains a reference to an ItemBase, which again is a
// reference to an item. The ItemBase might change after we have
// looked it up (for forward references), so we have to use a pointer.
struct Item
{
  ItemBase *i;

  void* getPtr(ItemType type)
  {
    if(i != null && i.type == type) return i.p;
    return null;
  }

  T* getType(T, ItemType Type)()
  {
    return cast(T*)getPtr(Type);
  }
}

struct ItemList
{
  private:
  ItemBaseList list;

  public:
  void addList(ItemList l)
    { list.addList(l.list); }

  void addList(ListKeeper source, ItemType type)
    { list.addList(source, type); }

  Item lookup(char[] id)
  {
    Item i;
    i.i = list.lookup(id);
    return i;
  }

  void endMerge()
    { list.endMerge(); }

  void endFile()
  in { list.list.validate(); }
  body {}

  void rehash(uint size)
    { list.list.rehash(size); }

  uint length() { return list.list.length(); }
}

// Aggregate lists, made by concatinating several other lists.
ItemList items;		// All inventory items, including leveled item lists
ItemList actors;	// All actors, ie. NPCs, creatures and leveled lists
ItemList cellRefs;	// All things that are referenced from cells
