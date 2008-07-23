/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (filereader.d) is part of the OpenMW package.

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

module esm.filereader;

private:
import std.stdio;
import std.stream;
import std.string;

import util.regions;
import monster.util.string;
import core.resource;

import esm.listkeeper;
import esm.defs;

public:

/*
 *  Exception class for TES3File
 */

class TES3FileException: Exception
{
  this(char[] msg) {super("Error reading TES3 file: " ~ msg);}
  this() {this("Unknown error");}
}

// Some flags are in use that we don't know. But we don't really know
// any of them.
enum RecordFlags : uint
  {
    Flag6 = 0x20,    // Eg. adventurers_v2.0.esp (only once per file?)
    Persistent = 0x400,
    Flag13 = 0x1000, // Eg. Astarsis_BR.esm (several times per file?)
    Blocked = 0x2000,

    Unknown = 0xffffffff - 0x3420
  }

enum FileType
  {
    Unknown,
    Esp, Plugin = Esp,
    Esm, Master = Esm,
    Ess, Savegame = Ess
  }

// Special files
enum SpecialFile
  {
    Other,
    Morrowind,
    Tribunal,
    Bloodmoon
  }

enum Version { Unknown, v12, v13 }

// This struct should contain enough data to put a TES3File object
// back into a specific file position and state. We use it to save the
// "position" of objects in a file (eg. a cell), so we can return
// there later and continue where we stopped (eg. when we want to load
// that specific cell.)
struct TES3FileContext
{
  char[] filename;
  uint leftRec, leftSub;
  ulong leftFile;
  NAME recName, subName;
  FileType type;
  Version ver;

  ulong filepos;
}

/**
 * Instance used to read TES3 files. Since we will only be reading one
 * file at a time, we might as well make one global instance.
 */
TES3File esFile;

/**
 * This struct reads an Elder Scrolls 3 file (esp, esm or ess)
 *
 * Makes heavy use of private variables to represent current
 * state.
 *
 * Relevant exceptions are
 * TES3FileException      - error interpreting file
 * StreamFileException    - file IO error
 */
struct TES3File
{
 private:
  BufferedFile file;// Input file

  // These are only used by getRecHeader and getSubHeader for
  // asserting the file's integrity.
  ulong leftFile;   // Number of unread bytes in file
  uint leftRec;    // Number of unread bytes in record

  // This is used by sub-record readers for integrity checking.
  uint leftSub;    // Number of bytes in subrecord

  // Name of current record and current sub-record.
  NAME recName, subName;

  char[] filename; // Filename
  FileType type;   // File type
  Version ver;     // File format version
  char[] author;   // File author (max 32 bytes (with null?))
  char[] desc;     // Description (max 256 bytes (ditto?))
  uint records;    // Number of records in the file (doesn't seem to be right?)
  SpecialFile spf; // Is this a file we have to treat in a special way?

  struct _mast
  {
    char[] name;   // File name of an esm master for this file
    ulong size;	   // The master file's size in bytes (used for
                   // version control)
  }

  // List of esm masters for this file. For savegames this list also
  // contains all plugins.
  _mast masters[];
  

  // TES3.HEDR, file header struct
  align(1) struct HEDRstruct
  {
    union
    {
      float ver;	// File format version, 1.2 and 1.3 supported.
      uint verHex;	// 1.2 = 0x3f99999a, 1.3 = 0x3fa66666
    }
    int type;		// 0=esp, 1=esm, 32=ess
    NAME32 author;	// Author's name
    NAME256 desc;	// File description blurb
    uint records;	// Number of records in file (?)
  }

  static assert(HEDRstruct.sizeof == 300);

  // Which memory region to use for allocations.
  RegionManager region;

 public:

  // A struct found in the headers of savegame files. Contains quick
  // information to get us going, like the cell name and the player
  // name.
  struct _saveData
  {
    float[6] unknown; // 24 bytes
    char[64] cell; // Cell name
    float unk2; // Unknown value
    char[32] player; // Player name
  }
  static assert(_saveData.sizeof == 124);
  _saveData saveData;

