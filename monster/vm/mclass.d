/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (mclass.d) is part of the Monster script language package.

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

module monster.vm.mclass;

import monster.compiler.functions;
import monster.compiler.types;
import monster.compiler.scopes;
import monster.compiler.tokenizer;
import monster.compiler.statement;
import monster.compiler.variables;
import monster.compiler.states;
import monster.compiler.structs;
import monster.compiler.block;
import monster.compiler.enums;

import monster.vm.codestream;
import monster.vm.idlefunction;
import monster.vm.arrays;
import monster.vm.error;
import monster.vm.vm;
import monster.vm.mobject;

import monster.util.flags;
import monster.util.string;
import monster.util.list;
import monster.util.freelist;

import std.string;
import std.stdio;
import std.file;
import std.stream;

typedef void *MClass; // Pointer to C++ equivalent of MonsterClass.

typedef int CIndex;

// Parameter to the constructor. Decides how the class is
// created. TODO: This system will be removed again, because we'll
// stop using constructors.
enum MC
  {
    File,    // Load class from file (default)
    NoCase,  // Load class from file, case insensitive name match
    String,  // Load class from string
  }

enum CFlags
  {
    None      = 0x00, // Initial value

    Parsed    = 0x01, // Class has been parsed
    Scoped    = 0x02, // Class has been inserted into the scope
    Resolved  = 0x04, // Class body has been resolved
    Compiled  = 0x08, // Class body has been compiled
    InScope   = 0x10, // We are currently inside the createScope
                      // function
    Module    = 0x20, // This is a module, not a class
    Singleton = 0x40, // This is a singleton. Also set for modules.
    Abstract  = 0x80, // No objects can be created from this class
  }

// The class that handles 'classes' in Monster.
final class MonsterClass
{
  /***********************************************
   *                                             *
   *    Static functions                         *
   *                                             *
   ***********************************************/

  // TODO: These should be moved to vm.vm

  // Get a class with the given name. It must already be loaded.
  static MonsterClass get(char[] name) { return global.getClass(name); }

  // Find a class with the given name. Load the file if necessary, and
  // fail if the class cannot be found.
  static MonsterClass find(char[] name) { return global.findClass(name); }

  static bool canParse(TokenArray tokens)
    {
      return
        Block.isNext(tokens, TT.Class) ||
        Block.isNext(tokens, TT.Singleton) ||
        Block.isNext(tokens, TT.Module);
    }

  static uint getTotalObjects() { return allObjects.length; }

 final:

  /*******************************************************
   *                                                     *
   *     Variables                                       *
   *                                                     *
   *******************************************************/

  // Index within the parent tree. This might become a list at some
  // point.
  int treeIndex;

  Token name; // Class name and location

  CIndex gIndex; // Global index of this class

  ClassScope sc;

  ObjectType objType; // Type for objects of this class
  Type classType; // Type for class references to this class (not
                  // implemented yet)

 private:
  // List of objects of this class. Includes objects of all subclasses
  // as well.
  PointerList objects;

  Flags!(CFlags) flags;

 public:

  bool isParsed() { return flags.has(CFlags.Parsed); }
  bool isScoped() { return flags.has(CFlags.Scoped); }
  bool isResolved() { return flags.has(CFlags.Resolved); }
  bool isCompiled() { return flags.has(CFlags.Compiled); }

  bool isSingleton() { return flags.has(CFlags.Singleton); }
  bool isModule() { return flags.has(CFlags.Module); }
  bool isAbstract() { return flags.has(CFlags.Abstract); }

  // Call whenever you require this function to have its scope in
  // order. If the scope is missing, this will call createScope if
  // possible, or fail if the class has not been loaded.
  void requireScope()
    {
      if(isScoped) return;
      if(!isParsed)
        fail("Cannot use class '" ~ name.str ~
             "': not found or forward reference",
             name.loc);

      createScope();
    }

  // Called whenever we need a completely compiled class, for example
  // when creating an object. Compiles the class if it isn't done
  // already.
  void requireCompile() { if(!isCompiled) compileBody(); }


  /*******************************************************
   *                                                     *
   *     Constructors                                    *
   *                                                     *
   *******************************************************/

  this() {}

