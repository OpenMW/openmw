/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (options.d) is part of the Monster script language
  package.

  Monster is distributed as free software: you can redistribute it
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

module monster.options;

/*
  The purpose of this file is to set compile time options for the
  Monster library - including compiler, VM and modules. This allows
  the user to customize the language in various ways to fit each
  project individually.

  For changes to take effect, you must recompile and reinstall the
  library.

  If you have suggestions for additional options and ways to customize
  the language, let us know!
*/

const:
static:


/*********************************************************


                     Language options


 *********************************************************/

// Set to false to make the entire language case insensitive. Affects
// all identifier and keyword matching. (Not implemented yet!)
bool caseSensitive = true;

// Include the case-insensitive string (and character) operators =i=,
// =I=, !=i= and !=I=.
bool ciStringOps = true;

// Skip lines beginning with a hash character '#'
bool skipHashes = true;

// Do we allow implicit downcasting of classes? Downcasting means
// casting from a parent class to a child class. The actual object
// type is checked at runtime. In any case you can always downcast
// explicitly, using ClassName(obj).
bool implicitDowncast = true;

// Allow implicit conversion from float to int (and similar
// conversions). If false, you must use explicit casting,
// ie. int(value)
bool implicitTruncate = false;


/*********************************************************


                     VM options


 *********************************************************/

// Whether to add the current working directory to the VFS at
// startup. If false, you must add your own directories (using
// vm.addPath) or your own VFS implementations, otherwise the library
// will not be able to find any script files.
bool vmAddCWD = false;

// Maximum stack size. Prevents stack overflow through infinite
// recursion and other bugs.
int maxStack = 100;

// Maximum function stack size
int maxFStack = 100;

// Whether we should limit the number of instructions that execute()
// can run at once. Enabling this will prevent infinite loops.
bool enableExecLimit = true;

// Maximum number of instructions to allow in once call to execute() (if 
long execLimit = 10000000;


/*********************************************************


                     Debugging options


 *********************************************************/

// Enable tracing of external functions on the function stack. If
// true, you may use vm.trace() and vm.untrace() to add your own
// functions to the internal Monster function stack for debug purposes
// (the functions will show up in debug output.) If false, these
// functions will be empty and most likely optimized away completely
// by the D compiler (in release builds).
bool enableTrace = true;





/*********************************************************


                     Modules


 *********************************************************/

// Load modules at startup? If false, you can still load modules
// manually using monster.modules.all.initAllModules().
bool loadModules = true;

// List of modules to load when initAllModules is called (and at
// startup if loadModules is true.)
char[] moduleList = "io math timer frames random thread";





/*********************************************************


                     Timer module


 *********************************************************/

// When true, idle function sleep() uses the system clock. When false,
// the time is only updated manually when the user calls vm.frame().
// The system clock is fine for small applications, but the manual
// method is much more optimized. It is highly recommended to use
// vm.frame() manually for games and other projects that use a
// rendering loop.
bool timer_useClock = false;