  // Get file information
  char[] getFilename() { return filename; }
  ulong getFileSize() { return file.size; }
  ulong getPosition() { return file.position; }
  SpecialFile getSpecial() { return spf; }

  char[] retSubName() { return subName; }

  bool isVer12() { return ver == Version.v12;}
  bool isVer13() { return ver == Version.v13;}
  FileType getFileType() { return type; }
  _mast[] getMasters() { return masters; }
  uint getRecords() { return records; }
  char[] getAuthor() { return author; }
  RegionManager getRegion() { return region; }

  // Store the current file state (position, file name, version, debug
  // info). The info should be enough to get us back on track for
  // reading from a file, without having to reread the header or any
  // previous records.
  void getContext(ref TES3FileContext c)
    {
      c.filename = filename;
      c.leftFile = leftFile;
      c.leftRec = leftRec;
      c.leftSub = leftSub;
      c.recName[] = recName;
      c.subName[] = subName;
      c.type = type;
      c.ver = ver;
      c.filepos = file.position;
    }

  // Opens the file if it is not already opened. A region manager has
  // to be specified.
  void restoreContext(TES3FileContext c, RegionManager r)
    {
      if(filename != c.filename)
	openFile(c.filename, r);
      file.seekSet(cast(long)c.filepos);

      // File is now open, copy state information
      filename = c.filename;
      leftFile = c.leftFile;
      leftRec = c.leftRec;
      leftSub = c.leftSub;
      recName[] = c.recName;
      subName[] = c.subName;
      type = c.type;
      ver = c.ver;
    }

  // Open a new file and assign a region
  private void openFile(char[] filename, RegionManager r)
  {
    close();
    debug writefln("Opening file");
    if(file is null) file = new BufferedFile(new File());
    file.open(filename);

    region = r;
  }

  void open(char[] filename, RegionManager r)
    {
      uint flags;

      debug writefln("openFile(%s, %s)", filename, r);
      openFile(filename, r);

      if(iEnds(filename, "Morrowind.esm")) spf = SpecialFile.Morrowind;
      else if(iEnds(filename, "Tribunal.esm")) spf = SpecialFile.Tribunal;
      else if(iEnds(filename, "Bloodmoon.esm")) spf = SpecialFile.Bloodmoon;
      else spf = SpecialFile.Other;

      debug writefln("Reading header");

      // Do NOT .dup this filename, since it is referenced outside the
      // GC's reach and might be deleted.
      this.filename = filename;

      leftFile = file.size;

      // First things first
      if(getRecName() != "TES3")
	fail("Not a valid Morrowind file");

      // Record header
      getRecHeader(flags);
      if(flags)
	writefln("WARNING: Header flags are non-zero");

      // Read and analyse the header data
      HEDRstruct hedr;
      readHNExact(&hedr, hedr.sizeof, "HEDR");

      // The float hedr.ver signifies the file format version. It can
      // take on these two values:
      // 0x3f99999a = 1.2
      // 0x3fa66666 = 1.3
      if( hedr.verHex == 0x3f99999a )
	ver = Version.v12;
      else if( hedr.verHex == 0x3fa66666 )
	ver = Version.v13;
      else
	{
	  ver = Version.Unknown;
	  writefln("WARNING: Unknown version: ", hedr.ver);
	  writefln("  Hex: %X h", *(cast(uint*)&hedr.ver));
	}

      switch(hedr.type)
	{
	case 0: type = FileType.Esp; break;
	case 1: type = FileType.Esm; break;
	case 32: type = FileType.Ess; break;
	default:
	  type = FileType.Unknown;
	  writefln("WARNING: Unknown file type: ", hedr.type);
	}

      author = region.copy(stripz(hedr.author));
      desc = region.copy(stripz(hedr.desc));
      records = hedr.records;

      masters = null;
      // Reads a MAST and a DATA fields
      while(isNextSub("MAST"))
	{
	  _mast ma;

	  // MAST entry - master file name
	  ma.name = getHString();

	  // DATA entry - master file size
	  ma.size = getHNUlong("DATA");

	  // Add to the master list!
	  masters ~= ma;
	}

      if(type == FileType.Savegame)
	{
          // Savegame-related data

          // Cell name, player name and player position
          readHNExact(&saveData, 124, "GMDT");

          // Contains eg. 0xff0000, 0xff00, 0xff, 0x0, 0x20. No idea.
	  getSubNameIs("SCRD");
	  skipHSubSize(20);

          // Screenshot. Fits with 128x128x4 bytes
	  getSubNameIs("SCRS");
	  skipHSubSize(65536);
	}
    }

