/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (assembler.d) is part of the Monster script language
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

module monster.compiler.assembler;

import std.string;
import std.stdio;

import monster.util.list;

import monster.compiler.bytecode;
import monster.compiler.linespec;
import monster.compiler.types;
import monster.compiler.tokenizer;

import monster.vm.error;

// Print the final output from assemble to stdout
//debug=printOutput;

// Element type
enum ET
  {
    None,               // Undefined
    OpCode,             // Instruction, variable number of bytes
    Data1,              // One byte of data
    Data4,              // One int of data
    Data8,              // Two ints of data
    Ptr,                // Data pointer
    Label,              // Jump label
    JumpPtr,            // Appears after a jump instruction, referes
                        // to a label.
    JumpIndex,          // Refers to a label by index, not by pointer
  }

// Represents one element in the code list.
struct CodeElement
{
  ET type;
  union
  {
    ubyte bytes[8];
    ubyte bdata;
    int idata;
    long ldata;
  }

  CodeElement *label; // Destination for jumps

  // Cumulative offset into the final code buffer
  int offset = -1;

  // Source code line corresponding to this instruction
  int line = -1;

  // Label number. This is different from the label / jump index,
  // which is a low-level reference. One label number is given to each
  // named label in state code. These are listed in a state label
  // export table in the compiled class file.
  int lnumber = -1;

  // Set the offset to val, return the offset of the next element (val
  // + size of this)
  uint setOffset(uint val)
  {
    offset = val;
    return val + getSize();
  }

  // Insert ourselves into the code block at the right offset.
  void insertCode(ubyte[] code)
  {
    // Zero-size elements don't have any code
    if(getSize() == 0) return;

    // Jumps are handled in a special way
    assert(type != ET.JumpIndex);
    if(type == ET.JumpPtr)
      {
        // Convert it back to a number, but this time the label offset
        type = ET.JumpIndex;
        idata = label.offset;
      }

    // Write the data
    int s = getSize();
    code[offset..offset+s] = bytes[0..s];
  }

  void print(LineSpec[] lines)
  {
    int index;
    PT ptype;

    // These start a new line
    if(type == ET.Label || type == ET.OpCode)
      {
        int fline = findLine(lines, offset);
        if(fline != line)
          writef("\n%s(%s): ", line, fline);
        else
          writef("\n%s: ", line);
      }

    switch(type)
      {
      case ET.Label:
        writef("label %s @ %s", idata, offset);
        break;

      case ET.OpCode:
        writef("%s", bcToString[idata]);
        break;

      case ET.Data1:
        writef(", byte ", idata);
        break;

      case ET.JumpPtr:
        writef(", ptr -> ", label.offset);
        break;

      case ET.JumpIndex:
        writef(", idx -> ", idata);
        break;

      case ET.Ptr:
        decodePtr(idata, ptype, index);
        writef(", ptr(%s, %s)", ptype, index);
        break;

      case ET.Data4:
        writef(", int %s", idata);
        break;

      case ET.Data8:
      case ET.None:
      default:
        assert(0);
      }
  }

  void setLabel(CodeElement *lab)
  {
    // Check that we only get called on jumps
    assert(type == ET.JumpIndex);

    // And check that the label actually matches this jump
    assert(lab.type == ET.Label);
    assert(lab.idata == idata);

    label = lab;
    type = ET.JumpPtr;
  }

  // Get the size of this element in the generated code. It does not
  // determine how much data is stored by this struct, or which
  // variable above it uses, only how big the result is in the
  // finished code buffer.
  int getSize()
  {
    switch(type)
      {
      case ET.Label:
        return 0;

      case ET.OpCode:
        assert(idata > 0);
        // Instructions are either one or two bytes. Single byte
        // instructions have values <= 255, everything else is coded
        // with two bytes.
        if(idata < 256) return 1;
        return 2;

      case ET.Data1:
        return 1;

      case ET.JumpPtr:
      case ET.JumpIndex:
      case ET.Ptr:
      case ET.Data4:
        return 4;

      case ET.Data8:
        return 8;

      case ET.None:
      default:
        assert(0);
      }
  }
}

Assembler tasm;

// A structure that takes a series of pseudo-assembler commands as
// input and stores them in a list of byte code instructions and data.
// We can perform various optimizations on this command list (not
// implemented yet) before storing it to file.
struct Assembler
{
  private:
  LinkedList!(CodeElement) codeList;