  this(MC type, char[] name1, char[] name2 = "", bool usePath = true)
    {
      if(type == MC.File || type == MC.NoCase)
        {
          if(type == MC.NoCase)
            loadCI(name1, name2, usePath);
          else
            load(name1, name2, usePath);

          return;
        }

      if(type == MC.String)
        {
          loadString(name1, name2);

          return;
        }

      assert(0, "encountered unknown MC type");
    }

  this(MC type, Stream str, char[] nam = "")
    {
    }

  this(ref TokenArray toks, char[] nam="")
    { loadTokens(toks, nam); }

  this(Stream str, char[] nam="")
    { loadStream(str, nam); }

  this(char[] nam1, char[] nam2 = "", bool usePath=true)
    { this(MC.File, nam1, nam2, usePath); }


  /*******************************************************
   *                                                     *
   *     Class loaders                                   *
   *                                                     *
   *******************************************************/

  // Load from file system. The names must specify a class name, a
  // file name, or both. The class name, if specified, must match the
  // loaded class name exactly. If usePath is true (default), the
  // include paths are searched.
  void load(char[] name1, char[] name2 = "", bool usePath=true)
    { doLoad(name1, name2, true, usePath); }

  // Same as above, except the class name check is case insensitive.
  void loadCI(char[] name1, char[] name2 = "", bool usePath=true)
    { doLoad(name1, name2, false, usePath); }

  void loadString(char[] str, char[] fname="")
  {
    assert(str != "");
    auto ms = new MemoryStream(str);
    if(fname == "") fname = "(string)";
    loadStream(ms, fname);
  }

  // Load a script from a stream. The filename parameter is only used
  // for error messages.
  void load(Stream str, char[] fname="(stream)")
    { loadStream(str, fname); }

  void loadTokens(ref TokenArray toks, char[] name)
    { parse(toks, name); }

  void loadStream(Stream str, char[] fname="(stream)", int bom = -1)
    {
      assert(str !is null);

      // Parse the stream
      parse(str, fname, bom);
    }


  /*******************************************************
   *                                                     *
   *     Management of member functions                  *
   *                                                     *
   *******************************************************/

  // Bind a delegate to the name of a native function. TODO: Add
  // optional signature check here at some point?
  void bind(char[] name, dg_callback nf)
    { bind_locate(name, FuncType.NativeDDel).natFunc_dg = nf; }

  // Same as above, but binds a function instead of a delegate.
  void bind(char[] name, fn_callback nf)
    { bind_locate(name, FuncType.NativeDFunc).natFunc_fn = nf; }

  // Used for C functions
  void bind_c(char[] name, c_callback nf)
    { bind_locate(name, FuncType.NativeCFunc).natFunc_c = nf; }

  // Bind an idle function
  void bind(char[] name, IdleFunction idle)
    { bind_locate(name, FuncType.Idle).idleFunc = idle; }

  // Find a function by index. Used internally, and works for all
  // function types.
  Function *findFunction(int index)
    {
      requireScope();
      assert(index >=0 && index < functions.length);
      assert(functions[index] !is null);
      return functions[index];
    }

  // Find a virtual function by index. ctree is the tree index of the
  // class where the function is defined, findex is the intra-class
  // function index.
  Function *findVirtualFunc(int ctree, int findex)
    {
      requireScope();
      assert(ctree >= 0 && ctree <= treeIndex);
      assert(findex >= 0 && findex < virtuals[ctree].length);
      assert(virtuals[ctree][findex] !is null);

      return virtuals[ctree][findex];
    }

  // Find a given callable function, virtually.
  Function *findFunction(char[] name)
  {
    requireScope();

    // Get the function from the scope
    auto ln = sc.lookupName(name);

    if(!ln.isFunc)
      fail("Function '" ~ name ~ "' not found.");

    auto fn = ln.func;

    if(!fn.isNormal && !fn.isNative)
      {
        // Being here is always bad. Now we just need to find
        // out what error message to give.
        if(fn.isAbstract)
          fail(name ~ " is abstract.");

        if(fn.isIdle)
          fail("Idle function " ~ name ~
               " cannot be called from native code.");
        assert(0);
      }

    assert(fn !is null);
    return fn;
  }

  /*******************************************************
   *                                                     *
   *     Binding of constructors                         *
   *                                                     *
   *******************************************************/

