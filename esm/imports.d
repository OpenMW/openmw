module esm.imports;

/* This is a file that imports common modules used by the load*.d
   record loaders. It is really a cut down version of what used to be
   the start of records.d.

   This file MUST NOT import records.d - directly or indirectly -
   because that will trigger a nice three page long list of template
   forwarding errors from the compiler.

   What happens is that when DMD/GDC compiles one of the load* files,
   it is forced to read records.d first (since it is an imported
   module) - but then it sees a template that referes to a struct in
   the current load* file, before that struct is defined. Curriously
   enough, DMD has no problems when you specify all the source files
   on the command line simultaneously. This trick doesn't work with
   GDC though, and DSSS doesn't use it either.

   This file was created to work around this compiler bug.
*/

public
{
import esm.defs;
import esm.filereader;
import esm.listkeeper;

import core.resource;
import core.memory;

import util.regions;
import monster.util.aa;

import std.stdio;
import std.string;

alias RegionBuffer!(ENAMstruct) EffectList;

// Records that are cross referenced often
import esm.loadscpt;
import esm.loadsoun;
import esm.loadspel;
import esm.loadench;

import monster.monster;
}

