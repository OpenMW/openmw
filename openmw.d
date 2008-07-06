/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (openmw.d) is part of the OpenMW package.

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

module morro;

import std.stdio;
import std.string;
import std.cstream;

import ogre.ogre;
import ogre.bindings;

import scene.celldata;
import scene.soundlist;

import core.resource;
import core.memory;
import core.config;

import monster.util.string;

import sound.audio;

import input.events;

//*
import std.gc;
import gcstats;

void poolSize()
{
  GCStats gc;
  getStats(gc);
  writefln("Pool size: ", comma(gc.poolsize));
  writefln("Used size: ", comma(gc.usedsize));
}
//*/

void main(char[][] args)
{
  bool render = true;
  bool help = false;
  bool resetKeys = false;
  bool showOgreFlag = false;

  // Some examples to try:
  //
  // "Abandoned Shipwreck, Upper Level";
  // "Gro-Bagrat Plantation";
  // "Abinabi";
  // "Abebaal Egg Mine";
  // "Ald-ruhn, Ald Skar Inn";
  // "Koal Cave";
  // "Ald-ruhn, Arobar Manor Bedrooms"
  // "Sud";
  // "Vivec, The Lizard's Head";
  // "ToddTest";

  // Cells to load
  char[][] cells;
  int[] eCells;

  foreach(char[] a; args[1..$])
    if(a == "-n") render = false;
    else if(a.begins("-e"))
      {
	int i = find(a,',');
	eCells ~= atoi(a[2..i]);
	eCells ~= atoi(a[i+1..$]);
      }
    else if(a == "-h") help=true;
    else if(a == "-rk") resetKeys = true;
    else if(a == "-oc") showOgreFlag = true;
    else cells ~= a;

  if(cells.length + eCells.length/2 > 1 )
    {
      writefln("More than one cell specified, rendering disabled");
      render=false;
    }

  initializeMemoryRegions();
  config.initialize(resetKeys);
  scope(exit) config.writeConfig();

  // If the -oc parameter is specified, we override any config
  // setting.
  if(showOgreFlag) config.finalOgreConfig = true;

  if(cells.length == 0 && eCells.length == 0)
    if(config.defaultCell.length)
      cells ~= config.defaultCell;

  if(cells.length == 1)
    config.defaultCell = cells[0];

  if(help || (cells.length == 0 && eCells.length == 0))
    {
      writefln("Syntax: %s [options] cell-name [cell-name]", args[0]);
      writefln("  Options:");
      writefln("    -n            Only load, do not render");
      writefln("    -ex,y         Load exterior cell (x,y)");
      writefln("    -rk           Reset key bindings to default");
      writefln("    -oc           Show the Ogre config dialogue");  
      writefln("    -h            Show this help");
      writefln("");
      writefln("Specifying more than one cell implies -n");
      return;
    }

  initializeSound();
  resources.initResources();

  // Files to load
  /*
  const char[][] files = ["Morrowind.esm",
			  "Tribunal.esm",
			  "Bloodmoon.esm"];
  /*/
  const char[][] files = ["Morrowind.esm"];
  //*/

  // Add the path to the file name. The path is read from the config
  // file.
  char[][] esmFiles;
  esmFiles.length = files.length;
  foreach(int i, char[] ef; files)
    // TODO: Quick hack, use FileFinder instead.
    esmFiles[i] = config.esmDir ~ ef;

  // Load all data from the ESM files
  loadTESFiles(esmFiles);

  CellData cd = cellList.get();

  foreach(char[] cName; cells)
    {
      // Release the last cell data
      cellList.release(cd);

      // Get a cell data holder and load an interior cell
      cd = cellList.get();

      try cd.loadIntCell(cName);
      catch(Exception e)
	{
	  writefln(e);
	  writefln("\nUnable to load cell '%s'. Aborting", cName);
	  return;
	}
    }

  for(int i; i<eCells.length; i+=2)
    {
      int x = eCells[i];
      int y = eCells[i+1];

      // Release the last cell data
      cellList.release(cd);

      // Get a cell data holder and load an interior cell
      cd = cellList.get();

      writefln("Will load %s,%s", x, y);
      try cd.loadExtCell(x,y);
      catch(Exception e)
	{
	  writefln(e);
	  writefln("\nUnable to load cell (%s,%s). Aborting", x,y);
	  return;
	}
    }
	    
  // Simple safety hack
  NodePtr putObject(MeshIndex m, Placement *pos, float scale)
    {
      if(m == null)
	writefln("WARNING: CANNOT PUT NULL OBJECT");
      else if(m.isEmpty)
	writefln("WARNING: CANNOT INSERT EMPTY MESH '%s'", m.getName);
      else return placeObject(m, pos, scale);
      return null;
    }

  /*
  Sound *l = cast(Sound*) sounds.lookup("Fire 40");
  if(l)
    {
      writefln("id: %s", l.id);
      writefln("volume: ", l.data.volume);
      writefln("range: %s-%s", l.data.minRange, l.data.maxRange);
      writefln("sound file name: ", l.sound.getName);
      writefln("playing... press enter to quit");

      SoundInstance inst = SoundList.getInstance(l, true);
      inst.setPos(0,0,0);
      inst.setPlayerPos(0, 0, 0);
      inst.play();
      din.readLine();
      inst.kill;
      render = false;
    }
  */

  if(render)
    {
      // Warm up OGRE
      setupOgre();

      // Clean up ogre when we're finished.
      scope(exit) cleanupOgre();

      if(cd.inCell)
	{
	  setAmbient(cd.ambi.ambient, cd.ambi.sunlight,
		     cd.ambi.fog, cd.ambi.fogDensity);

	  // Not all interior cells have water
	  if(cd.inCell.flags & CellFlags.HasWater)
	    cpp_createWater(cd.water);
	}
      else
	{
	  Color c;
	  c.red = 180;
	  c.green = 180;
	  c.blue = 180;
	  setAmbient(c, c, c, 0);

	  // Put in the water
	  cpp_createWater(cd.water);

	  // Create an ugly sky
	  cpp_makeSky();
	}

      // TODO: We get some strange lamp-shaped activators in some scenes,
      // eg in Abebaal. These are sound activators (using scripts), but
      // they still appear. Find out if they have some special flags
      // somewhere (eg. only-show-in-editor), or if we just have to filter
      // them by the "Sound_*" name. Deal with it later.

      // Insert the meshes of statics into the scene
      foreach(ref LiveStatic ls; cd.statics)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      // Inventory lights
      foreach(ref LiveLight ls; cd.lights)
	{
	  NodePtr n = putObject(ls.m.model, &ls.base.pos, ls.base.scale);
	  ls.lightNode = attachLight(n, ls.m.data.color, ls.m.data.radius);
	  Sound *s = ls.m.sound;
	  if(s)
	    {
	      writefln("Dynamic light %s has sound %s", ls.m.id, s.id);
	      ls.loopSound = soundScene.insert(s, true);
	      if(ls.loopSound)
		ls.loopSound.setPos(ls.base.pos.position[0],
				    ls.base.pos.position[1],
				    ls.base.pos.position[2]);
	    }
	}
      // Static lights
      foreach(ref LiveLight ls; cd.statLights)
	{
	  NodePtr n = putObject(ls.m.model, &ls.base.pos, ls.base.scale);
	  ls.lightNode = attachLight(n, ls.m.data.color, ls.m.data.radius);
	  Sound *s = ls.m.sound;
	  if(s)
	    {
	      writefln("Static light %s has sound %s", ls.m.id, s.id);
	      ls.loopSound = soundScene.insert(s, true);
	      if(ls.loopSound)
		ls.loopSound.setPos(ls.base.pos.position[0],
				    ls.base.pos.position[1],
				    ls.base.pos.position[2]);
	    }
	}
      // Misc items
      foreach(ref LiveMisc ls; cd.miscItems)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      /*
      // NPCs (these are complicated, usually do not have normal meshes)
      foreach(ref LiveNPC ls; cd.npcs)
      putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      */
      // Containers
      foreach(ref LiveContainer ls; cd.containers)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      // Doors
      foreach(ref LiveDoor ls; cd.doors)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      // Activators
      foreach(ref LiveActivator ls; cd.activators)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      // Potions
      foreach(ref LivePotion ls; cd.potions)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      // Apparatus
      foreach(ref LiveApparatus ls; cd.appas)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      // Ingredients
      foreach(ref LiveIngredient ls; cd.ingredients)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      // Armors
      foreach(ref LiveArmor ls; cd.armors)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      // Weapons
      foreach(ref LiveWeapon ls; cd.weapons)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      // Books
      foreach(ref LiveBook ls; cd.books)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      // Clothes
      foreach(ref LiveClothing ls; cd.clothes)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      // Tools
      foreach(ref LiveTool ls; cd.tools)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);
      // Creatures (these often look like shit 
      foreach(ref LiveCreature ls; cd.creatures)
	putObject(ls.m.model, &ls.base.pos, ls.base.scale);

      // Initialize the internal input and event manager. The
      // lower-level input system (OIS) is initialized by the
      // setupOgre() call further up.
      initializeInput();

      // Start swangin'
      jukebox.enableMusic();

      // Run it until the user tells us to quit
      startRendering();
    }
  else debug(verbose) writefln("Skipping rendering");

  debug(verbose)
    {
      writefln();
      writefln("%d statics", cd.statics.length);
      writefln("%d misc items", cd.miscItems.length);
      writefln("%d inventory lights", cd.lights.length);
      writefln("%d static lights", cd.statLights.length);
      writefln("%d NPCs", cd.npcs.length);
      writefln("%d containers", cd.containers.length);
      writefln("%d doors", cd.doors.length);
      writefln("%d activators", cd.activators.length);
      writefln("%d potions", cd.potions.length);
      writefln("%d apparatuses", cd.appas.length);
      writefln("%d ingredients", cd.ingredients.length);
      writefln("%d armors", cd.armors.length);
      writefln("%d weapons", cd.weapons.length);
      writefln("%d books", cd.books.length);
      writefln("%d tools", cd.tools.length);
      writefln("%d clothes", cd.clothes.length);
      writefln("%d creatures", cd.creatures.length);
      writefln();
    }
  /*
  writefln("Statics:");
  foreach(ref s; cd.statics)
    {
      writefln("%s: %s", s.m.id, s.m.model.getName);
    }
  */

  // This isn't necessary but it's here for testing purposes.
  cellList.release(cd);

  // Write some memory statistics
  poolSize();
  writefln(esmRegion);
}