  int toInt(float f) { return *(cast(int*)&f); }

  // Add a new code element to the list
  void add(ET type, long data)
  {
    if(type == ET.OpCode && data > 255)
      {
        // Instruction values over 255 are stored as two bytes
        data -= 256;
        assert(data < 255);

        data <<= 8; // Move the instruction into the next byte
        data += BC.MultiByte; // Set the multi-byte indicator
      }

    CodeElement ce;
    ce.ldata = data;
    ce.type = type;
    ce.line = line;
    codeList ~= ce;
  }

  // Low level opcode functions
  void cmd(BC code) { add(ET.OpCode, code); }
  void addb(int i)
  {
    assert(i >= 0 && i < 256);
    add(ET.Data1, i);
  }
  void addib(int i)
  {
    assert(i >= -128 && i < 127);
    add(ET.Data1, i);
  }
  void addi(int i) { add(ET.Data4, i); }
  void addl(long l) { add(ET.Data8, l); }
  void label_add(int i) { add(ET.Label, i); }
  void jump_add(int i) { add(ET.JumpIndex, i); }

  // Add a pointer
  void addPtr(PT type, int index)
  { add(ET.Ptr, codePtr(type, index)); }

  // Current index in the table of jump points
  int jumpIndex;

  // Highest inserted label number
  int maxNumber;

  // Current line number
  int line;

  // Add a command that varies depending on a type size. If size is >
  // 1, it is added as an integer parameter to the instruction.
  void cmdmult(BC codeOne, BC codeMult, int size)
  {
    assert(size >= 1);
    if(size == 1) cmd(codeOne);
    else
      {
        cmd(codeMult);
        addi(size);
      }
  }

  // Like cmdmult, but size can only be 1 or 2, and the size isn't
  // added.
  void cmd2(BC codeOne, BC codeTwo, int size)
  {
    assert(size == 1 || size == 2);
    if(size == 1) cmd(codeOne);
    else cmd(codeTwo);
  }

  public:

  // Start a new function
  void newFunc()
  {
    codeList.reset();
    jumpIndex = 0;
    maxNumber = -1;
    line = -1;
  }

  // Set the current line. A line table is stored in the file, so
  // specific instructions can be translated to a line in the source
  // file. Mainly used for debugging.
  void setLine(int ln)
  {
    // Never overwrite a valid line with -1
    if(ln == -1) return;

    line = ln;
  }

  int getLine() { return line; }

  // Higher level opcode commands

  // Exit a function. params is the number of function parameters
  // and/or local variables to pop, ret is the return value size
  void exit(int params = 0, int ret = 0)
  {
    if(params)
      {
	if(ret == 1)
          {
            // Function has parameters and a return value of size 1
            cmd(BC.ReturnVal);
            addi(params);
          }
        else if(ret > 1)
          {
            // Return value larger than 1
            cmd(BC.ReturnValN);
            addi(params);
            addi(ret);
          }
	else
          {
            // Parameters, but no return value
            cmd(BC.Return);
            addi(params);
          }
        return;
      }

    // No parameters / local variables. The return value (if any) as
    // already on the top of the stack, so we can just exit quietly
    // and pass it along.
    cmd(BC.Exit);
  }

  void error(Err code)
  {
    cmd(BC.Error);
    addb(code);

    assert(code != 0);
  }

  void callFunc(int func, int cls, bool isFar)
  {
    if(isFar) cmd(BC.CallFar);
    else cmd(BC.Call);
    addi(cls);
    addi(func);
  }

  void callIdle(int func, int cls, bool isFar)
  {
    if(isFar) cmd(BC.CallIdleFar);
    else cmd(BC.CallIdle);
    addi(cls);
    addi(func);
  }

  void newObj(int i)
  {
    cmd(BC.New);
    addi(i);
  }

  void cloneObj() { cmd(BC.Clone); }

  // Copy the topmost int on the stack
  void dup() { cmd(BC.Dup); }