  // Close the file. We do not clear any object data at this point.
  void close()
    {
      debug writefln("close()");
      if(file !is null)
	file.close();
      leftFile = leftRec = leftSub = 0;
      debug writefln("Clearing strings");

      recName[] = '\0';
      subName[] = '\0';

      // This tells restoreContext() that we have to reopen the file
      filename = null;

      debug writefln("exit close()");
    }

  /*
   * Error reporting
   */

  void fail(char[] msg)
    {
      throw new TES3FileException
	(msg ~ "\nFile: " ~ filename ~ "\nRecord name: " ~ recName
	 ~ "\nSubrecord name: " ~ subName);
    }

  /************************************************************************
   *
   * Highest level readers, reads a name and looks it up in the given
   * list.
   *
   ************************************************************************/

  // This should be more than big enough for references.
  private char lookupBuffer[200];

  // Get a temporary string. This is faster and more memory efficient
  // that the other string functions (because it is allocation free),
  // but the returned string is only valid until tmpHString() is
  // called again.
  char[] tmpHString()
  {
    getSubHeader();
    assert(leftSub <= lookupBuffer.length, "lookupBuffer wasn't large enough");

    // Use this to test the difference in memory consumption.
    return getString(lookupBuffer[0..leftSub]);
  }

  // These are used for file lookups
  MeshIndex getMesh()
  { getSubNameIs("MODL"); return resources.lookupMesh(tmpHString()); }
  SoundIndex getSound()
  { getSubNameIs("FNAM"); return resources.lookupSound(tmpHString()); }
  IconIndex getIcon(char[] s = "ITEX")
  { getSubNameIs(s); return resources.lookupIcon(tmpHString()); }
  TextureIndex getTexture()
  { getSubNameIs("DATA"); return resources.lookupTexture(tmpHString()); }

  // The getO* functions read optional records. If they are not
  // present, return null.

  MeshIndex getOMesh()
  { return isNextSub("MODL") ? resources.lookupMesh(tmpHString()) : MeshIndex.init; }
  /*
  SoundIndex getOSound()
  { return isNextSub("FNAM") ? resources.lookupSound(tmpHString()) : SoundIndex.init; }
  */
  IconIndex getOIcon()
  { return isNextSub("ITEX") ? resources.lookupIcon(tmpHString()) : IconIndex.init; }
  TextureIndex getOTexture(char[] s="TNAM")
  { return isNextSub(s) ? resources.lookupTexture(tmpHString()) : TextureIndex.init; }

  // Reference with name s
  template getHNPtr(Type)
  {
    Type* getHNPtr(char[] s, ListKeeper list)
      { getSubNameIs(s); return cast(Type*) list.lookup(tmpHString()); }
  }

  // Reference, only get header
  template getHPtr(Type)
  {
    Type* getHPtr(ListKeeper list)
      { return cast(Type*) list.lookup(tmpHString()); }
  }

  // Optional reference with name s
  template getHNOPtr(Type)
  {
    Type* getHNOPtr(char[] s, ListKeeper list)
      { return isNextSub(s) ? cast(Type*)list.lookup(tmpHString()) : null; }
  }

  /************************************************************************
   *
   *  Somewhat high level reading methods. Knows about headers and
   *  leftFile/leftRec/leftSub.
   *
   ************************************************************************/

  // "Automatic" versions. Sets and returns recName and subName and
  // updates leftFile/leftRec.
  char[] getRecName()
    {
      if(!hasMoreRecs())
	fail("No more records, getRecName() failed");
      getName(recName);
      leftFile-= 4;
      return recName;
    }

  // This is specially optimized for LoadINFO
  bool isEmptyOrGetName()
    {
      if(leftRec)
	{
	  file.readBlock(subName.ptr, 4);
	  leftRec -= 4;
	  return false;
	}
      return true;
    }

