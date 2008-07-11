/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (resource.d) is part of the OpenMW package.

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

module core.resource;

import std.stdio;
import std.string;
import std.stream;
import std.file;
import std.path;

import monster.util.aa;
import monster.util.string;
import util.random;

import bsa.bsafile;

import core.memory;
import core.config;

import ogre.bindings;
import ogre.meshloader;

import sound.audio;
import sound.sfx;

import nif.nif;

import core.filefinder;
//import core.config;

// Random number generator
DRand rnd;

// These are handles for various resources. They may refer to a file
// in the file system, an entry in a BSA archive, or point to an
// already loaded resource. Resource handles that are not implemented
// yet are typedefed as ints for the moment.
typedef int IconIndex;

alias SoundResource* SoundIndex;
alias TextureResource* TextureIndex;
alias MeshResource* MeshIndex;

ResourceManager resources;

// Called from ogre/cpp_bsaarchive.cpp. We will probably move these
// later.
extern(C)
{
  // Does the file exist in the archives?
  int d_bsaExists(char *filename)
    {
      char[] name = toString(filename);

      auto res = resources.lookupTexture(name);
      if(res.bsaFile != -1)
        return 1;

      return 0;
    }

  // Open a file. Return the pointer and size.
  void d_bsaOpenFile(char *filename,
                     void **retPtr, uint *retSize)
    {
      char[] name = toString(filename);
      void[] result;

      //writefln("calling d_bsaOpenFile(%s, %s)", bsaFile, name);

      auto tex = resources.lookupTexture(name);

      if(tex.bsaFile == -1) result = null;
      else result = resources.archives[tex.bsaFile].findSlice(tex.bsaIndex);
      *retPtr = result.ptr;
      *retSize = result.length;
    }
}

struct ResourceManager
{
  private:
  // Holds lists of resources in the file system.
  FileFinder
    meshes,
    icons,
    textures,
    sounds,
    bookart,
    bsa, esm, esp;

  // Archives
  BSAFile archives[];

  // List of resources that have already been looked up (case
  // insensitive)
  HashTable!(char[], MeshIndex, ESMRegionAlloc, CITextHash) meshLookup;
  HashTable!(char[], TextureIndex, ESMRegionAlloc, CITextHash) textureLookup;
  HashTable!(char[], SoundIndex, ESMRegionAlloc, CITextHash) soundLookup;

  public:

  // Hack. Set to true in esmtool to disable all the lookup*
  // functions.
  bool dummy = false;

  void initResources()
  {
    rnd = new DRand;

    bsa = new FileFinder(config.bsaDir, "bsa", Recurse.No);
    archives.length = bsa.length;
    foreach(int i, ref BSAFile f; archives)
      f = new BSAFile(bsa[i+1]);

    sounds = new FileFinder(config.sndDir);

    // Simple playlists, similar to the Morrowind one. Later I imagine
    // adding an interactive MP3 player with a bit more finesse, but
    // this will do for now.
    char[][] music;

    char[][] getDir(char[] dir)
      {
	dir = FileFinder.addSlash(dir);
	char[][] res = listdir(dir);
	foreach(ref char[] fn; res)
	  fn = dir ~ fn;
	return res;
       }

    jukebox.setPlaylist(getDir(config.musDir));
    battleMusic.setPlaylist(getDir(config.musDir2));

    meshLookup.reset();
    textureLookup.reset();
    soundLookup.reset();

    meshBuffer[0..7] = "meshes\\";
    texBuffer[0..9] = "textures\\";
  }