  /* All jump and label commands come in two versions, one for
     jump-before-label, and one for label-before-jump. Eg.:
    
     Label-before-jump:

     int l1 = tasm.label(); // Jump back here
     ...
     tasm.jumpz(l1);

     Jump-before-label:

     int l2 = tasm.jump();
     ...
     tasm.label(l2); // Jump forwards to here


     The following other cases are also currently possible:

     - Orphaned (backward) label:
       tasm.label() with no corresponding jump

     - Multiple backward jumps:
       a=tasm.label(); tasm.jump(a); tasm.jump(a); ...

     Other combinations are NOT allowed! However, these should be
     enough to implement more complex jump patterns like
     break/continue. The more advanced combinations are handled in
     LabelStatement and GotoStatement in statement.d
  */

  // Generic jump command to jump to a labed that is not yet
  // defined. Returns a jump index.
  int genjump(BC code)
  {
    cmd(code);

    // Add and return the current jump index. Then increase it for the
    // next jump.
    jump_add(jumpIndex);
    return jumpIndex++;
  }

  // Generic version to jump to an existing label index
  void genjump(BC code, int label)
  {
    cmd(code);
    jump_add(label);
  }

  // Jump if zero
  int jumpz() { return genjump(BC.JumpZ); }
  void jumpz(int i) { genjump(BC.JumpZ, i); }

  // Jump if non-zero
  int jumpnz() { return genjump(BC.JumpNZ); }
  void jumpnz(int i) { genjump(BC.JumpNZ, i); }

  // Just jump dammit!
  int jump() { return genjump(BC.Jump); }
  void jump(int i) { genjump(BC.Jump, i); }

  // Used for label before jump
  int label()
  {
    label_add(jumpIndex);
    return jumpIndex++;
  }

  // Label after jump
  void label(int i) { label_add(i); }

  // Assign a lnumber to the last label element (used by the functions
  // below)
  private void setLNumber(int number)
  {
    with(codeList.getTail().value)
      {
        // Some sanity checks
        assert(type == ET.Label);
        assert(lnumber == -1);

        // Set the label number
        lnumber = number;
      }

    // Remember the max number given
    if(number > maxNumber) maxNumber = number;
  }

  // Assign a number to a label (used for named labels, to get their
  // offset)
  int labelNum(int number)
  {
    int l = label();

    if(number != -1)
      setLNumber(number);

    return l;
  }

  void labelNum(int i, int number)
  {
    label(i);

    if(number != -1)
      setLNumber(number);
  }

  // Push the current stack position
  void getStack() { cmd(BC.GetStack); }

  // Variable and stack management:
  void push(int i)
  {
    cmd(BC.PushData);
    addi(i);
  }

  void push(float f) { push(*(cast(int*)&f)); }
  void push(uint u) { push(*(cast(int*)&u)); }

  union Share
  {
    double d;
    long l;
    ulong u;
    int ints[2];
  }

  void push8(long l)
  {
    Share s;
    s.l = l;
    pushArray(s.ints);
  }

  void push8(ulong u)
  {
    Share s;
    s.u = u;
    pushArray(s.ints);
  }

  void push8(double d)
  {
    Share s;
    s.d = d;
    pushArray(s.ints);
  }

  void pushArray(int[] data ...)
  {
    // TODO: We should use a push8 instruction or something for larger
    // data types, but that's optimization.
    foreach(int i; data)
      push(i);
  }

  //void pushArray(float[] data...) { pushArray(cast(int[])data); }

  // Push a value from the stack
  void pushLocal(int index, int siz)
  {
    assert(siz >= 1);
    assert(index >= 0 || index <= -siz);

    // Local variables are counted forwards from the current stack
    // frame pointer. The frame pointer is set whenever a new function
    // starts, that is AFTER the parameters have been pushed. Thus we
    // count parameters backwards from the stack pointer, and
    // variables forwards.

    // For multi-int variables, just push each int as a separate
    // variable. We can optimize this later.
    while(siz-- > 0)
      {
        cmd(BC.PushLocal);
        addi(index++);
      }
  }

  // Push a value from a class variable. The second parameter is the
  // number of ints to push.
  void pushClass(int index, int siz)
  {
    assert(siz >= 1);
    assert(index >= 0);

    while(siz-- > 0)
      {
        cmd(BC.PushClassVar);
        addi(index++);
      }
  }

  void pushParentVar(int index, int classIndex, int siz)
  {
    assert(index >= 0);
    assert(classIndex >= 0);
    assert(siz >= 1);

    while(siz-- > 0)
      {
        cmd(BC.PushParentVar);
        addi(classIndex);
        addi(index++);
      }
  }

