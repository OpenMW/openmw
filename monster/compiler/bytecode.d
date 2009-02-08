/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (bytecode.d) is part of the Monster script language
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

module monster.compiler.bytecode;

// Byte code operands
enum BC
  {
    Exit = 1,           // Exit function.

    Call,               // Call function in this object. Takes a class
                        // tree index and a function index, both ints.

    CallFar,            // Call function in another object. Takes a
                        // class index tree and a function index. The
                        // object must be pushed on the stack.

    CallIdle,           // Calls an idle function in this object.
                        // Takes a class tree index and a function
                        // index.

    CallIdleFar,        // Calls an idle function in another
                        // object. Also takes an object index from the
                        // stack.

    Return,             // Takes a parameter nr (int). Equivalent to:
                        // POPN nr (remove nr values of the stack)
                        // EXIT

    ReturnVal,          // Takes parameter nr (int). Equivalent to:
                        // POP value and store it in TMP
                        // POPN nr (remove function parameters)
                        // PUSH TMP (put return value back on stack)
                        // EXIT

    ReturnValN,         // Same as ReturnVal, but takes a second
                        // parameter (int). This gives the size of the
                        // return value.

    State,              // Set state. Followed by an int giving the
                        // state index, another int giving the label
                        // index and a third int giving the class tree
                        // index. State index -1 means the empty
                        // state, and -1 for the label means no label
                        // is specified (eg state = test; instead of
                        // state = test.label). For state -1 the label
                        // index must also be -1, and the class index
                        // is ignored.

    New,                // Create a new object. Followed by an int
                        // giving the class index (in the file lookup
                        // table)

    Clone,              // Clones an object - create a new object of
                        // the same class, then copy variable values
                        // and state from the old object to the
                        // new. Replaces the object index on the stack
                        // with the new index.

    Jump,               // Jump to given position (int)

    JumpZ,              // Pop a value, if it is zero then jump to
                        // given position (int)

    JumpNZ,             // Jump if non-zero

    PushData,           // Push the next four bytes verbatim

    PushLocal,          // Push value of a local variable or function
                        // parameter, index given as a signed
                        // int. Gives the index from the current stack
                        // frame. 0 is the first int, 1 is the second
                        // int, -1 is the last int in the function
                        // parameter area, etc.

    PushClassVar,       // Push value in object data segment (in this
                        // object). Parameter is an int offset.

    PushParentVar,      // Push value in data segment of parent
                        // object. int class tree index, int offset

    PushFarClassVar,    // Push value from the data segment in another
                        // object. The object reference is already on
                        // the stack. int class tree index, int offset

    PushFarClassMulti,  // Pushes multiple ints from the data
                        // segment. Takes the variable size (int) as
                        // the first parameter, otherwise identical to
                        // PushFarClassVar.

    PushThis,           // Push the 'this' object reference

    PushSingleton,      // Push the singleton object. Takes the global
                        // class index (int) as parameter.

    // The Push*8 instructions are not implemented yet as they are
    // just optimizations of existing features. The names are reserved
    // for future use.

    Push8,              // Push the next 8 bytes (two ints)
                        // verbatim.
    PushLocal8,         // Does the same as PushLocal except it also
                        // pushes the next int onto the stack. The
                        // index must be > 0 or <= -2.
    PushClassVar8,      // Same as PushClassVar but pushes two ints
    PushFarClassVar8,


    Pop,                // Pop data and forget about it

    PopN,               // Takes a byte parameter N, equivalent to
                        // calling Pop N times.

    Dup,                // Duplicate the next value on the
                        // stack. Equivalent to: a=pop; push a; push
                        // a;

    StoreRet,           // Basic operation for moving data to
                        // memory. Schematically pops a Ptr of the
                        // stack, pops a value, moves the value into
                        // the Ptr, and then pushes the value back.

    Store,              // Same as StoreRet but does not push the
                        // value back. Not implemented, but will later
                        // replace storeret completely.

    StoreRet8,          // Same as StoreRet except two ints are popped
                        // from the stack and moved into the data.

    StoreRetMult,       // Takes the size as an int parameter

    IAdd,               // Standard addition, operates on the two next
                        // ints in the stack, and stores the result in
                        // the stack.

    ISub,               // Subtraction. a-b, where a is pushed first.

    IMul,               // Multiplication

    IDiv,               // Division. The dividend must be pushed
                        // first, the divisor second.
    UDiv,               // uint

    IDivRem,            // Reminder division
    UDivRem,            // uint

    INeg,               // Negate the next integer on the stack

    FAdd,               // Float arithmetic
    FSub,
    FMul,
    FDiv,
    FIDiv,
    FDivRem,
    FNeg,

    LAdd,               // Long arithmetic
    LSub,
    LMul,
    LDiv,
    ULDiv,
    LDivRem,
    ULDivRem,
    LNeg,

    DAdd,               // Double arithmetic
    DSub,
    DMul,
    DDiv,
    DIDiv,
    DDivRem,
    DNeg,

    IsEqual,            // Pop two ints, push true (1) if they are
                        // equal or false (0) otherwise
    IsEqualMulti,       // Takes an int parameter giving the value size

    IsCaseEqual,        // Same as IsEqual, but uses (unicode) case
                        // insensitive match. Values are assumed to be
                        // dchars.

    CmpArray,           // Compare two arrays. Only does exact
                        // byte-for-byte matching.
    ICmpStr,            // Does a case insensitive check of two
                        // strings. Both these pop two array indices
                        // of the stack and push a bool.

    PreInc,             // ++, pops an address, increases variable,
                        // and pushes the value

    PreDec,             // -- (these might change)

    PostInc,            // ++ and -- that push the original value
    PostDec,            // rather than the new one

    PreInc8, PreDec8,
    PostInc8, PostDec8, // 64 bit versions

    Not,                // Inverse the bool on the stack

    ILess,              // Pops b, pops a, pushes true if a < b, false
                        // otherwise. Works on signed ints.

    ULess,              // unsigned ints
    LLess,              // long
    ULLess,             // ulong
    FLess,              // float
    DLess,              // double

    // All the Cast* instructions pops a variable of the given type of
    // the stack, and pushes back an equivalent variable of the new
    // type.

    CastI2L,            // int to long (signed)

    CastI2F,            // int to float
    CastU2F,            // uint to float
    CastL2F,            // long to float
    CastUL2F,           // ulong to float
    CastD2F,            // double to float

    CastI2D,            // int to double
    CastU2D,            // uint to double
    CastL2D,            // long to double
    CastUL2D,           // ulong to double
    CastF2D,            // float to double

    CastT2S,            // cast any type to string. Takes the type
                        // index (int) as a parameter

    FetchElem,          // Get an element from an array. Pops the
                        // index, then the array reference, then
                        // pushes the value. The element size is
                        // determined from the array.

    GetArrLen,          // Get the length of an array. Pops the array,
                        // pushes the length.

    PopToArray,         // Takes a raw length n (int) and an element
                        // size (int). Creates an array from the last
                        // n values on the stack and pops them off.
                        // Pushes the new array index. The values are
                        // copied into the new array, and are
                        // independent of the stack.

    NewArray,           // Takes one int giving the array nesting
                        // level (rank), one int giving the element
                        // size (s) and s ints giving the initial
                        // value. Pops the lengths (one int per rank
                        // level) from the stack. Pushes a new array
                        // of the given length. The lengths should
                        // pushed in the same order they appear in a
                        // new-expression, ie. new int[1][2] are
                        // pushed as 1 then 2.

    CopyArray,          // Pops two array indices from the stack, and
                        // copies the data from one to another. Pushes
                        // back the array index of the
                        // destination. The destination array is
                        // popped first, then the source. The lengths
                        // must match. If the arrays may overlap in
                        // memory without unexpected effects.

    DupArray,           // Pops an array index of the stack, creates a
                        // copy of the array, and pushes the index of
                        // the new array.

    MakeConstArray,     // Pops an array index, creates a const
                        // reference to the same data, pushes the
                        // index.

    IsConstArray,       // Pops the array index, pushes bool
                        // reflecting the const status

    Slice,              // Create a slice. Pops the second index, then
                        // the first, then the array index. Pushes a
                        // new array that is a slice of the original.

    FillArray,          // Fill an array. Pop an array index, then a
                        // value (int). Sets all the elements in the
                        // array to the value. Pushes the array index
                        // back. Takes an int specifying the element
                        // size.

    CatArray,           // Concatinate two arrays, on the stack.

    CatLeft,            // Concatinate an array with a left
                        // element. The element is pushed first, so
                        // the array index is first off the stack.
    CatRight,           // Concatinate with right element. Array
                        // pushed first, then element. Both these take
                        // an element size (int).

    ReverseArray,       // Reverses an array. The index is on the
                        // stack. The array is reversed in place, and
                        // the index is left untouched on the stack.

    CreateArrayIter,    // Create array iterator. Expects the stack to
                        // hold an index(int), a value(int) and an
                        // array index, pushed in that order. Replaces
                        // the array index with an iterator index,
                        // sets the rest to reflect the first element
                        // in iteration. Takes two byte parameters
                        // that are either 0 or 1. If the first is
                        // set, then the array is iterated in the
                        // reverse order. If the second is set, then
                        // the value is a reference, ie. changes to it
                        // will be transfered back to the orignal
                        // array. Pushes false if the array is empty,
                        // true otherwise.

    CreateClassIter,    // Create a class iterator. Expects the
                        // stack to hold a value (object index =
                        // int). Takes one int parameter, which is the
                        // class index. Pushes false if no objects
                        // exist, true otherwise.

    IterNext,           // Iterate to the next element. Leaves the
                        // iteration variables on the stack. Pushes
                        // false if this was the last element, true
                        // otherwise.

    IterBreak,          // Break off iteration. Does exactly the same
                        // thing as IterNext would done if it had just
                        // finished the last iteration step, except it
                        // does not push anything onto the stack.

    IterUpdate,         // Update the original array or data from
                        // reference variables in this
                        // iteration. Called whenever a 'ref' variable
                        // is changed, to make sure that the changes
                        // take effect. Takes an int parameter that
                        // gives the stack position (in the current
                        // frame) of the iterator index.

    GetStack,           // Internal debugging function. Pushes the
                        // current stack position on the stack. Might
                        // be removed later.

    MultiByte,          // This instruction consists of a second byte
                        // or an extra int. Reserved for future
                        // use. This is intended for when / if the
                        // number of instructions exceeds
                        // 256. Instructions in this list with numbers
                        // >= 256 will be coded in this way.

    // Instructions appearing after the MultiByte mark might be coded
    // as multi-byte. They are handled in a more time-consuming
    // matter. You should only use this space for instructions that
    // are seldomly executed.

    Error,              // Throw an exception. Takes an error code
                        // (byte) defined below. The library user will
                        // later be able to choose whether this halts
                        // execution entirely or just kills the
                        // offending object.

    Last
  }