  void bindConst(dg_callback nf)
    {
      assert(natConst.ftype == FuncType.Native,
             "Cannot set native constructor for " ~ toString ~ ": already set");
      natConst.ftype = FuncType.NativeDDel;
      natConst.natFunc_dg = nf;
    }

  void bindConst(fn_callback nf)
    {
      assert(natConst.ftype == FuncType.Native,
             "Cannot set native constructor for " ~ toString ~ ": already set");
      natConst.ftype = FuncType.NativeDFunc;
      natConst.natFunc_fn = nf;
    }

  void bindConst_c(c_callback nf)
    {
      assert(natConst.ftype == FuncType.Native,
             "Cannot set native constructor for " ~ toString ~ ": already set");
      natConst.ftype = FuncType.NativeCFunc;
      natConst.natFunc_c = nf;
    }


  /*******************************************************
   *                                                     *
   *     Management of member variables                  *
   *                                                     *
   *******************************************************/

  Variable* findVariable(char[] name)
    {
      requireScope();

      auto ln = sc.lookupName(name);

      if(!ln.isVar)
        fail("Variable " ~ name ~ " not found");

      Variable *vb = ln.var;

      assert(vb.vtype == VarType.Class);

      return vb;
    }


  /*******************************************************
   *                                                     *
   *     Management of member states                     *
   *                                                     *
   *******************************************************/

  State* findState(char[] name)
  {
    requireScope();

    auto ln = sc.lookupName(name);
    if(!ln.isState)
      fail("State " ~ name ~ " not found");

    State *st = ln.state;

    return st;
  }

  // Look up state and label based on indices. We allow lindex to be
  // -1, in which case a null label is returned.
  StateLabelPair findState(int sindex, int lindex)
    {
      requireScope();
      assert(sindex >=0 && sindex < states.length);

      StateLabelPair res;
      res.state = states[sindex];

      assert(res.state !is null);

      if(lindex == -1)
        res.label = null;
      else
        {
          assert(lindex >= 0 && lindex < res.state.labelList.length);
          res.label = res.state.labelList[lindex];
          assert(res.label !is null);
        }

      return res;
    }

  // Find a state and a given label within it. Fails if it is not
  // found.
  StateLabelPair findState(char[] name, char[] label)
    {
      requireScope();
      assert(label != "");

      StateLabelPair pr;
      pr.state = findState(name);
      pr.label = pr.state.findLabel(label);

      if(pr.label is null)
        fail("State " ~ name ~ " does not have a label named " ~ label);

      return pr;
    }


  /*******************************************************
   *                                                     *
   *     Object managament                               *
   *                                                     *
   *******************************************************/

  // Loop through all objects of this type
  int opApply(int delegate(ref MonsterObject v) del)
    {
      int dg(ref void *vp)
        {
          auto mop = cast(MonsterObject*)vp;
          return del(*mop);
        }
      return objects.opApply(&dg);
    }

  // Get the first object in the list for this class
  MonsterObject* getFirst()
    { return cast(MonsterObject*)objects.getHead().value; }

  MonsterObject* getNext(MonsterObject *ob)
    {
      auto iter = (*getListPtr(ob, treeIndex)).getNext();
      if(iter is null) return null;
      return cast(MonsterObject*)iter.value;
    }

  // Get the singleton object
  MonsterObject* getSing()
    {
      if(!isSingleton)
        fail("Class is not a singleton: " ~ name.str);
      requireCompile();
      assert(singObj !is null);
      return singObj;
    }

  MonsterObject* createObject()
    { return createClone(null); }

  // Get the whole allocated buffer belonging to this object
  private int[] getDataBlock(MonsterObject *obj)
    {
      assert(obj !is null);
      assert(obj.cls is this);
      return (cast(int*)obj.data.ptr)[0..totalData.length];
    }

  private vpIter getListPtr(MonsterObject *obj, int i)
    {
      auto ep = cast(ExtraData*) &obj.data[i][$-MonsterObject.exSize];
      return &ep.node;
    }