  // These three functions are so similar that I should probably split
  // out big parts of them into one common function.
  SoundIndex lookupSound(char[] id)
  {
    if(dummy) return null;

    assert(id != "", "loadSound called with empty id");

    SoundIndex si;

    if( soundLookup.inList(id, si) ) return si;

    si = esmRegion.newT!(SoundResource);

    // Check if the file exists
    int index = sounds[id];

    // If so, get the real file name
    if(index) si.file = sounds[index];
    // Otherwise, make this an empty resource
    else
      {
        writefln("Lookup failed to find sound %s", id);
        si.file = null;
      }

    si.res.loaded = false;

    // Copy name and insert. We MUST copy here, since indices during
    // load are put in a temporary buffer, and thus overwritten.
    si.name = esmRegion.copyz(id);
    soundLookup[si.name] = si;

    return si;
  }

  // Quick but effective hack
  char[80] meshBuffer;

  MeshIndex lookupMesh(char[] id)
  {
    if(dummy) return null;

    MeshIndex mi;

    // If it is already looked up, return the result
    if( meshLookup.inList(id, mi) ) return mi;

    mi = esmRegion.newT!(MeshResource);

    // If not, find it. For now we only check the BSA.
    char[] search;
    if(id.length < 70)
      {
	// Go to great lengths to avoid the concat :) The speed gain
	// is negligible (~ 1%), but the GC memory usage is HALVED!
	// This may mean significantly fewer GC collects during a long
	// run of the program.
	meshBuffer[7..7+id.length] = id;
	search = meshBuffer[0..7+id.length];
      }
    else
      search = "meshes\\" ~ id;

    //writefln("lookupMesh(%s): searching for %s", id, search);

    mi.bsaIndex = -1;
    mi.bsaFile = -1;
    foreach(int ind, BSAFile bs; archives)
      {
	mi.bsaIndex = bs.getIndex(search);
	if(mi.bsaIndex != -1) // Found something
	  {
	    mi.bsaFile = ind;
	    break;
	  }
      
      }

    if(mi.bsaIndex == -1)
      {
        writefln("Lookup failed to find mesh %s", search);
        assert(mi.bsaFile == -1);
      }

    // Resource is not loaded
    mi.node = null;

    // Make a copy of the id
    mi.name = esmRegion.copyz(id);
    meshLookup[mi.name] = mi;

    return mi;
  }

  char[80] texBuffer;

  TextureIndex lookupTexture(char[] id)
  {
    if(dummy) return null;

    TextureIndex ti;

    // Checked if we have looked it up before
    if( textureLookup.inList(id, ti) ) return ti;

    // Create a new resource locator
    ti = esmRegion.newT!(TextureResource);

    ti.name = esmRegion.copyz(id);
    ti.newName = ti.name;
    ti.type = ti.name[$-3..$];

    char tmp[];
    if(id.length < 70)
      {
	// See comment in lookupMesh
	texBuffer[9..9+id.length] = id;
	tmp = texBuffer[0..9+id.length];
      }
    else
      tmp = "textures\\" ~ id;

    void searchBSAs(char[] search)
      {
	// Look it up in the BSA
	ti.bsaIndex = -1;
	ti.bsaFile = -1;
	foreach(int ind, BSAFile bs; archives)
	  {
	    ti.bsaIndex = bs.getIndex(search);
	    if(ti.bsaIndex != -1) // Found something
	      {
		ti.bsaFile = ind;
		break;
	      }
	  }
      }

    searchBSAs(tmp);

    // If we can't find it, try the same filename but with .dds as the
    // extension. Bethesda did at some point convert all their
    // textures to dds to improve loading times. However, they did not
    // update their esm-files or require them to use the correct
    // extention (if they had, it would have broken a lot of user
    // mods). So we must support files that are referenced as eg .tga
    // but stored as .dds.
    if(ti.bsaIndex == -1 && ti.type != "dds")
      {
	tmp[$-3..$] = "dds";
	searchBSAs(tmp);
	if(ti.bsaIndex != -1)
          {
            // Store the real name in newName.
            ti.newName = esmRegion.copyz(ti.name);

            // Get a slice of the extension and overwrite it.
            ti.type = ti.newName[$-3..$];
            ti.type[] = "dds";
          }
      }

    // Check that extensions match, to be on the safe side
    assert(ti.type == ti.newName[$-3..$]);

    if(ti.bsaIndex == -1)
      {
        writefln("Lookup failed to find texture %s", tmp);
        assert(ti.bsaFile == -1);
      }

    ti.ml = null;

    textureLookup[ti.name] = ti;

    return ti;
  }