// Make sure all single-byte instructions will in fact fit in a single
// byte.
static assert(BC.MultiByte < 255);

// Make us aware when we break the byte barrier
static assert(BC.Last < 255);

enum Err
  {
    None,		// Should never happen
    NoReturn,		// Function is missing a return statement
  }

// Used for coded pointers. The first byte in a coded pointer gives
// the pointer type, and the remaining 24 bits gives an index whose
// meaning is determined by the type. The pointers can be used both
// for variables and for functions.
enum PT
  {
    Null        = 0, // Null pointer. The index must also be zero.

    // Variable pointers

    Stack       = 1, // Index is relative to function stack
                     // frame. Used for local variables.

    DataOffs    = 2, // This class, this object. Index is data
                     // segment offset.

    DataOffsCls = 4, // Variable is in this object, but in another
                     // class. The class MUST be a parent class of the
                     // current object. A class tree index follows
                     // this pointer on the stack.

    FarDataOffs = 5, // Another class, another object. The index is a
                     // data offset. Pop the class index off the
                     // stack, and then object index.

    ArrayIndex  = 30, // Pointer to an array element. The array and
                      // the index are pushed on the stack, the
                      // pointer index is zero.
  }

char[] errorString(ubyte er)
{
  if(er < errorToString.length)
    return errorToString[er];
  return "Unknown error code";
}