  // Create a new object based on an existing object
  MonsterObject* createClone(MonsterObject *source)
    {
      requireCompile();

      if(isModule && singObj !is null)
        fail("Cannot create instances of module " ~ name.str);

      MonsterObject *obj = allObjects.getNew();

      obj.state = null;
      obj.cls = this;

      // Allocate the object data segment from a freelist
      int[] odata = Buffers.getInt(totalData.length);

      // Copy the data, either from the class (in case of new objects)
      // or from the source (when cloning.)
      if(source !is null)
        {
          assert(!isAbstract);
          assert(source.cls is this);

          assert(source.data.length == tree.length);

          // Copy data from the object
          odata[] = getDataBlock(source);
        }
      else
        {
          if(isAbstract)
            fail("Cannot create objects from abstract class " ~ name.str);

          // Copy init values from the class
          odata[] = totalData[];
        }

      // Use this to get subslices of the data segment
      int[] slice = odata;
      int[] get(int ints)
        {
          assert(ints <= slice.length);
          int[] res = slice[0..ints];
          slice = slice[ints..$];
          return res;
        }

      // The beginning of the block is used for the int data[][]
      // array.
      obj.data = cast(int[][]) get(iasize*tree.length);

      // Set up the a slice for the data segment of each class
      foreach(i, c; tree)
        {
          // Just get the slice - the actual data is already set up.
          obj.data[i] = get(c.data.length + MonsterObject.exSize);

          // Insert ourselves into the per-class list. We've already
          // allocated size for a node, we just have to add it to the
          // list.
          auto node = getListPtr(obj, i);
          node.value = obj; // Store the object pointer
          c.objects.insertNode(node);
        }

      // At this point we should have used up the entire slice
      assert(slice.length == 0);

      // Call constructors
      foreach(c; tree)
        {
          // Custom native constructor
          if(c.natConst.ftype != FuncType.Native)
            natConst.call(obj);

          // TODO: Call script constructors here
        }

      // Set the same state as the source
      if(source !is null)
        obj.setState(source.state, null);

      // Make sure that getDataBlock works
      assert(getDataBlock(obj).ptr == odata.ptr &&
             getDataBlock(obj).length == odata.length);

      return obj;
    }

  // Free an object and its thread
  void deleteObject(MonsterObject *obj)
    {
      assert(obj.cls is this);

      if(isModule)
        fail("Cannot delete instances of module " ~ name.str);

      // Shut down any active code in the thread
      obj.clearState();

      // clearState should also clear the thread
      assert(obj.sthread is null);

      // This effectively marks the object as dead
      obj.cls = null;

      foreach_reverse(i, c; tree)
        {
          // TODO: Call destructors here

          // Remove from class list
          c.objects.removeNode(getListPtr(obj,i));
        }

      // Return it to the freelist
      allObjects.remove(obj);

      // Return the data segment
      Buffers.free(getDataBlock(obj));
    }


  /*******************************************************
   *                                                     *
   *     Misc. functions                                 *
   *                                                     *
   *******************************************************/

  // Check if this class is a child of cls.
  bool childOf(MonsterClass cls)
    {
      requireScope();

      int ind = cls.treeIndex;
      // If 'cls' is part of our parent tree, then we are a child.
      return ind < tree.length && tree[ind] is cls;
    }

  // Check if this class is a parent of cls.
  bool parentOf(MonsterClass cls)
    { return cls.childOf(this); }

  // Ditto for a given object
  bool parentOf(MonsterObject *obj)
    { return obj.cls.childOf(this); }

  // Get the tree-index of a given parent class
  int upcast(MonsterClass mc)
    {
      requireScope();

      int ind = mc.treeIndex;
      if(ind < tree.length && tree[ind] is mc)
        return ind;

      fail("Cannot upcast " ~ toString ~ " to " ~ mc.toString);
    }

  // Get the given class from a tree index
  MonsterClass upcast(int ind)
    {
      requireScope();

      if(ind < tree.length) return tree[ind];

      fail("Cannot upcast " ~toString ~ " to index " ~ .toString(ind));
    }

  // Get the global index of this class
  CIndex getIndex() { requireScope(); return gIndex; }
  int getTreeIndex() { requireScope(); return treeIndex; }
  char[] getName() { assert(name.str != ""); return name.str; }
  char[] toString() { return getName(); }
  uint numObjects() { return objects.length; }

 private:

  /*******************************************************
   *                                                     *
   *     Private variables                               *
   *                                                     *
   *******************************************************/

  // Contains the entire class tree for this class, always with
  // ourselves as the last entry. Any class in the list is always
  // preceded by all the classes it inherits from.
  MonsterClass tree[];