  // Push a variable from another object
  void pushFarClass(int index, int classIndex, int siz)
  {
    assert(index >= 0);
    assert(classIndex >= 0);
    assert(siz >= 1);

    // The FAR versions of variables, assumes that the object
    // reference has already been pushed.
    cmdmult(BC.PushFarClassVar, BC.PushFarClassMulti, siz);
    addi(classIndex);
    addi(index++);
  }

  // Push 'this', the current object reference
  void pushThis() { cmd(BC.PushThis); }

  // Push the singleton object of the given class index
  void pushSingleton(int classIndex)
  {
    assert(classIndex >= 0);
    cmd(BC.PushSingleton);
    addi(classIndex);
  }

  private void pushPtr(PT pt, int index)
  {
    cmd(BC.PushData);
    addPtr(pt, index);
  }

  // Push addresses instead of values

  // Pointer to data segment of this object
  void pushClassAddr(int i) { pushPtr(PT.DataOffs, i); }

  // Imported local variable
  void pushParentVarAddr(int i, int classIndex)
  {
    assert(classIndex >= 0);

    // We have to push the class index FIRST. It will be popped
    // immediately after the pointer is decoded.
    push(classIndex);
    pushPtr(PT.DataOffsCls, i);
  }

  // Data segment of another object, but this class (use offset.)
  void pushFarClassAddr(int i, int classIndex)
  {
    assert(classIndex >= 0);
    push(classIndex);
    pushPtr(PT.FarDataOffs, i);
  }

  // Local stack variable
  void pushLocalAddr(int i) { pushPtr(PT.Stack, i); }

  // Push the address of an array element. The index and array are
  // already on the stack.
  void elementAddr() { pushPtr(PT.ArrayIndex,0); }

  // Convert an array index to a const reference
  void makeArrayConst() { cmd(BC.MakeConstArray); }

  // Check if an array is const
  void isArrayConst() { cmd(BC.IsConstArray); }

  void makeSlice() { cmd(BC.Slice); }

  // Pops the last 'len' elements off the stack and creates an array
  // from them. The element size (in ints) is given in the second
  // parameter.
  void popToArray(int len, int elemSize)
  {
    cmd(BC.PopToArray);
    addi(len*elemSize);
    addi(elemSize);
  }

  // Create a new array, initialized with the given value and with the
  // given dimension number. The lengths are popped of the stack.
  void newArray(int ini[], int rank)
  {
    cmd(BC.NewArray);
    addi(rank);
    addi(ini.length);
    foreach(i; ini) addi(i);
  }

  // Copies two array references from the stack, copies the data in
  // the last into the first. The (raw) lengths must match.
  void copyArray() { cmd(BC.CopyArray); }

  // Fills an array with a specific value.
  void fillArray(int size)
  {
    cmd(BC.FillArray);
    addi(size);
  }

  // Creates a copy of the array whose index is on the stack
  void dupArray() { cmd(BC.DupArray); }

  // Just remove a value from the stack (will later be optimized away
  // in most cases.)
  void pop(int i=1)
  {
    if(i == 0) return;
    if(i == 1) cmd(BC.Pop);
    else
      {
        cmd(BC.PopN);
        addb(i);
      }
  }

  void mov(int s)
  {
    if(s < 3) cmd2(BC.StoreRet, BC.StoreRet8, s);
    else cmdmult(BC.StoreRet, BC.StoreRetMult, s);
  }

  // Set object state to the given index
  void setState(int st, int label, int cls)
  {
    cmd(BC.State);
    addi(st);
    addi(label);
    addi(cls);
  }

  // Fetch an element from an array
  void fetchElement() { cmd(BC.FetchElem); }

  // Array concatination. The first concatinates two arrays, the
  // others adds a single element to the array from either side.
  void catArray() { cmd(BC.CatArray); }
  void catArrayLeft(int s) { cmd(BC.CatLeft); addi(s); }
  void catArrayRight(int s) { cmd(BC.CatRight); addi(s); }

  // Get the length of an array. Converts the array index to an int
  // (holding the length) on the stack.
  void getArrayLength() { cmd(BC.GetArrLen); }

  // Reverse an array in place
  void reverseArray() { cmd(BC.ReverseArray); }

  // Create an array iterator
  void createArrayIterator(bool isRev, bool isRef)
  {
    cmd(BC.CreateArrayIter);
    addb(isRev);
    addb(isRef);
  }