  IconIndex lookupIcon(char[] id) { return -3; }

  // Inserts a given mesh into ogre. Currently only reads the BSA
  // file. Is only called from within MeshResource itself, and should
  // never be called when the mesh is already loaded.
  private void loadMesh(MeshIndex mi)
    in
    {
      assert(!mi.isLoaded);
      assert(!mi.isEmpty);
    }
    body
    {
      // Get a slice of the mesh
      void[] s = archives[mi.bsaFile].findSlice(mi.bsaIndex);

      // Load the NIF into memory. No need to call close(), the file is
      // automatically closed when the data is loaded.
      nifMesh.open(s, mi.name);

      // Load and insert nif
      // TODO: Might add BSA name to the handle name, for clarity
      mi.node = meshLoader.loadMesh(mi.name);

      // TODO: We could clear the BSA memory mapping here to free some
      // mem
    }

}

struct SoundResource
{
  private:
  char[] name;
  char[] file;
  SoundFile res;

  public:
  char[] getName() { return name; }

  SoundInstance getInstance()
  in
  {
    assert(!isEmpty());
  }
  body
  {
    if(!isLoaded())
      res.load(file);

    return res.getInstance();
  }

  bool isEmpty() { return file == ""; }
  bool isLoaded() { return res.loaded; }
}

struct MeshResource
{
  private:
  char[] name;
  int bsaFile;
  int bsaIndex;

  // Points to the 'template' SceneNode of this mesh. Is null if this
  // mesh hasn't been inserted yet.
  NodePtr node;

  public:

  NodePtr getNode()
  in
  {
    assert(!isEmpty());
  }
  body
  {
    if(node == null) resources.loadMesh(this);
    return node;
  }

  char[] getName() { return name; }

  // Returns true if this resource does not exist (ie. file not found)
  // TODO: This must be modified later for non-BSA files.
  bool isEmpty()
  {
    return bsaIndex == -1;
  }

  // Returns true if resource is loaded
  bool isLoaded()
  {
    return node != null;
  }
}

struct TextureResource
{
  private:
  char[] name;
  char[] newName; // Converted name, ie. with extension converted to
                  // .dds if necessary
  int bsaFile;	// If set to -1, the file is in the file system
  int bsaIndex;
  char[] type;  // Texture format, eg "tga" or "dds";
  ManualLoader ml;

  public:

  char[] getName() { return name; }
  char[] getNewName() { return newName; }

  // Returns true if this resource does not exist (ie. file not found)
  bool isEmpty()
  {
    return bsaIndex == -1;
  }

  // Returns true if resource is loaded
  bool isLoaded()
  {
    return ml != null;
  }
}

// OLD STUFF

/+

  void initResourceManager()
    {
      // Find all resource files
      char[] morroDir = config.morrowindDirectory();
      bsa = new FileFinder(morroDir, "bsa");

      // Jump to the data files directory
      morroDir = config.dataFilesDirectory();

      esm = new FileFinder(morroDir, "esm", Recurse.No);
      esp = new FileFinder(morroDir, "esp", Recurse.No);

      meshes = new FileFinder(morroDir ~ "Meshes");
      icons = new FileFinder(morroDir ~ "Icons");
      textures = new FileFinder(morroDir ~ "Textures");
      sounds = new FileFinder(morroDir ~ "Sound");
      bookart = new FileFinder(morroDir ~ "BookArt");

      char[][] bsas = config.bsaArchives();
      archives.length = bsas.length;
      writef("Loading BSA archives...");

      writefln(" Done\n");
    }

  void killBSAs()
    {
      foreach(BSAFile f; archives)
	delete f;
    }
}
+/