  // List of variables and functions declared in this class, ordered
  // by index.
  Function* functions[];
  Variable* vars[];
  State* states[];

  // Singleton object - used for singletons and modules only.
  MonsterObject *singObj;

  // Function table translation list. Same length as tree[]. For each
  // class in the parent tree, this list holds a list equivalent to
  // the functions[] list in that class. The difference is that all
  // overrided functions have been replaced by their successors.
  Function*[][] virtuals;

  int[] data; // Contains the initial object data segment
  int[] sdata; // Static data segment

  // The total data segment that's assigned to each object. It
  // includes the data segment of all parent objects and some
  // additional internal data.
  int[] totalData;

  // Direct parents of this class
  MonsterClass parents[];
  Token parentNames[];

  // Used at compile time
  VarDeclStatement[] vardecs;
  FuncDeclaration[] funcdecs;
  StateDeclaration[] statedecs;
  StructDeclaration[] structdecs;
  EnumDeclaration[] enumdecs;
  ImportStatement[] imports;

  // Native constructor, if any
  Function natConst;

  /*******************************************************
   *                                                     *
   *     Various private functions                       *
   *                                                     *
   *******************************************************/

  // Create the data segment for this class. TODO: We will have to
  // handle string literals and other array constants later. This is
  // called at the very end, after all code has been compiled. That
  // means that array literals can be inserted into the class in the
  // compile phase and still "make it" into the data segment as static
  // data.
  int[] getDataSegment()
    {
      assert(sc !is null && sc.isClass(), "Class does not have a class scope");
      uint dataSize = sc.getDataSize;
      int[] data = new int[dataSize];
      int totSize = 0;

      foreach(VarDeclStatement vds; vardecs)
        foreach(VarDeclaration vd; vds.vars)
        {
          int size = vd.var.type.getSize();
          int[] val;
          totSize += size;

          val = vd.getCTimeValue();

          data[vd.var.number..vd.var.number+size] = val[];
        }
      // Make sure the total size of the variables match the total size
      // requested by variables through addNewDataVar.
      assert(totSize == dataSize, "Data size mismatch in scope");

      return data;
    }

  // Load file based on file name, class name, or both. The order of
  // the strings doesn't matter, and name2 can be empty. useCase
  // determines if we require a case sensitive match between the given
  // class name and the loaded name. If usePath is true we search the
  // include paths for scripts.
  void doLoad(char[] name1, char[] name2, bool useCase, bool usePath)
  {
    char[] fname, cname;

    if(name1 == "")
      fail("Cannot give empty first parameter to load()");

    if(name1.iEnds(".mn"))
      {
        fname = name1;
        cname = name2;
      }
    else
      {
        fname = name2;
        cname = name1;
      }

    if(cname.iEnds(".mn"))
      fail("load() recieved two filenames: " ~ fname ~ " and " ~ cname);

    // The filename must either be empty, or end with .mn
    if(fname != "" && !fname.iEnds(".mn"))
      fail("Neither " ~ name1 ~ " nor " ~ name2 ~
           " is a valid script filename.");

    // Remember if cname was originally set
    bool cNameSet = (cname != "");

    // Make sure both cname and fname have values.
    if(!cNameSet)
      cname = classFromFile(fname);
    else if(fname == "")
      fname = classToFile(cname);
    else
      // Both values were given, make sure they are sensible
      if(icmp(classFromFile(fname),cname) != 0)
        fail(format("Class name %s does not match file name %s",
                    cname, fname));

    assert(cname != "" && !cname.iEnds(".mn"));
    assert(fname.iEnds(".mn"));

    bool checkFileName()
      {
        if(cname.length == 0)
          return false;

        if(!validFirstIdentChar(cname[0]))
          return false;

        foreach(char c; cname)
          if(!validIdentChar(c)) return false;

        return true;
      }

    if(!checkFileName())
      fail(format("Invalid class name %s (file %s)", cname, fname));

    if(usePath && !vm.findFile(fname))
      fail("Cannot find script file " ~ fname);

    // Create a temporary file stream and load it
    auto bf = new BufferedFile(fname);
    auto ef = new EndianStream(bf);
    int bom = ef.readBOM();
    loadStream(ef, fname, bom);
    delete bf;

    // After the class is loaded, we can check it's real name.

    // If the name matches, we're done.
    if(cname == name.str) return;

    // Allow a case insensitive match if useCase is false or the name
    // was not given.
    if((!useCase || !cNameSet) && (icmp(cname, name.str) == 0)) return;

    // Oops, name mismatch
    fail(format("%s: Expected class name %s does not match loaded name %s",
                fname, cname, name.str));
    assert(0);
  }