  // Create a class iterator
  void createClassIterator(int classNum)
  {
    cmd(BC.CreateClassIter);
    addi(classNum);
  }

  // Do the next iteration
  void iterateNext() { cmd(BC.IterNext); }

  // Break off iteration
  void iterateBreak() { cmd(BC.IterBreak); }

  void iterateUpdate(int pos)
  {
    if(pos < 0)
      fail("iterUpdate stack position must be positive");

    cmd(BC.IterUpdate);
    addi(pos);
  }

  // Value modifiers
  void add(Type t)
  {
    if(t.isInt || t.isUint) cmd(BC.IAdd);
    else if(t.isLong || t.isUlong) cmd(BC.LAdd);
    else if(t.isFloat) cmd(BC.FAdd);
    else if(t.isDouble) cmd(BC.DAdd);
    else assert(0);
  }

  void sub(Type t)
  {
    if(t.isInt || t.isUint) cmd(BC.ISub);
    else if(t.isLong || t.isUlong) cmd(BC.LSub);
    else if(t.isFloat) cmd(BC.FSub);
    else if(t.isDouble) cmd(BC.DSub);
    else assert(0);
  }

  void mul(Type t)
  {
    if(t.isInt || t.isUint) cmd(BC.IMul);
    else if(t.isLong || t.isUlong) cmd(BC.LMul);
    else if(t.isFloat) cmd(BC.FMul);
    else if(t.isDouble) cmd(BC.DMul);
    else assert(0);
  }

  void div(Type t)
  {
    if(t.isInt) cmd(BC.IDiv);
    else if(t.isUint) cmd(BC.UDiv);
    else if(t.isLong) cmd(BC.LDiv);
    else if(t.isUlong) cmd(BC.ULDiv);
    else if(t.isFloat) cmd(BC.FDiv);
    else if(t.isDouble) cmd(BC.DDiv);
    else assert(0);
  }

  void idiv(Type t)
  {
    if(t.isIntegral) div(t);
    else if(t.isFloat) cmd(BC.FIDiv);
    else if(t.isDouble) cmd(BC.DIDiv);
    else assert(0);
  }

  void divrem(Type t)
  {
    if(t.isInt) cmd(BC.IDivRem);
    else if(t.isUint) cmd(BC.UDivRem);
    else if(t.isLong) cmd(BC.LDivRem);
    else if(t.isUlong) cmd(BC.ULDivRem);
    else if(t.isFloat) cmd(BC.FDivRem);
    else if(t.isDouble) cmd(BC.DDivRem);
    else assert(0);
  }

  void neg(Type t)
  {
    if(t.isInt) cmd(BC.INeg);
    else if(t.isLong) cmd(BC.LNeg);
    else if(t.isFloat) cmd(BC.FNeg);
    else if(t.isDouble) cmd(BC.DNeg);
    else assert(0);
  }

  void incDec(TT op, bool postfix, int s)
  {
    if(op == TT.PlusPlus)
      if(postfix) cmd2(BC.PostInc, BC.PostInc8, s);
      else cmd2(BC.PreInc, BC.PreInc8, s);
    else if(op == TT.MinusMinus)
      if(postfix) cmd2(BC.PostDec, BC.PostDec8, s);
      else cmd2(BC.PreDec, BC.PreDec8, s);
    else
      assert(0, "Illegal op type");
  }

  // Type casting
  void castIntToLong(bool fromSign)
  {
    if(fromSign) cmd(BC.CastI2L);

    // Converting to unsigned long is as simple as setting zero as the
    // upper int. We don't need a separate instruction to do this.
    else push(0);
  }

  void castLongToInt() { pop(); }

  void castIntToFloat(Type fr, Type to)
  {
    assert(fr.isIntegral);
    assert(to.isFloating);
    if(to.isFloat)
      {
        if(fr.isInt) cmd(BC.CastI2F);
        else if(fr.isUint) cmd(BC.CastU2F);
        else if(fr.isLong) cmd(BC.CastL2F);
        else if(fr.isUlong) cmd(BC.CastUL2F);
        else assert(0);
      }
    else
      {
        assert(to.isDouble);
        if(fr.isInt) cmd(BC.CastI2D);
        else if(fr.isUint) cmd(BC.CastU2D);
        else if(fr.isLong) cmd(BC.CastL2D);
        else if(fr.isUlong) cmd(BC.CastUL2D);
        else assert(0);
      }
  }