union _CodePtr
{
  // How the pointer is coded
  align(1) struct
  {
    ubyte type;
    int val24;
  }

  // The end result is stored in val32
  align(1) struct
  {
    int val32;
    ubyte remains;
  }
}
static assert(_CodePtr.sizeof == 5);

// Encode a "pointer". Pointers are two shorts encoded into an
// int. The first byte is the pointer type, the remaining 24 bits
// gives the index.
int codePtr(PT type, int index)
{
  assert(index >= -(1<<23) && index < (1<<24),
         "index out of range for 24 bit value");
  assert(type != 0 || index == 0,
         "null pointers must have index == 0");
  assert(type == PT.Stack || index >= 0,
         "only PT.Stack can have a negative index");


  _CodePtr t;
  t.type = type;
  t.val24 = index;

  assert(t.remains == 0);

  return t.val32;
}

void decodePtr(int ptr, out PT type, out int index)
{
  _CodePtr t;
  t.val32 = ptr;

  type = cast(PT) t.type;
  index = t.val24;

  assert(type != 0 || index == 0,
         "null pointers must have index == 0");
}

// Getting the name of an enum should be much easier than creating
// braindead constructions like this. Although this is still much
// better than the C++ equivalent. I'm just happy I did it through a
// script instead of typing it all by hand.

// These kind of braindead constructions will luckily be completely
// unnecessary in Monster script, We will not only will have .name
// property on enums, but make it easy to assign other values (like
// numbers and descriptions) to them as well.
char[][] errorToString =
[
 Err.None: "No error!",
 Err.NoReturn: "Function ended without returning a value"
 ];