  // I've tried to optimize this slightly, since it gets called a LOT.
  void getSubName()
    {
      if(leftRec <= 0)
	fail("No more sub-records, getSubName() failed");

      // Don't bother with error checking, we will catch an EOF upon
      // reading the subrecord data anyway.
      file.readBlock(subName.ptr, 4);

      leftRec -= 4;
    }

  // We often expect a certain subrecord type, this makes it easy to
  // check.
  void getSubNameIs(char[] s)
    {
      getSubName();
      if( subName != s )
	fail("Expected subrecord "~s~" but got "~subName);
    }

  // Checks if the next sub-record is called s. If it is, run
  // getSubName, if not, return false.
  bool isNextSub(char[] s)
    {
      if(!leftRec) return false;

      getName(subName);
      if(subName != s)
	{
	  file.seekCur(-4);
	  return false;
	}
      leftRec -= 4;

      //getSubName();
      return true;
    }

  // Same as isNextSub, only it works on records instead of
  // sub-records. It also loads the record header.
  bool isNextHRec(char[] s)
    {
      if(!leftFile) return false;
      getName(recName);
      if(recName != s)
	{
	  file.seekCur(-4);
	  return false;
	}
      leftFile -= 4;

      uint flags;
      getRecHeader(flags);

      return true;
    }

  bool hasMoreSubs() { return leftRec > 0; }
  bool hasMoreRecs() { return leftFile > 0; }

  // Remaining size of current record
  uint getRecLeft() { return leftRec; }
  // Size of current sub record
  uint getSubSize() { return leftSub; }

  // Skip the rest of this record
  void skipRecord()
    {
      file.seekCur(leftRec);
      leftRec = 0;
    }

  // Skip current sub record and return size
  uint skipHSub()
    {
      getSubHeader();
      file.seekCur(leftSub);
      return leftSub;
    }

  // Skip sub record and check it's size
  void skipHSubSize(uint size)
    {
      getSubHeader();
      if(leftSub != size)
	fail(format("Size mismatch: got %d, wanted %d", leftSub, size));
      file.seekCur(leftSub);
    }

  // These read an entire sub-record, including the header. They also
  // adjust and check leftSub and leftRecord variables through calling
  // getSubHeader().
  void readHExact(void * p, uint size)
    {
      getSubHeader();
      if(leftSub != size)
	fail(format("Size mismatch: got %d, wanted %d", leftSub, size));
      readExact(p, leftSub);
    }

  template TgetHType(T)
    { T TgetHType() { T t; readHExact(&t, t.sizeof); return t;} }

  // To make these easier to use (and to further distinguish them from
  // the above "raw" versions), these return their value instead of
  // using an ref argument.
  alias TgetHType!(uint) getHUint;
  alias TgetHType!(int) getHInt;
  alias TgetHType!(float) getHFloat;
  alias TgetHType!(ulong) getHUlong;
  alias TgetHType!(byte) getHByte;

  // Reads a string sub-record, including header
  char[] getHString()
    {
      getSubHeader();

      // Hack to make MultiMark.esp load. Zero-length strings do not
      // occur in any of the official mods, but MultiMark makes use of
      // them. For some reason, they break the rules, and contain a
      // byte (value 0) even if the header says there is no data. If
      // Morrowind accepts it, so should we.
      if(leftSub == 0)
	{
	  // Skip the following zero byte
	  leftRec--;
	  assert(file.getc() == 0);
	  // TODO: Report this by setting a flag or something?
	  return null;
	}

      return getString(region.getString(leftSub));
    }

  // Other quick aliases (this is starting to get messy)
  // Get string sub record string with name s
  char[] getHNString(char[] s)
    { getSubNameIs(s); return getHString(); }

  // Get optional sub record string with name s
  char[] getHNOString(char[] s)
    { return isNextSub(s) ? getHString() : null; }

  template TgetHNType(T)
    { T TgetHNType(char[] s) { T t; readHNExact(&t, t.sizeof, s); return t;} }