  // Helper function for the bind() variants
  Function* bind_locate(char[] name, FuncType ft)
  {
    requireScope();

    // Look the function up in the scope
    auto ln = sc.lookupName(name);
    auto fn = ln.func;

    if(!ln.isFunc)
      fail("Cannot bind to '" ~ name ~ "': no such function");

    if(ft == FuncType.Idle)
      {
        if(!fn.isIdle())
          fail("Cannot bind to non-idle function '" ~ name ~ "'");
      }
    else
      {
        if(!fn.isNative())
          fail("Cannot bind to non-native function '" ~ name ~ "'");
      }

    fn.ftype = ft;

    return fn;
  }


  /*******************************************************
   *                                                     *
   *     Compiler-related private functions              *
   *                                                     *
   *******************************************************/

  // Identify what kind of block the given set of tokens represent,
  // parse them, and store it in the appropriate list;
  void store(ref TokenArray toks)
    {
      if(FuncDeclaration.canParse(toks))
	{
	  auto fd = new FuncDeclaration;
	  funcdecs ~= fd;
	  fd.parse(toks);
	}
      else if(VarDeclStatement.canParse(toks))
	{
	  auto vd = new VarDeclStatement;
	  vd.parse(toks);
	  vardecs ~= vd;
	}
      else if(StateDeclaration.canParse(toks))
	{
	  auto sd = new StateDeclaration;
	  sd.parse(toks);
	  statedecs ~= sd;
	}
      else if(StructDeclaration.canParse(toks))
        {
          auto sd = new StructDeclaration;
          sd.parse(toks);
          structdecs ~= sd;
        }
      else if(EnumDeclaration.canParse(toks))
        {
          auto sd = new EnumDeclaration;
          sd.parse(toks);
          enumdecs ~= sd;
        }
      else if(ImportStatement.canParse(toks))
        {
          auto sd = new ImportStatement;
          sd.parse(toks);
          imports ~= sd;
        }
      else
        fail("Illegal type or declaration", toks);
    }

  // Converts a stream to tokens and parses it.
  void parse(Stream str, char[] fname, int bom)
    {
      assert(str !is null);
      TokenArray tokens = tokenizeStream(fname, str, bom);
      parse(tokens, fname);
    }

  // Parses a list of tokens, and do other setup.
  void parse(ref TokenArray tokens, char[] fname)
    { 
      assert(!isParsed(), "parse() called on a parsed class " ~ name.str);

      alias Block.isNext isNext;

      natConst.ftype = FuncType.Native;
      natConst.name.str = "native constructor";
      natConst.owner = this;

      // TODO: Check for a list of keywords here. class, module,
      // abstract, final. They can come in any order, but only certain
      // combinations are legal. For example, class and module cannot
      // both be present, and most other keywords only apply to
      // classes. 'function' is not allowed at all, but should be
      // checked for to make sure we're loading the right kind of
      // file. If neither class nor module are found, that is also
      // illegal in class files.

      if(isNext(tokens, TT.Module))
        {
          flags.set(CFlags.Module);
          flags.set(CFlags.Singleton);
        }
      else if(isNext(tokens, TT.Singleton))
        flags.set(CFlags.Singleton);
      else if(!isNext(tokens, TT.Class))
	fail("File must begin with a class or module statement", tokens);

      if(!isNext(tokens, TT.Identifier, name))
	fail("Class statement expected identifier", tokens);

      // Module implies singleton
      assert(isSingleton || !isModule);

      if(isSingleton && isAbstract)
        fail("Modules and singletons cannot be abstract", name.loc);

      // Insert ourselves into the global scope. This will also
      // resolve forward references to this class, if any.
      global.insertClass(this);

      // Get the parent classes, if any
      if(isNext(tokens, TT.Colon))
        {
          if(isModule)
            fail("Inheritance not allowed for modules.");

          Token pName;
          do
            {
              if(!isNext(tokens, TT.Identifier, pName))
                fail("Expected parent class identifier", tokens);

              parentNames ~= pName;
            }
          while(isNext(tokens, TT.Comma));
        }

      if(!isNext(tokens, TT.Semicolon))
	fail("Missing semicolon after class statement", name.loc);

      if(parents.length > 1)
        fail("Multiple inheritance is currently not supported", name.loc);

      // Parse the rest of the file
      while(!isNext(tokens, TT.EOF)) store(tokens);

      // The tokenizer shouldn't allow more tokens after this point
      assert(tokens.length == 0, "found tokens after end of file");

      flags.set(CFlags.Parsed);
    }

