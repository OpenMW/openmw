/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (vm.d) is part of the Monster script language package.

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

module monster.vm.vm;

import monster.vm.error;
import monster.vm.fstack;
import monster.vm.thread;
import monster.vm.mclass;
import monster.vm.mobject;

import monster.compiler.tokenizer;
import monster.compiler.linespec;
import monster.compiler.functions;
import monster.compiler.assembler;
import monster.compiler.scopes;

import std.file;
import monster.util.string;

VM vm;

struct VM
{
  // Run a script file in the context of the object obj. If no object
  // is given, an instance of an empty class is used.
  void run(char[] file, MonsterObject *obj = null)
  {
    if(obj !is null)
      {
        auto func = Function(file, obj.cls);
        func.call(obj);
      }
    else
      {
        auto func = Function(file);
        func.call();
      }
  }

  // Path to search for script files. Extremely simple at the moment.
  private char[][] includes = [""];

  void addPath(char[] path)
  {
    // Make sure the path is slash terminated.
    if(!path.ends("/") && !path.ends("\\"))
      path ~= '/';

    includes ~= path;
  }

  // Search for a file in the various paths. Returns true if found,
  // false otherwise. Changes fname to point to the correct path.
  bool findFile(ref char[] fname)
  {
    // Check against our include paths. In the future we will replace
    // this with a more flexible system, allowing virtual file systems,
    // archive files, complete platform independence, improved error
    // checking etc.
    foreach(path; includes)
      {
        char[] res = path ~ fname;
        if(exists(res))
          {
            fname = res;
            return true;
          }
      }

    return false;
  }

}
