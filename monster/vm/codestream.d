/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (codestream.d) is part of the Monster script language
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

module monster.vm.codestream;

import monster.minibos.string;
import monster.minibos.stdio;
import monster.vm.error;
import monster.compiler.linespec;

// CodeStream is a simple utility structure for reading data
// sequentially. It holds a piece of byte compiled code, and keeps
// track of the position within the code.
struct CodeStream
{
  private:
  ubyte[] data;
  int len;
  ubyte *pos;

  // Position of the last instruction
  ubyte *cmdPos;

  // Used to convert position to the corresponding source code line,
  // for error messages.
  LineSpec[] lines;

  // Size of debug output
  const int preView = 50;
  const int perLine = 16;

  public:

  void setData(ubyte[] data,
               LineSpec[] lines)
  {
    this.data = data;
    this.lines = lines;
    len = data.length;
    pos = data.ptr;
  }

  // Called when the end of the stream was unexpectedly encountered
  void eos(char[] func)
  {
    char[] res = format("Premature end of input:\nCodeStream.%s() missing %s byte(s)\n",
			func, -len);

    res ~= debugString();

    fail(res);
  }

  char[] debugString()
  {
    int start = data.length - preView;
    if(start < 0) start = 0;

    char[] res = format("\nLast %s bytes of byte code:\n", data.length-start);
    foreach(int i, ubyte val; data[start..$])
      {
	if(i%perLine == 0)
	  res ~= format("\n 0x%-4x:   ", i+start);
	res ~= format("%-4x", val);
      }
    return res;
  }

  void debugPrint()
  {
    writefln(debugString());
  }

  // Jump to given position
  void jump(int newPos)
  {
    if(newPos<0 || newPos>=data.length)
      fail("Jump out of range");
    len = data.length - newPos;
    pos = &data[newPos];
  }

  // Get the current position
  int getPos()
  {
    return pos-data.ptr;
  }

  // Get the current line
  int getLine()
  {
    // call shared.linespec.findLine
    return findLine(lines, cmdPos-data.ptr);
  }

  ubyte get()
  {
    if(len--) return *(pos++);
    eos("get");
  }

  // Used for getting an instruction. It stores the offset which can
  // be used to infer the line number later.
  ubyte getCmd()
  {
    cmdPos = pos;
    return get();
  }

  int getInt()
  {
    len -= 4;
    if(len < 0) eos("getInt");
    int i = *(cast(int*)pos);
    pos+=4;
    return i;
  }

  // Get a slice of the 'size' next ints
  int[] getIntArray(uint size)
  {
    size *=4; // Convert size to bytes
    len -= size;
    if(len < 0) eos("getArray");
    int[] res = cast(int[])pos[0..size];
    pos += size;
    return res;
  }
}