  // Insert the class into the scope system. All parent classes must
  // be loaded before this is called.
  void createScope()
    {
      // Since debugging self inheritance can be a little icky, add an
      // explisit recursion check.
      assert(!flags.has(CFlags.InScope), "createScope called recursively");
      flags.set(CFlags.InScope);

      assert(isParsed());
      assert(!isScoped(), "createScope called on already scoped class " ~
             name.str);

      // Set the scoped flag - this makes sure we are not called
      // recursively below.
      flags.set(CFlags.Scoped);

      // Transfer the parent list
      parents.length = parentNames.length;
      foreach(int i, pName; parentNames)
        {
          // Find the class. findClass guarantees that the returned
          // class is scoped.
          MonsterClass mc = global.findClass(pName);

          assert(mc !is null);
          assert(mc.isScoped);

          parents[i] = mc;

          // Direct self inheritance
          if(mc is this)
            fail("Class " ~ name.str ~ " cannot inherit from itself",
                 name.loc);

          if(mc.isModule)
            fail("Cannot inherit from module " ~ mc.name.str);

          // If a parent class is not a forward reference and still
          // does not have a scope, it means that it is itself running
          // this function. This can only happen if we are a parent of
          // it.
          if(mc.sc is null)
            fail("Class " ~ name.str ~ " is a parent of itself (through "
                 ~ mc.name.str ~ ")", name.loc);
        }

      // For now we only support one parent class.
      assert(parents.length <= 1);

      // Since there's only one parent, we can copy its tree and add
      // ourselv to the list. TODO: At some point we need to
      // automatically add Object to this list.
      if(parents.length == 1)
        tree = parents[0].tree;
      else
        tree = null;
      tree = tree ~ this;
      treeIndex = tree.length-1;

      assert(tree.length > 0);
      assert(tree[$-1] is this);
      assert(tree[treeIndex] is this);

      // The parent scope is the scope of the parent class, or the
      // global scope if there is no parent.
      Scope parSc;
      if(parents.length != 0) parSc = parents[0].sc;
      // TODO: Should only be allowed for Object
      else parSc = global;

      assert(parSc !is null);

      // Create the scope for this class
      sc = new ClassScope(parSc, this);

      // Set the type
      objType = new ObjectType(this);
      classType = objType.getMeta();

      // Insert custom types first. This will never refer to other
      // identifiers.
      foreach(dec; structdecs)
        dec.insertType(sc);
      foreach(dec; enumdecs)
        dec.insertType(sc);

      // Resolve imports next. May refer to custom types, but no other
      // ids.
      foreach(dec; imports)
        dec.resolve(sc);

      // Then resolve the type headers.
      foreach(dec; structdecs)
        dec.resolve(sc);
      foreach(dec; enumdecs)
        dec.resolve(sc);

      // Resolve variable declarations. They will insert themselves
      // into the scope.
      foreach(dec; vardecs)
	dec.resolve(sc);

      // Add function declarations to the scope.
      foreach(dec; funcdecs)
	sc.insertFunc(dec.fn);

      // Ditto for states.
      foreach(dec; statedecs)
	sc.insertState(dec.st);

      // Resolve function headers.
      foreach(func; funcdecs)
	func.resolve(sc);

      // Set up the function and state lists
      functions.length = funcdecs.length;
      foreach(fn; funcdecs)
        functions[fn.fn.index] = fn.fn;

      states.length = statedecs.length;
      foreach(st; statedecs)
        states[st.st.index] = st.st;

      // Now set up the virtual function table. It's elements
      // correspond to the classes in tree[].

      if(parents.length)
        {
          // This will get a lot trickier if we allow multiple inheritance
          assert(parents.length == 1);

          // Set up the virtuals list
          auto pv = parents[0].virtuals;
          virtuals.length = pv.length+1;

          // We have to copy every single sublist, since we're not
          // allowed to change our parent's data
          foreach(i,l; pv)
            virtuals[i] = l.dup;

          // Add our own list
          virtuals[$-1] = functions;
        }
      else
        virtuals = [functions];

      assert(virtuals.length == tree.length);

      // Trace all our own functions back to their origin, and replace
      // them. Since we've copied our parents list, and assume it is
      // all set up, we only have to worry about our own
      // functions. (For multiple inheritance this might be a bit more
      // troublesome, but definitely doable.)
      foreach(fn; functions)
        {
          auto o = fn.overrides;

          // And we have to loop backwards through the overrides that
          // o overrides as well.
          while(o !is null)
            {
              // Find the owner class tree index of the function we're
              // overriding
              assert(o.owner !is this);
              int clsInd = o.owner.treeIndex;
              assert(clsInd < tree.length-1);
              assert(tree[clsInd] == o.owner);

              // Next, get the function index and replace the pointer
              virtuals[clsInd][o.index] = fn;

              // Get the function that o overrides too, and fix that
              // one as well.
              o = o.overrides;
            }
        }

      flags.unset(CFlags.InScope);
    }

