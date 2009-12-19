/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (inifile.d) is part of the OpenMW package.

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

module core.inifile;

import std.stdio;
import std.stream;
import std.string;
import std.path;
import std.file;

import monster.util.string;
import monster.util.aa;

void safeMkdir(char[] npath)
{
  char curDir[];
  int index = 0;
  int srch;

  do
    {
      while(npath[index..$].begins("/"))
        index++;

      srch = npath[index..$].find('/');

      if(srch == -1)
        curDir = npath;
      else
        {
          index += srch;
          curDir = npath[0..index];
        }
      index++;

      if(!exists(curDir) || !isdir(curDir))
        mkdir(curDir);
    }
  while(srch != -1)
}

// Writes an ini file.
struct IniWriter
{
  File ini;
  bool firstSection = true;

  void openFile(char[] file)
  {
    if(ini is null) ini = new File();
    char[] oldFile = file~".old";

    if(exists(file))
      copy(file, oldFile);

    // Make sure the output directory exists
    char[] dr = getDirName(file);
    if(dr.length)
      safeMkdir(dr);

    ini.open(file, FileMode.OutNew);
  }

  void close()
  {
    ini.close();
  }

  void section(char[] name)
  {
    if(firstSection)
      firstSection = false;
    else
      ini.writefln();

    ini.writefln("[%s]", name);
  }

  void comment(char[] str)
  {
    ini.writefln("; %s", str);
  }

  void writeType(T)(char[] name, T t)
  {
    ini.writefln("%s=%s", name, t);
  }

  alias writeType!(int) writeInt;
  alias writeType!(char[]) writeString;

  void writeFloat(char[] name, float f)
  {
    ini.writefln("%s=%.3s", name, f);
  }

  void writeBool(char[] name, bool b)
  {
    ini.writefln("%s=%s", name, b?"yes":"no");
  }
}

// Keytype that holds section name and variable name
struct SecVar
{
  private:
  char[] section;
  char[] variable;

  public:

  bool opEquals(ref SecVar v)
  {
    return section == v.section && variable == v.variable;
  }

  uint toHash()
  {
    const auto ID = typeid(char[]);
    return ID.getHash(&section) + ID.getHash(&variable);
  }

  static SecVar opCall(char[] sec, char[] var)
  {
    SecVar sv;
    sv.section = sec;
    sv.variable = var;
    return sv;
  }

  char[] toString()
  {
    return "[" ~ section ~ "]:" ~ variable;
  }
}

struct IniReader
{
  private:
  HashTable!(SecVar, char[]) vars;
  char[] section;

  // Start a new section
  void setSection(char[] sec)
  {
    section = sec.dup;
  }

  // Insert a new value from file
  void set(char[] variable, char[] value)
  {
    vars[SecVar(section, variable.dup)] = value.dup;
  }

  public:

  void reset()
  {
    vars.reset();
    section = null;
    wasRead = false;
  }

  int getInt(char[] sec, char[] var, int def)
  {
    char[] value;
    if(vars.inList(SecVar(sec,var), value))
      return cast(int)atoi(value);
    else
      return def;
  }

  float getFloat(char[] sec, char[] var, float def)
  {
    char[] value;
    if(vars.inList(SecVar(sec,var), value))
      return atof(value);
    else
      return def;
  }

  char[] getString(char[] sec, char[] var, char[] def)
  {
    char[] value;
    if(vars.inList(SecVar(sec,var), value))
      return value;
    else
      return def;
  }

  // Return true if the string matches some case of 'yes', and false
  // otherwise.
  bool getBool(char[] sec, char[] var, bool def)
  {
    char[] value;
    if(vars.inList(SecVar(sec,var), value))
      return icmp(value, "yes") == 0;
    else
      return def;
  }

  bool wasRead = false;

  void readFile(char[] fn)
  {
    // Reset this struct
    reset();

    // If the file doesn't exist, simply exit. This will work fine,
    // and default values will be used instead.
    if(!exists(fn)) return;

    wasRead = true;

    // Read buffer. Finite in size but perfectly safe - the readLine
    // routine allocates more mem if it needs it.
    char[300] buffer;

    scope File ini = new File(fn);
    while(!ini.eof)
      {
	char[] line = ini.readLine(buffer);

	// Remove leading and trailing whitespace
	line = strip(line);

	// Ignore comments and blank lines
	if(line.length == 0 || line.begins(";")) continue;

	// New section?
	if(line.begins("["))
	  {
	    if(!line.ends("]"))
	      {
		//writefln("Malformed section: %s", line);
		continue;
	      }

	    setSection(line[1..$-1]);
	    continue;
	  }

	// Split line into a key and a value
	int index = line.find('=');
	if(index != -1)
	  {
	    char[] value = line[index+1..$];
	    line = line[0..index];
	    set(line, value);
	  }
	//else writefln("Malformed value: '%s'", line);
      }
  }
}