  void castFloatToFloat(int fs, int ts)
  {
    if(fs == 1 && ts == 2) cmd(BC.CastF2D);
    else if(fs == 2 && ts == 1) cmd(BC.CastD2F);
    else assert(0);
  }

  // Cast a generic type to string
  void castToString(int tindex)
  {
    cmd(BC.CastT2S);
    addi(tindex);
  }
  
  // Boolean operators
  void isEqual(int s) { cmdmult(BC.IsEqual, BC.IsEqualMulti, s); }

  void isCaseEqual() { cmd(BC.IsCaseEqual); }

  void less(Type t)
  {
    if(t.isUint || t.isChar) cmd(BC.ULess);
    else if(t.isInt) cmd(BC.ILess);
    else if(t.isLong) cmd(BC.LLess);
    else if(t.isFloat) cmd(BC.FLess);
    else if(t.isUlong) cmd(BC.ULLess);
    else if(t.isDouble) cmd(BC.DLess);
  }

  void not() { cmd(BC.Not); }

  void cmpArray() { cmd(BC.CmpArray); }
  void icmpString() { cmd(BC.ICmpStr); }

  // Convert the current code list to byte code, and return it. The
  // lines parameter is filled with an array of line specifiers that
  // can be used to translate instruction position to a source code
  // line number. The offsets parameter, when not null, must be of
  // length maxNumber+1. It will be filled with the offsets of
  // numbered labels.
  ubyte[] assemble(ref LineSpec lines[], uint offsets[] = null)
  in
  {
    if(offsets.length != 0)
      assert(offsets.length == 1+maxNumber,
             "offset table must match number of named labels");
    else
      assert(maxNumber == -1,
             "assemble() called with named labels but nowhere to store offsets");
  }
  body
  {
    lines = null;

    // List of jumps, used for converting jump numbers to specific
    // labels.
    CodeElement* jumpList[];
    CodeElement* labelList[];

    jumpList.length = jumpIndex;
    labelList.length = jumpIndex;

    // Go through the code list and resolve jumps and labels
    foreach(ref CodeElement ce; codeList)
      {
        assert(ce.type != ET.JumpPtr);

        if(ce.type == ET.JumpIndex)
          {
            // Has the label been looked up?
            if(labelList[ce.idata] != null)
              // Set the jump pointer to replace the index
              ce.setLabel(labelList[ce.idata]);
            else
              {
                // Store the jump until the label is found. There can
                // only be one jump stored in each jumplist element.
                assert(jumpList[ce.idata] == null);
                jumpList[ce.idata] = &ce;
              }
            continue;
          }

        if(ce.type == ET.Label)
          {
            // Is there a jump that refers to this label?
            if(jumpList[ce.idata] != null)
              {
                // Make the jump point to this label.
                jumpList[ce.idata].setLabel(&ce);
                jumpList[ce.idata] = null;
              }
            else
              // Set the label up for later referal
              labelList[ce.idata] = &ce;
          }
      }

    // Do all low-level optimalizations here.

    // Finishing up. Go through the list and set the offsets and the
    // line specifications.
    uint offset = 0;
    LineSpec ls;
    ls.line = -2;
    ls.pos = -1;
    foreach(ref CodeElement ce; codeList)
      {
        // Only store line specs for opcodes
        if(ce.type == ET.OpCode &&
           ls.line != ce.line && offset != ls.pos)
          {
            // New line!
            ls.pos = offset;
            ls.line = ce.line;
            lines ~= ls;
          }
        
        offset = ce.setOffset(offset);
      }

    // Output the current code list
    debug(printOutput)
      {
        writefln("Compiled function:");
        foreach(CodeElement ce; codeList)
          ce.print(lines);
        writefln();
      }

    // Store the offsets of named labels in the 'offsets' table
    if(offsets.length > 0)
      {
        foreach(CodeElement ce; codeList)
          if(ce.lnumber != -1)
            offsets[ce.lnumber] = ce.offset;
      }

    // Set up the code list, fill it, and return it.
    ubyte[] code;
    code.length = offset;
    foreach(ref CodeElement ce; codeList)
      ce.insertCode(code);

    return code;
  }
}
