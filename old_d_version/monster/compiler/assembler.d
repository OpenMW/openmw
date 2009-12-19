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
import monster.options;

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
    StackPtr,           // Reference to a stack value - is replaced
                        // with a normal Ptr during compilation
    StackMark,          // A marked stack position
    MarkRef,            // Reference to a previously marked stack position
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
  // named label in state code, and these are used to set up state
  // offsets correctly.
  int lnumber = -1;

  // Represents the stack imprint of this element. May be positive,
  // zero or negative.
  int imprint;

  // Current stack position BEFORE this instruction is executed. This
  // is the cumulative imprints of all the elements before us.
  int stackPos = -1;

  // Counts the number of jumps to this label. Labels with zero
  // ingoing jumps are deleted.
  int jumps = 0;

  // Set the offset to val, return the offset of the next element (val
  // + size of this)
  uint setOffset(uint val)
  {
    offset = val;
    return val + getSize();
  }

  bool isCmd(BC ops[]...)
  {
    if(type != ET.OpCode) return false;

    foreach(op; ops)
      if(idata == op) return true;

    return false;
  }

  bool isStateLabel()
  { return type == ET.Label && lnumber != -1; }

  // Set the stack position, returns the next stack position
  int setStack(int stack)
  {
    if(stackPos != -1)
      assert(stack == stackPos, "Stack mismatch detected");

    stackPos = stack;
    return stack + imprint;
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

  // A principal element is one that may own other elements behind it,
  // and it is not owned by any other element.
  bool isPrincipal()
  {
    return
      type == ET.Label ||
      type == ET.OpCode ||
      type == ET.StackMark;
  }

  void print(LineSpec[] lines)
  {
    int index;
    PT ptype;

    // These start a new line
    if(isPrincipal())
      {
        int fline = findLine(lines, offset);
        if(fline != line)
          writef("\n%s(%s) ", line, fline);
        else
          writef("\n%s ", line);
        writef("stk=%s(%s): ", stackPos, imprint);
      }

    switch(type)
      {
      case ET.Label:
        writef("label %s @ %s", idata, offset);
        if(jumps == 0) writef(" (inactive)");
        break;

      case ET.OpCode:
        writef("%s", bcToString[idata]);
        break;

      case ET.Data1:
        writef(", byte ", idata);
        break;

      case ET.JumpPtr:
        writef(" -> label %s @ %s", label.idata, label.offset);
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

  void setMark(CodeElement *mrk)
  {
    assert(type == ET.MarkRef);
    assert(mrk !is null);
    assert(mrk.type == ET.StackMark);

    label = mrk;
    mrk.jumps++;
  }

  void setLabel(CodeElement *lab)
  {
    // Check that we only get called on jumps
    assert(type == ET.JumpIndex);

    // And check that the label actually matches this jump
    assert(lab !is null);
    assert(lab.type == ET.Label);
    assert(lab.idata == idata);

    label = lab;
    lab.jumps++;
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
        assert(0, "not used yet");
        return 2;

      case ET.Data1:
        return 1;

      case ET.JumpPtr:
      case ET.JumpIndex:
      case ET.Ptr:
      case ET.StackPtr:
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

  // Add an opcode, with optional stack imprint
  void cmd(BC code, int stk=0)
  {
    add(ET.OpCode, code);
    if(stk != 0) setStack(stk);
  }

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

  // Current index in the table of jump points
  int jumpIndex;

  // Highest inserted label number
  int maxNumber;

  // Current line number
  int line;

  // Add a command that varies depending on a type size. If size is >
  // 1, it is added as an integer parameter to the instruction.
  void cmdmult(BC codeOne, BC codeMult, int size, int stk=0)
  {
    assert(size >= 1);
    if(size == 1) cmd(codeOne, stk);
    else
      {
        cmd(codeMult, stk);
        addi(size);
      }
  }

  // Like cmdmult, but size can only be 1 or 2, and the size isn't
  // added.
  void cmd2(BC codeOne, BC codeTwo, int size, int stk=0)
  {
    assert(size == 1 || size == 2);
    if(size == 1) cmd(codeOne, stk);
    else cmd(codeTwo, stk);
  }

  // Set the stack imprint of the last element in the code list
  void setStack(int imp)
  {
    assert(codeList.length > 0);

    with(codeList.getTail().value)
      {
        // Only allowed for opcodes
        assert(type == ET.OpCode);
        imprint = imp;
      }
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
  void exit(int params, int locals, int ret)
  {
    // Calculate the stack imprint. Notice that we omit the params,
    // since they are irrelevant for this function (outside the
    // current 'frame', and we want to end up at stack level zero.
    int imprint = -locals -ret;

    // Count the parameters and locals as one from here on.
    params += locals;

    if(params)
      {
	if(ret == 1)
          {
            // Function has parameters and a return value of size 1
            cmd(BC.ReturnVal, imprint);
            addi(params);
          }
        else if(ret > 1)
          {
            // Return value larger than 1
            cmd(BC.ReturnValN, imprint);
            addi(params);
            addi(ret);
          }
	else
          {
            // Parameters, but no return value
            cmd(BC.Return, imprint);
            addi(params);
          }
        return;
      }

    // No parameters / local variables. The return value (if any) is
    // already on the top of the stack, so we can just exit quietly
    // and pass it along.
    cmd(BC.Exit, imprint);
  }

  void error(Err code)
  {
    cmd(BC.Error);
    addb(code);

    assert(code != 0);
  }

  void call(bool isFar, int imprint)
  {
    // The stack imprint from the instruction itself must be added to
    // the imprint from the function call (which removes parameters
    // but pushes a return value onto the stack.)
    if(isFar) cmd(BC.CallFar, -2 + imprint);
    else cmd(BC.Call, -1 + imprint);
  }

  void newObj(int clsIndex, int params, int imprint)
  {
    // The instruction removes all the pushed parameters, and pushes
    // an object index.
    cmd(BC.New, 1-imprint);
    addi(clsIndex);
    addi(params);
  }

  void cloneObj() { cmd(BC.Clone); }

  // Copy the topmost int on the stack
  void dup() { cmd(BC.Dup, 1); }

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

     Other combinations are NOT allowed! However, these are enough to
     implement more complex jump patterns like break/continue. The
     more advanced combinations are handled in LabelStatement and
     GotoStatement in statement.d
  */

  // Generate the jump opcode, and set the stack adjustment
  // accordingly
  private void jumpOp(BC code)
  {
    cmd(code);

    // JumpZ and JumpNZ pop a value from the stack
    if(code != BC.Jump)
      setStack(-1);
  }

  // Generic jump command to jump to a labed that is not yet
  // defined. Returns a jump index.
  private int genjump(BC code)
  {
    jumpOp(code);

    // Add and return the current jump index. Then increase it for the
    // next jump.
    jump_add(jumpIndex);
    return jumpIndex++;
  }

  // Generic version to jump to an existing label index
  private void genjump(BC code, int label)
  {
    jumpOp(code);
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
  void getStack() { cmd(BC.GetStack, 1); }

  // Variable and stack management
  void push(int i)
  {
    cmd(BC.PushData, 1);
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
    // TODO: We should use a push8 instructions or similar for larger
    // data types, but that's optimization.
    foreach(int i; data)
      push(i);
  }

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
        cmd(BC.PushLocal, 1);
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
        cmd(BC.PushClassVar, 1);
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
        cmd(BC.PushParentVar, 1);
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
    cmdmult(BC.PushFarClassVar, BC.PushFarClassMulti, siz, siz-1);
    addi(classIndex);
    addi(index++);
  }

  // Push 'this', the current object reference
  void pushThis() { cmd(BC.PushThis, 1); }

  // Push the singleton object of the given class index
  void pushSingleton(int classIndex)
  {
    assert(classIndex >= 0);
    cmd(BC.PushSingleton, 1);
    addi(classIndex);
  }

  // Push a coded pointer, consisting of the type pt and the given
  // index.
  private void pushPtr(PT pt, int index)
  {
    cmd(BC.PushData, 1);
    // Add the coded pointer as the pushed data
    add(ET.Ptr, codePtr(pt, index));
  }

  // Push addresses instead of values

  // Pointer to data segment of this object
  void pushClassAddr(int i)
  {
    pushArray(0,0);
    pushPtr(PT.DataOffs, i);
  }

  // Imported local variable
  void pushParentVarAddr(int i, int classIndex)
  {
    assert(classIndex >= 0);

    push(0);
    push(classIndex);
    pushPtr(PT.DataOffsCls, i);
  }

  // Data segment of another object, but this class (use offset.) The
  // object index is already on the stack
  void pushFarClassAddr(int i, int classIndex)
  {
    assert(classIndex >= 0);
    push(classIndex);
    pushPtr(PT.FarDataOffs, i);
  }

  // Local stack variable
  void pushLocalAddr(int i)
  {
    pushArray(0,0);
    cmd(BC.PushData, 1);
    add(ET.StackPtr, i);
  }

  CodeElement *markStack()
  {
    add(ET.StackMark, 0);
    return &codeList.getTail().value;
  }

  void pushMark(CodeElement *mark)
  {
    assert(mark !is null);
    cmd(BC.PushLocal, 1);
    add(ET.MarkRef, 1);
    codeList.getTail().value.setMark(mark);
  }

  // Get the function part of a function reference (currently just a
  // string containing the name)
  void refFunc() { cmd(BC.RefFunc, -1); }

  // Push the address of an array element. The index and array are
  // already on the stack.
  void elementAddr() { pushPtr(PT.ArrayIndex,0); }

  // Convert an array index to a const reference
  void makeArrayConst() { cmd(BC.MakeConstArray); }

  // Check if an array is const
  void isArrayConst() { cmd(BC.IsConstArray); }

  void makeSlice() { cmd(BC.Slice, -2); }

  // Pops the last 'len' elements off the stack and creates an array
  // from them. The element size (in ints) is given in the second
  // parameter.
  void popToArray(int len, int elemSize)
  {
    cmd(BC.PopToArray, 1-len*elemSize);
    addi(len*elemSize);
    addi(elemSize);
  }

  // Create a new array, initialized with the given value and with the
  // given dimension number. The lengths are popped of the stack.
  void newArray(int ini[], int rank)
  {
    cmd(BC.NewArray, 1-rank);
    addi(rank);
    addi(ini.length);
    foreach(i; ini) addi(i);
  }

  // Copies two array references from the stack, copies the data in
  // the last into the first. The (raw) lengths must match.
  void copyArray() { cmd(BC.CopyArray, -2); }

  // Fills an array with a specific value.
  void fillArray(int size)
  {
    cmd(BC.FillArray, -size-1);
    addi(size);
  }

  // Creates a copy of the array whose index is on the stack
  void dupArray() { cmd(BC.DupArray); }

  // Just remove a value from the stack (will later be optimized away
  // in most cases.)
  void pop(int i=1)
  {
    if(i == 0) return;
    if(i == 1) cmd(BC.Pop, -1);
    else
      {
        cmd(BC.PopN, -i);
        addb(i);
      }
  }

  void mov(int s)
  {
    // A Store* removes a pointer (3 ints) plus the value itself (s
    // ints)
    if(s < 3) cmd2(BC.Store, BC.Store8, s, -s-3);
    else cmdmult(BC.Store, BC.StoreMult, -s-3);
  }

  // Set object state to the given index
  void setState(int st, int label, int cls)
  {
    cmd(BC.State);
    addi(st);
    addi(label);
    addi(cls);
  }

  // Get the given field of an enum. If field is -1, get the "value"
  // field.
  void getEnumValue(Type type, int field=-1)
  {
    assert(type.isEnum, "given type is not an enum");

    auto et = cast(EnumType)type;
    assert(et !is null);

    if(field == -1)
      cmd(BC.EnumValue, 1);
    else
      {
        assert(field >= 0 && field <= et.fields.length);

        // Get the number of bytes we're pushing onto the stack
        int size = et.fields[field].type.getSize();

        cmd(BC.EnumField, size-1);
        addi(field);
      }
    addi(type.tIndex);
  }

  void enumValToIndex(int tIndex)
  {
    assert(Type.typeList[tIndex].isEnum,
           "given type index is not an enum");
    cmd(BC.EnumValToIndex, -1);
    addi(tIndex);
  }

  void enumNameToIndex(int tIndex)
  {
    assert(Type.typeList[tIndex].isEnum,
           "given type index is not an enum");
    cmd(BC.EnumNameToIndex);
    addi(tIndex);
  }

  // Fetch an element from an array
  void fetchElement(int s) { cmd(BC.FetchElem, -2+s); }

  // Array concatination. The first concatinates two arrays, the
  // others adds a single element to the array from either side.
  void catArray() { cmd(BC.CatArray, -1); }
  void catArrayLeft(int s) { cmd(BC.CatLeft, -s); addi(s); }
  void catArrayRight(int s) { cmd(BC.CatRight, -s); addi(s); }

  // Get the length of an array. Converts the array index to an int
  // (holding the length) on the stack.
  void getArrayLength() { cmd(BC.GetArrLen); }

  // Reverse an array in place
  void reverseArray() { cmd(BC.ReverseArray); }

  // Create an array iterator
  void createArrayIterator(bool isRev, bool isRef)
  {
    cmd(BC.CreateArrayIter, 1);
    addb(isRev);
    addb(isRef);
  }

  // Create a class iterator
  void createClassIterator(int classNum)
  {
    cmd(BC.CreateClassIter, 2);
    addi(classNum);
  }

  // Do the next iteration
  void iterateNext() { cmd(BC.IterNext, 1); }

  // Break off iteration
  void iterateBreak() { cmd(BC.IterBreak); }

  void iterateUpdate(int pos)
  {
    assert(pos >= 0);
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
    setStack(-t.getSize());
  }

  void sub(Type t)
  {
    if(t.isInt || t.isUint) cmd(BC.ISub);
    else if(t.isLong || t.isUlong) cmd(BC.LSub);
    else if(t.isFloat) cmd(BC.FSub);
    else if(t.isDouble) cmd(BC.DSub);
    else assert(0);
    setStack(-t.getSize());
  }

  void mul(Type t)
  {
    if(t.isInt || t.isUint) cmd(BC.IMul);
    else if(t.isLong || t.isUlong) cmd(BC.LMul);
    else if(t.isFloat) cmd(BC.FMul);
    else if(t.isDouble) cmd(BC.DMul);
    else assert(0);
    setStack(-t.getSize());
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
    setStack(-t.getSize());
  }

  void idiv(Type t)
  {
    if(t.isIntegral) div(t);
    else if(t.isFloat) cmd(BC.FIDiv);
    else if(t.isDouble) cmd(BC.DIDiv);
    else assert(0);
    setStack(-t.getSize());
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
    setStack(-t.getSize());
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
    // Each instruction pops a ptr (3 ints) and pushes a result (s
    // ints)
    if(op == TT.PlusPlus)
      if(postfix) cmd2(BC.PostInc, BC.PostInc8, s, s-3);
      else cmd2(BC.PreInc, BC.PreInc8, s, s-3);
    else if(op == TT.MinusMinus)
      if(postfix) cmd2(BC.PostDec, BC.PostDec8, s, s-3);
      else cmd2(BC.PreDec, BC.PreDec8, s, s-3);
    else
      assert(0, "Illegal op type");
  }

  // Type casting
  void castStrToChar() { cmd(BC.CastS2C); }

  void castIntToLong(bool fromSign)
  {
    if(fromSign) cmd(BC.CastI2L, 1);

    // Converting to unsigned long is as simple as setting zero as the
    // upper int. We don't need a separate instruction to do this.
    else push(0);
  }

  void castLongToInt() { pop(); }

  void castFloatToInt(Type fr, Type to)
  {
    assert(fr.isFloating);
    assert(to.isIntegral);
    if(fr.isFloat)
      {
        if(to.isInt) cmd(BC.CastF2I);
        else if(to.isUint) cmd(BC.CastF2U);
        else if(to.isLong) cmd(BC.CastF2L);
        else if(to.isUlong) cmd(BC.CastF2UL);
        else assert(0);
      }
    else
      {
        assert(fr.isDouble);
        if(to.isInt) cmd(BC.CastD2I);
        else if(to.isUint) cmd(BC.CastD2U);
        else if(to.isLong) cmd(BC.CastD2L);
        else if(to.isUlong) cmd(BC.CastD2UL);
        else assert(0);
      }
    setStack(to.getSize() - fr.getSize());
  }

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
    setStack(to.getSize() - fr.getSize());
  }

  void castFloatToFloat(int fs, int ts)
  {
    if(fs == 1 && ts == 2) cmd(BC.CastF2D);
    else if(fs == 2 && ts == 1) cmd(BC.CastD2F);
    else assert(0);
    setStack(ts-fs);
  }

  // Cast a generic type to string
  void castToString(Type from)
  {
    cmd(BC.CastT2S, 1-from.getSize());
    addi(from.tIndex);
  }
  
  // Cast an object to a given class. Takes a global class index. The
  // object on the stack isn't changed in any way, but the VM checks
  // if a cast is valid.
  void downCast(int cindex)
  {
    cmd(BC.DownCast);
    addi(cindex);
  }

  // Boolean operators
  void isEqual(int s)
  {
    cmdmult(BC.IsEqual, BC.IsEqualMulti, s, 1-2*s);
  }

  void isCaseEqual() { cmd(BC.IsCaseEqual, -1); }

  void less(Type t)
  {
    if(t.isUint || t.isChar) cmd(BC.ULess);
    else if(t.isInt) cmd(BC.ILess);
    else if(t.isLong) cmd(BC.LLess);
    else if(t.isFloat) cmd(BC.FLess);
    else if(t.isUlong) cmd(BC.ULLess);
    else if(t.isDouble) cmd(BC.DLess);
    setStack(1-2*t.getSize());
  }

  void not() { cmd(BC.Not); }

  void cmpArray() { cmd(BC.CmpArray, -1); }
  void icmpString() { cmd(BC.ICmpStr, -1); }

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

    // Run optimizations on the instruction list
    static if(optimizeAsm)
      optimize();

    // Next, go through the list and compute stack positions. We have
    // to loop the entire block since we're not always able to fix the
    // entire code in one go.
    bool changed, dead;
    do
      {
        changed = false;
        dead = false;

        int stackPos = 0;
        Node node = codeList.getHead();
        while(node !is null)
          {
            // Do we currently have a stack position to set?
            if(stackPos != -1)
              {
                // Remember if we change anything
                if(node.value.stackPos == -1)
                  changed = true;

                // Set the stack. If we're overwriting an existing
                // value, setStack will check that it's correct.
                stackPos = node.value.setStack(stackPos);

                assert(stackPos >= 0, "Assembler detected stack underflow");
              }
            else
              {
                // Nope, we don't have any information to give

                if(node.value.stackPos == -1)
                  // Neither has the code. That means we have dead
                  // code (unset stack values)
                  dead = true;
                else
                  // This code point has a stack value - this means
                  // that it's a label that has been 'marked'
                  // previously by a jump command. Let's copy its
                  // value.
                  {
                    assert(node.value.type == ET.Label);
                    assert(node.value.imprint == 0);
                    stackPos = node.value.stackPos;
                  }
              }

            // Did we have or get a stack value this round?
            if(stackPos != -1)
              {
                // The stack position should be exactly zero when we enter
                // and exit the function. That includes state labels.
                if(isCmd(node, BC.Return, BC.ReturnVal, BC.ReturnValN,
                              BC.Exit) || node.value.isStateLabel())
                  assert(stackPos == 0);

                // For jumps, mark the label as well
                if(isCmd(node, BC.Jump, BC.JumpZ, BC.JumpNZ))
                  {
                    auto ptr = node.getNext();
                    assert(ptr.value.type == ET.JumpPtr);
                    auto ce = ptr.value.label;
                    assert(ce !is null);
                    ce.setStack(stackPos);
                  }
              }

            // After exits, returns and jumps, we don't know what the
            // stack value is.
            if(isCmd(node, BC.Jump, BC.Return, BC.ReturnVal, BC.ReturnValN,
                        BC.Exit))
              stackPos = -1;

            node = getNext(node);
          }
      }
    while(changed && dead);

    static if(optimizeAsm)
      // Optimization should have removed all dead code
      assert(!dead);

    // Replace all PushLocal indices with relative indices.
    {
      Node node = codeList.getHead();

      while(node !is null)
        {
          auto next = node.getNext();
          if(next is null)
            break;

          int sp = node.value.stackPos;

          if(isCmd(node, BC.PushLocal, BC.IterUpdate))
            {
              with(next.value)
                {
                  if(sp != -1)
                    {
                      // Normal index reference
                      if(type == ET.Data4)
                        {
                          assert(idata < sp);
                          idata = sp-idata-1;
                        }
                      // Marked stack value
                      else if(type == ET.MarkRef)
                        {
                          assert(label !is null);
                          assert(label.jumps > 0);
                          label.jumps--;

                          // Get the marked stack position
                          int sp2 = label.stackPos;
                          assert(sp2 != -1);

                          // Convert to relative index and store it as
                          // a normal pushlocal
                          assert(sp2 < sp);
                          idata = sp-sp2-1;
                          type = ET.Data4;
                        }
                      else assert(0);
                    }
                }
            }

          else if(isCmd(node, BC.PushData) &&
                  next.value.type == ET.StackPtr)
            {
              // Convert to a normal pointer
              next.value.type = ET.Ptr;

              // Find the relative stack position
              int rel = next.value.idata;
              assert(rel < sp);
              rel = sp-rel-1;
              next.value.idata = codePtr(PT.Stack,rel);
            }

          node = next;
        }
    }

    // Kill stack marks
    {
      Node node = codeList.getHead();
      while(node !is null)
        {
          auto next = node.getNext();
          if(node.value.type == ET.StackMark)
            codeList.remove(node);
          node = next;
        }
    }

    // TODO: Do additional PushLocal-optimization here. We only need
    // specific optimizations that deal with pushlocal, such as
    // replacing pushlocal 0 with dup, or with replacing pushlocal +
    // getarrlen (a normal combination for $) with a specialized
    // instruction.

    // Also TODO: Move all the optimization code to asmopt.d, or this
    // file will get too messy.

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
    static if(printAsmOutput)
      {
        writef("\nCompiled function:");
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

  // Some handy definitions and functions
  alias codeList.Iterator Node;

  // True if the given node is an opcode or a label
  bool isPrincipal(Node n)
  {
    assert(n !is null);
    return
      n.value.isPrincipal();
  }

  // Skip until the next principal node
  Node getNext(Node node)
  {
    assert(node !is null);

    do node = node.getNext();
    while(node !is null && !isPrincipal(node))

      return node;
  }

  bool isCmd(Node node, BC ops[]...)
  {
    assert(node !is null);
    return node.value.isCmd(ops);
  }

  /*************************************************

               Optimization

  *************************************************/

  // All assembler-level optimization goes into this function
  void optimize()
  {
    // Whether anything changed this round. If we can run the entire
    // function with out changing anything, then we're done.
    bool changed;

    // Deletes an opcode or label, and all it's data nodes. Returns
    // the next node.
    Node deleteNode(Node node)
      {
        changed = true;

        assert(node !is null);
        assert(isPrincipal(node));

        // Remove this node and all following non-principal ones.
        do
          {
            auto tmp = node;
            node = node.getNext();

            // If this is a jump or a stack reference, make sure we
            // notify the label
            if(tmp.value.type == ET.JumpPtr ||
               tmp.value.type == ET.MarkRef)
              {
                auto lb = tmp.value.label;
                assert(lb !is null);
                assert(lb.jumps > 0);
                lb.jumps--;
              }

            codeList.remove(tmp);
          }
        while(node !is null && !isPrincipal(node));

        return node;
      }

    // Pushed value, in cases of static pushes
    int pushValue;
    // Is this a static push? (Ie. is the data available in the code
    // itself?)
    bool isStaticPush(Node node)
      {
        if(isCmd(node, BC.PushData))
          {
            node = node.getNext();
            assert(node.value.type == ET.Data4 ||
                   node.value.type == ET.Ptr);
            pushValue = node.value.idata;
            return true;
          }
        return false;
      }

    // Is this a push, of any kind? TODO: This should cover ANY
    // operation that has an imprint of +1 and no side effects. For
    // operations with imprint 0 and no side effects followed by a
    // pop, we can work backwards from the pop and remove the
    // operations. Essentially, we need more meta information about
    // the instructions (such as whether they have side effects) so
    // that we can fully automate this and not keep instruction-lists
    // here.
    bool isPush(Node node)
      {
        return isCmd(node,
                     BC.PushData, BC.PushLocal, BC.PushClassVar,
                     BC.PushParentVar, BC.PushThis, BC.Dup);
      }

    bool isValidLabel(Node node)
      {
        assert(node !is null);

        // Valid labels have jumps to them, or are state labels.
        if(node.value.type == ET.Label &&
           (node.value.jumps > 0 ||
            node.value.lnumber != -1))
          return true;
        return false;
      }

    // TODO: We need to run several of these multiple times. For
    // example, when jumps get deleted, old labels can become
    // inactive, making ever more code dead or prone for other
    // optimizations. The brute force approach is to just run the
    // entire function until it has an entire pass without changing
    // anything. This approach will probably work quite well, too. We
    // can refine it later, and see what other optimizers (GCC, LLVM,
    // Lua, Java, DMD, and so on) do.

  rerun:
    changed = false;


    /*************************************************

                 Removing dead code

    *************************************************/

    // Remove dead code - non-label code that occurs after jumps,
    // exits and returns
    {
      auto node = codeList.getHead();
      bool killSpree = false;
      while(node !is null)
        {
          // Active labels turn the killing spree off
          if(isValidLabel(node))
            killSpree = false;

          // Everything else is obliterated if we're on the
          // spree.
          if(killSpree)
            node = deleteNode(node);

          // If we're not on a spree, any non-conditional jump, exit
          // or return should start one up.
          else
            {
              if(isCmd(node,
                       BC.Jump, BC.Exit, BC.Return,
                       BC.ReturnVal, BC.ReturnValN))
                killSpree = true;

              // In any case, if we didn't delete the node, skip to
              // the next one
              node = getNext(node);
            }
        }
    }

    /*************************************************

                 Remove pointless jumps

    *************************************************/

    // Jumps where there's nothing between the jump op and the
    // label. These are common after dead code removal. Can probably
    // be squeezed into a more general peephole loop later.
    {
      auto node = codeList.getHead();
      while(node !is null)
        {
          Node next = node.getNext();

          if(node.value.type == ET.JumpPtr &&
             node.value.label == &next.value)
            {
              // Step back one to get the jump opcode
              node = node.getPrev();
              assert(node !is null && node.value.type == ET.OpCode);

              // TODO: We only handle pure jumps at the moment, but
              // later we should replace the others with a pop.
              if(node.value.idata == BC.Jump)
                next = deleteNode(node);
            }

          node = next;
        }
    }

    /*************************************************

                 Peephole loop

    *************************************************/

    // These are just some examples I toyed around with. Later we will
    // probably generalize this a whole lot, eg. with a replace() set
    // of functions or something similar.
    {
      auto node = codeList.getHead();
      while(node !is null)
        {
          Node next = getNext(node);

          // Kill inactive labels, except for state labels.
          if(node.value.type == ET.Label &&
             node.value.jumps == 0 &&
             node.value.lnumber == -1)
            deleteNode(node);

          // All the following depend on pairs of instructions.
          else if(next !is null)
            {
              // Is this a push (of any type?)
              if(isPush(node))
                {
                  // Push and pop? Delete both
                  if(isCmd(next, BC.Pop))
                    {
                      next = deleteNode(node);
                      next = deleteNode(next);
                    }
                }

              // Is this a static push?
              else if(isStaticPush(node))
                {
                  // Conditional jump that always fails?
                  if((isCmd(next, BC.JumpZ) && pushValue != 0) ||
                     (isCmd(next, BC.JumpNZ) && pushValue == 0))
                    {
                      // Delete both the push and the jump
                      next = deleteNode(node);
                      next = deleteNode(next);
                    }

                  // Conditional jump that always follows through?
                  else if((isCmd(next, BC.JumpZ) && pushValue == 0) ||
                          (isCmd(next, BC.JumpNZ) && pushValue != 0))
                    {
                      // Delete the push, replace the jump with an
                      // unconditional one
                      next = deleteNode(node);
                      next.value.idata = BC.Jump;
                    }
                }
            }

          node = next;
        }
    }

    // If the code changed, run the optimizations again
    if(changed)
      goto rerun;
  }
}