  // This calls resolve on the interior of functions and states.
  void resolveBody()
    {
      if(!isScoped)
        createScope();

      assert(!isResolved, getName() ~ " is already resolved");

      // Resolve the functions
      foreach(func; funcdecs)
        func.resolveBody();

      // Resolve states
      foreach(state; statedecs)
	state.resolve(sc);

      // TODO: Resolve struct functions
      /*
      foredach(stru; structdecs)
        stru.resolveBody(sc);
      */

      // Validate all variable types
      foreach(var; vardecs)
        var.validate();

      flags.set(CFlags.Resolved);
    }

  alias int[] ia;
  // These are platform dependent:
  static const iasize = ia.sizeof / int.sizeof;

  void compileBody()
    {
      assert(!isCompiled, getName() ~ " is already compiled");
 
      // Resolve the class body if it's not already done
      if(!isResolved) resolveBody();

      // Require that all parent classes are compiled before us
      foreach(mc; tree[0..$-1])
        mc.requireCompile();

      // Generate data segment and byte code for functions and
      // states. The result is stored in the respective objects.
      foreach(f; funcdecs) f.compile();
      foreach(s; statedecs) s.compile();

      // Set the data segment for this class.
      data = getDataSegment();

      // Calculate the total data size we need to allocate for each
      // object
      uint tsize = 0;
      foreach(c; tree)
        {
          tsize += c.data.length; // Data segment size
          tsize += MonsterObject.exSize; // Extra data per object
          tsize += iasize; // The size of our entry in the data[]
                           // table
        }

      // Allocate the buffer
      totalData = new int[tsize];

      // Use this to get subslices of the data segment
      int[] slice = totalData;
      int[] get(int ints)
        {
          assert(ints <= slice.length);
          int[] res = slice[0..ints];
          slice = slice[ints..$];
          return res;
        }

      // Skip the data[] list
      get(iasize*tree.length);

      // Assign the data segment values
      foreach(c; tree)
        {
          int[] d = get(c.data.length);
          d[] = c.data[];

          // Skip the extra data
          get(MonsterObject.exSize);
        }

      // At this point we should have used up the entire slice
      assert(slice.length == 0);

      flags.set(CFlags.Compiled);

      // If it's a singleton, set up the object.
      if(isSingleton)
        {
          assert(singObj is null);
          singObj = createObject();
        }
    }
}

// Convert between class name and file name. These are currently just
// guesses. TODO: Move these into MC, into the user functions, or
// eliminate them completely.
char[] classToFile(char[] cname)
{
  return tolower(cname) ~ ".mn";
}

char[] classFromFile(char[] fname)
{
  fname = getBaseName(fname);
  assert(fname.ends(".mn"));
  return fname[0..$-3];
}

// Utility functions, might move elsewhere.
char[] getBaseName(char[] fullname)
{
  foreach_reverse(i, c; fullname)
    {
      version(Win32)
        {
          if(c == ':' || c == '\\' || c == '/')
            return fullname[i+1..$];
        }
      version(Posix)
        {
          if (fullname[i] == '/')
            return fullname[i+1..$];
        }
    }
  return fullname;
}