  template TgetHNOType(T)
    {
      T TgetHNOType(char[] s, T def)
	{
	  if(isNextSub(s))
	    {
	      T t;
	      readHExact(&t, t.sizeof);
	      return t;
	    }
	  else return def;
	}
    }

  alias TgetHNType!(uint) getHNUint;
  alias TgetHNType!(int) getHNInt;
  alias TgetHNType!(float) getHNFloat;
  alias TgetHNType!(ulong) getHNUlong;
  alias TgetHNType!(byte) getHNByte;
  alias TgetHNType!(short) getHNShort;
  alias TgetHNType!(byte) getHNByte;

  alias TgetHNOType!(float) getHNOFloat;
  alias TgetHNOType!(int) getHNOInt;
  alias TgetHNOType!(byte) getHNOByte;

  void readHNExact(void* p, uint size, char[] s)
    { getSubNameIs(s); readHExact(p,size); }

  // Record header
  // This updates the leftFile variable BEYOND the data that follows
  // the header, ie beyond the entire record. You are supposed to use
  // the leftRec variable when reading record data.
  void getRecHeader(out uint flags)
    {
      // General error checking
      if(leftFile < 12)
	fail("End of file while reading record header");
      if(leftRec)
	fail(format("Previous record contains %d unread bytes", leftRec));

      getUint(leftRec);
      getUint(flags);// This header entry is always zero
      assert(flags == 0);
      getUint(flags);
      leftFile -= 12;

      // Check that sizes add up
      if(leftFile < leftRec)
	fail(format(leftFile, " bytes left in file, but next record contains ",
		    leftRec," bytes"));

      // Adjust number of bytes left in file
      leftFile -= leftRec;
    }

  // Sub-record head
  // This updates leftRec beyond the current sub-record as
  // well. leftSub contains size of current sub-record.
  void getSubHeader()
    {
      if(leftRec < 4)
	fail("End of record while reading sub-record header");

      if(file.readBlock(&leftSub, 4) != 4)
	fail("getSubHeader could not read header length");

      leftRec -= 4;

      // Adjust number of record bytes left
      leftRec -= leftSub;

      // Check that sizes add up
      if(leftRec < 0)
	fail(format(leftRec+leftSub,
	     " bytes left in record, but next sub-record contains ",
	      leftSub," bytes"));
    }

  void getSubHeaderIs(uint size)
    {
      getSubHeader();
      if(leftSub != size)
	fail(format("Expected header size to be ", size, ", not ", leftSub));
    }

  /*************************************************************************
   *
   *  Low level reading methods
   *
   *************************************************************************/

  /// Raw data of any size
  void readExact(void *buf, uint size)
    {
      assert(size != 0);
      file.readExact(buf,size);
    }

  // One byte
  void getByte(out byte b) { file.read(b); }
  void getUByte(out ubyte b) { file.read(b); }
  // Two bytes
  void getUShort(out ushort s) { file.read(s); }
  // Four bytes
  void getUint(out uint u) { file.read(u); }
  void getInt(out int i) { file.read(i); }
  void getFloat(out float f) { file.read(f); }
  // Eight bytes
  void getUlong(out ulong l) { file.read(l); }

  // Get a record or subrecord name, four bytes
  void getName(NAME name)
    {
      file.readBlock(name.ptr, 4);
      /*
      if(file.readBlock(name.ptr, 4) != 4)
	fail("getName() could not find more data");
      */
    }

  // Fill buffer of predefined length. If actual string is shorter
  // (ie. null terminated), the buffer length is set
  // accordingly. Chopped string is returned. All strings pass through
  // this function, so any character encoding conversions should
  // happen here.
  char[] getString(char[] str)
    {
      if(str.length != file.readBlock(str.ptr,str.length))
	fail("getString() could not find enough data in stream");

      str = stripz(str);
      makeUTF8(str); // TODO: A hack. Will replace non-utf characters
                     // with question marks. This is neither a very
                     // desirable result nor a very optimized
                     // implementation of it.
      return str;
    }

  // Use this to allocate and read strings of predefined length
  char[] getString(int l)
  {
    char[] str = region.getString(l);
    return getString(str);
  }
}