char[][] bcToString =
[
 BC.Exit: "Exit",
 BC.Call: "Call",
 BC.CallFar: "CallFar",
 BC.CallIdle: "CallIdle",
 BC.Return: "Return",
 BC.ReturnVal: "ReturnVal",
 BC.ReturnValN: "ReturnValN",
 BC.State: "State",
 BC.New: "New",
 BC.Jump: "Jump",
 BC.JumpZ: "JumpZ",
 BC.JumpNZ: "JumpNZ",
 BC.PushData: "PushData",
 BC.PushLocal: "PushLocal",
 BC.PushClassVar: "PushClassVar",
 BC.PushParentVar: "PushParentVar",
 BC.PushFarClassVar: "PushFarClassVar",
 BC.PushFarClassMulti: "PushFarClassMulti",
 BC.PushThis: "PushThis",
 BC.PushSingleton: "PushSingleton",
 BC.Push8: "Push8",
 BC.PushLocal8: "PushLocal8",
 BC.PushClassVar8: "PushClassVar8",
 BC.PushFarClassVar8: "PushFarClassVar8",
 BC.Pop: "Pop",
 BC.PopN: "PopN",
 BC.Dup: "Dup",
 BC.StoreRet: "StoreRet",
 BC.Store: "Store",
 BC.StoreRet8: "StoreRet8",
 BC.StoreRetMult: "StoreRetMult",
 BC.FetchElem: "FetchElem",
 BC.GetArrLen: "GetArrLen",
 BC.IMul: "IMul",
 BC.IAdd: "IAdd",
 BC.ISub: "ISub",
 BC.IDiv: "IDiv",
 BC.IDivRem: "IDivRem",
 BC.UDiv: "UDiv",
 BC.UDivRem: "UDivRem",
 BC.INeg: "INeg",
 BC.LMul: "LMul",
 BC.LAdd: "LAdd",
 BC.LSub: "LSub",
 BC.LDiv: "LDiv",
 BC.LDivRem: "LDivRem",
 BC.ULDiv: "ULDiv",
 BC.ULDivRem: "ULDivRem",
 BC.LNeg: "LNeg",
 BC.DMul: "DMul",
 BC.DAdd: "DAdd",
 BC.DSub: "DSub",
 BC.DDiv: "DDiv",
 BC.DIDiv: "DIDiv",
 BC.DDivRem: "DDivRem",
 BC.DNeg: "DNeg",
 BC.FAdd: "FAdd",
 BC.FSub: "FSub",
 BC.FMul: "FMul",
 BC.FDiv: "FDiv",
 BC.FIDiv: "FIDiv",
 BC.FDivRem: "FDivRem",
 BC.FNeg: "FNeg",
 BC.IsEqual: "IsEqual",
 BC.IsEqualMulti: "IsEqualMulti",
 BC.IsCaseEqual: "IsCaseEqual",
 BC.CmpArray: "CmpArray",
 BC.ICmpStr: "ICmpStr",
 BC.PreInc: "PreInc",
 BC.PreDec: "PreDec",
 BC.PostInc: "PostInc",
 BC.PostDec: "PostDec",
 BC.PreInc8: "PreInc8",
 BC.PreDec8: "PreDec8",
 BC.PostInc8: "PostInc8",
 BC.PostDec8: "PostDec8",
 BC.Not: "Not",
 BC.ILess: "ILess",
 BC.ULess: "ULess",
 BC.LLess: "LLess",
 BC.ULLess: "ULLess",
 BC.FLess: "FLess",
 BC.DLess: "DLess",
 BC.CastI2L: "CastI2L",
 BC.CastI2F: "CastI2F",
 BC.CastU2F: "CastU2F",
 BC.CastL2F: "CastL2F",
 BC.CastUL2F: "CastUL2F",
 BC.CastD2F: "CastD2F",
 BC.CastI2D: "CastI2D",
 BC.CastU2D: "CastU2D",
 BC.CastL2D: "CastL2D",
 BC.CastUL2D: "CastUL2D",
 BC.CastF2D: "CastF2D",
 BC.CastT2S: "CastT2S",
 BC.PopToArray: "PopToArray",
 BC.NewArray: "NewArray",
 BC.CopyArray: "CopyArray",
 BC.DupArray: "DupArray",
 BC.MakeConstArray: "MakeConstArray",
 BC.IsConstArray: "IsConstArray",
 BC.Slice: "Slice",
 BC.FillArray: "FillArray",
 BC.CatArray: "CatArray",
 BC.CatLeft: "CatLeft",
 BC.CatRight: "CatRight",
 BC.CreateArrayIter: "CreateArrayIter",
 BC.IterNext: "IterNext",
 BC.IterBreak: "IterBreak",
 BC.IterUpdate: "IterUpdate",
 BC.CreateClassIter: "CreateClassIter",
 BC.GetStack: "GetStack",
 BC.MultiByte: "MultiByte",
 BC.Error: "Error",
 ];
