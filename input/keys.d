/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (keys.d) is part of the OpenMW package.

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

/*
 * This module handles keyboard and mouse button configuration
 */

module input.keys;

import std.string;
import std.stdio;

import input.ois;

// List of all functions we need to map to keys. If you add new keys,
// REMEMBER to add strings for them below as well. TODO: We should
// redo this entire section so that we insert actual functions into
// the system instead. That way, if we do not insert a function, the
// key gets treated as a "non-event" key. Then we will also force the
// definition of strings and function call to be in the same
// place. The Keys enum can be eliminated, really. Default keysyms can
// be added when the functions are inserted. But don't do anything
// until you know how this will interact with script code.
enum Keys
  {
    None = 0,

    // Movement
    MoveLeft, MoveRight,
    TurnLeft, TurnRight,
    MoveForward, MoveBackward,

    // Used eg. when flying or swimming
    MoveUp, MoveDown,

    // These are handled as events, while the above are not.
    FirstEvent,

    // Sound control
    MainVolUp, MainVolDown,
    MusVolUp, MusVolDown,
    SfxVolUp, SfxVolDown,
    Mute,

    // This will not be part of the finished product :)
    ToggleBattleMusic,
    Debug,

    // Misc
    Pause,
    ScreenShot,
    Exit,

    Length
  }

// List of keyboard-bound functions
char[][] keyToString;

// Lookup for keyboard key names. TODO: This is currently case
// sensitive, we should use my own AA to fix that.
int[char[]] stringToKeysym;

// Represents a key binding for a single function. Each function can
// have any number keys bound to it, including mouse buttons. This
// might be extended later to allow joystick buttons and other input
// devices.
struct KeyBind
{
  int syms[];

  // Does the given keysym match this binding?
  bool isMatch(int sym, char ch = 0)
  {
    assert(sym != 0, "don't send empty syms to isMatch");

    // We don't match characters yet
    if(sym == KC.CharOnly)
      return false;

    foreach(s; syms)
      if(sym == s) return true;
    return false;
  }

  // Does any of the given syms match this binding?
  bool isMatchArray(int arr[] ...)
  {
    foreach(i; arr)
      if(i!=0  && isMatch(i)) return true;
    return false;
  }

  // Assign key bindings to this structure. Can be called multiple
  // times or with multiple paramters (or an array) to bind multiple
  // keys.
  void bind(int symlist[] ...)
  {
    syms ~= symlist;
  }

  // Remove all bindings to this function
  void clear()
  {
    syms = null;
  }

  // Remove the given syms from this binding, if found.
  void remove(int symlist[] ...)
  {
    foreach(rs; symlist)
      // Just loop though all the syms and set matching values to
      // zero. isMatch() will ignore zeros.
      foreach(ref s; syms)
        if(s == rs) s = 0;
  }

  // Turn the keysym list into a comma separated list of key names
  char[] getString()
  {
    char[] res = null;
    bool notFirst = false;

    foreach(int k; syms)
      if(k != 0) // Ignore empty keysyms
        {
          if(notFirst) res ~= ",";
          else notFirst = true;

          res ~= keysymToString[k];
        }

    //writefln("getString returned %s", res);
    return res;
  }
}

KeyBindings keyBindings;

// This structure holds the bindings of all the functions
struct KeyBindings
{
  KeyBind[] bindings;

  // Bind the given function to the given key(s)
  void bind(Keys func, char[] key1, char[] key2 = "")
  {
    bind(func, getSym(key1), getSym(key2));
  }

  void bind(Keys func, int syms[] ...)
  {
    // Find other bindings that match this key
    foreach(int i, ref KeyBind kb; bindings)
      if(kb.isMatchArray(syms))
        kb.remove(syms);
    bindings[func].bind(syms);
  }

  // Find the function that matches the given keysym. We could
  // optimize this, but I'm not sure it's worth it.
  Keys findMatch(KC keysym, dchar ch)
  {
    int start=cast(int)Keys.FirstEvent + 1;
    foreach(int i, ref KeyBind kb; bindings[start..$])
      if( kb.isMatch(keysym, ch) )
        return cast(Keys)(i+start);
    return cast(Keys)0; // No match
  }

  static int getSym(char[] key)
  {
    key = strip(key);
    if(key.length)
      {
	int *p = key in stringToKeysym;
	if(p) return *p;
	else writefln("Warning: unknown key '%s'", key);
      }
    return 0;
  }

  // Bind a function to a comma-separated key list (intended to be
  // used directly with the ini file reader.)
  void bindComma(Keys func, char[] keys)
  {
    int index = keys.find(',');
    if(index != -1)
      {
        // Bind the first in the list
        bind(func, keys[0..index]);
        // Recurse on the rest
        bindComma(func, keys[index+1..$]);
      }
    // Last or only element in the list
    else bind(func, keys);
  }

  // Remove all key bindings
  void clear()
  {
    foreach(ref kb; bindings)
      kb.clear();
  }

  void initKeys()
  {
    // Keyboard functions
    keyToString.length = Keys.Length;

    keyToString[Keys.MoveLeft] = "Move Left";
    keyToString[Keys.MoveRight] = "Move Right";
    keyToString[Keys.TurnLeft] = "Turn Left";
    keyToString[Keys.TurnRight] = "Turn Right";
    keyToString[Keys.MoveForward] = "Move Forward";
    keyToString[Keys.MoveBackward] = "Move Backward";
    keyToString[Keys.MoveUp] = "Move Up";
    keyToString[Keys.MoveDown] = "Move Down";

    keyToString[Keys.MainVolUp] = "Increase Main Volume";
    keyToString[Keys.MainVolDown] = "Decrease Main Volume";
    keyToString[Keys.MusVolUp] = "Increase Music Volume";
    keyToString[Keys.MusVolDown] = "Decrease Music Volume";
    keyToString[Keys.SfxVolUp] = "Increase SFX Volume";
    keyToString[Keys.SfxVolDown] = "Decrease SFX Volume";
    keyToString[Keys.Mute] = "Mute Sound";

    keyToString[Keys.ToggleBattleMusic] = "Toggle Battle Music";
    keyToString[Keys.Debug] = "OGRE Test Action";

    keyToString[Keys.Pause] = "Pause";
    keyToString[Keys.ScreenShot] = "Screen Shot";
    keyToString[Keys.Exit] = "Quick Exit";
    //keyToString[Keys.] = "";

    bindings.length = Keys.Length;

    // Store all the key strings in a lookup-table
    foreach(int k, ref char[] s; keysymToString)
      if(s.length) stringToKeysym[s] = k;
  }
}
