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
import monster.compiler.block;

import monster.vm.vm;
import monster.vm.codestream;
import monster.vm.scheduler;
import monster.vm.idlefunction;
import monster.vm.fstack;
import monster.vm.arrays;
import monster.vm.error;
import monster.vm.mobject;

import monster.util.flags;
import monster.util.freelist;
import monster.util.string;

import monster.minibos.string;
import monster.minibos.stdio;
import monster.minibos.file;
import monster.minibos.stream;

// TODO: Needed to fix DMD/GDC template problems. Remove if this bug
// is fixed.
import monster.util.list;
alias _lstNode!(CodeThread) _tmp1;
alias __FreeNode!(CodeThread) _tmp2;

alias _lstNode!(MonsterObject) _tmp3;
alias __FreeNode!(MonsterObject) _tmp4;

typedef void *MClass; // Pointer to C++ equivalent of MonsterClass.

typedef int CIndex;

// Parameter to the constructor. Decides how the class is created.
enum MC
  {
    None = 0,    // Initial value
    File = 1,    // Load class from file (default)
    NoCase = 2,  // Load class from file, case insensitive name match
    String = 3,  // Load class from string
    Stream = 4,  // Load class from stream
    Manual = 5,  // Manually create class
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
  }

// The class that handles 'classes' in Monster.
final class MonsterClass
{
  /***********************************************
   *                                             *
   *    Static path functions                    *
   *                                             *
   ***********************************************/
  // TODO: These will probably be moved elsewhere.

  // Path to search for script files. Extremely simple at the moment.
  static char[][] includes = [""];

  static void addPath(char[] path)
  {
    // Make sure the path is slash terminated.
    if(!path.ends("/") && !path.ends("\\"))
      path ~= '/';

    includes ~= path;
  }

  // Search for a file in the various paths. Returns true if found,
  // false otherwise. Changes fname to point to the correct path.
  static bool findFile(ref char[] fname)
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

  /***********************************************
   *                                             *
   *    Static class functions                   *
   *                                             *
   ***********************************************/

  // Get a class with the given name. It must already be loaded.
  static MonsterClass get(char[] name) { return global.getClass(name); }

  // Find a class with the given name. Load the file if necessary, and
  // fail if the class cannot be found.
  static MonsterClass find(char[] name) { return global.findClass(name); }

 final:

  /*******************************************************
   *                                                     *
   *     Variables                                       *
   *                                                     *
   *******************************************************/

  alias FreeList!(CodeThread) ThreadList;
  alias FreeList!(MonsterObject) ObjectList;

  // TODO: Put as many of these as possible in the private
  // section. Ie. move all of them and see what errors you get.

  // Contains the entire class tree for this class, always with
  // ourselves as the last entry. Any class in the list is always
  // preceded by all the classes it inherits from.
  MonsterClass tree[];

  // Index within the parent tree. This might become a list at some
  // point.
  int treeIndex;

  Token name; // Class name and location

  CIndex gIndex; // Global index of this class

  ClassScope sc;

  ObjectType objType; // Type for objects of this class
  Type classType; // Type for class references to this class (not
                  // implemented yet)

  Flags!(CFlags) flags;

  bool isParsed() { return flags.has(CFlags.Parsed); }
  bool isScoped() { return flags.has(CFlags.Scoped); }
  bool isResolved() { return flags.has(CFlags.Resolved); }
  bool isCompiled() { return flags.has(CFlags.Compiled); }

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

  // List of variables and functions declared in this class, ordered
  // by index.
  Function* functions[];
  Variable* vars[];
  State* states[];


  /*******************************************************
   *                                                     *
   *     Constructors                                    *
   *                                                     *
   *******************************************************/

  // By default we leave the loadType at None. This leaves us open to
  // define the class later. Calling eg. setName will define the class
  // as a manual class.
  this() {}

  this(MC type, char[] name1, char[] name2 = "", bool usePath = true)
    {
      loadType = type;

      if(type == MC.File || type == MC.NoCase)
        {
          loadType = MC.File;

          if(type == MC.NoCase)
            loadCI(name1, name2, usePath);
          else
            load(name1, name2, usePath);

          return;
        }

      if(type == MC.String)
        {
          assert(name2 == "", "MC.String only takes one parameter");
          loadString(name1);

          return;
        }

      if(type == MC.Manual)
        {
          assert(name2 == "", "MC.Manual only takes one parameter");
          setName(name1);
          return;
        }

      assert(0, "encountered unknown MC type");
    }

  this(MC type, Stream str, char[] nam = "")
    {
      assert(type == MC.Stream);
      loadType = type;
      loadStream(str, nam);
    }

  this(Stream str, char[] nam="")
    { this(MC.Stream, str, nam); }

  this(char[] nam1, char[] nam2 = "", bool usePath=true)
    { this(MC.File, nam1, nam2, usePath); }

  // Used when binding to classes in other languages.
  this(MClass cpp, char* nam1, char* nam2 = null) { assert(0); }


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

  void loadString(char[] str)
  {
    assert(str != "");
    auto ms = new MemoryStream(str);
    loadStream(ms, "(string)");
  }

  // Load a script from a stream. The filename parameter is only used
  // for error messages.
  void load(Stream str, char[] fname="(stream)")
    { loadStream(str, fname); }

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
  void bind(char[] name, c_callback nf)
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

  // Find a given callable function.
  Function *findFunction(char[] name)
  {
    requireScope();

    // Get the function from the scope
    auto fn = sc.findFunc(name);

    if(fn is null)
      fail("Function '" ~ name ~ "' not found.");

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

    return fn;
  }

  /*******************************************************
   *                                                     *
   *     Binding of constructors                         *
   *                                                     *
   *******************************************************/

  void bindConst(dg_callback nf)
    {
      assert(constType == FuncType.Native,
             "Cannot set native constructor for " ~ toString ~ ": already set");
      constType = FuncType.NativeDDel;
      dg_const = nf;
    }

  void bindConst(fn_callback nf)
    {
      assert(constType == FuncType.Native,
             "Cannot set native constructor for " ~ toString ~ ": already set");
      constType = FuncType.NativeDFunc;
      fn_const = nf;
    }

  void bindConst(c_callback nf)
    {
      assert(constType == FuncType.Native,
             "Cannot set native constructor for " ~ toString ~ ": already set");
      constType = FuncType.NativeCFunc;
      c_const = nf;
    }


  /*******************************************************
   *                                                     *
   *     Management of member variables                  *
   *                                                     *
   *******************************************************/

  Variable* findVariable(char[] name)
    {
      requireScope();

      Variable *vb = sc.findVar(name);

      if(vb is null)
        fail("Variable " ~ name ~ " not found");

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

    State *st = sc.findState(name);

    if(st is null)
      fail("State " ~ name ~ " not found");

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
    { return objects.opApply(del); }

  // Get the first object in the 'objects' list. Used for
  // iterator-like looping through objects, together with getNext in
  // MonsterObject. Returns null if no objects exist.
  MonsterObject* getFirst()
    { return objects.getHead(); }

  // Create a new object, and assign a thread to it.
  MonsterObject* createObject()
    {
      requireCompile();

      // Create the thread
      CodeThread *trd = threads.getNew();

      // Create an object tree equivalent of the class tree
      MonsterObject* otree[];
      otree.length = tree.length;

      assert(otree.length > 0);

      // Fill the list with objects, and assign the thread.
      foreach(i, ref obj; otree)
        {
          obj = tree[i].getObject();
          obj.thread = trd;
        }

      // Pick out the top object
      MonsterObject* top = otree[$-1];

      assert(tree[$-1] is this);
      assert(top !is null);

      // Initialize the thread
      trd.initialize(top);

      // For each object we assign a slice of the object list. TODO:
      // In the future it's likely that these lists might have
      // different contents from each other (eg. in the case of
      // multiple inheritance), and simple slices will not be good
      // enough. This is the main reason why we give each object its
      // own tree, instead of using one shared list in the thread.
      foreach(i, ref obj; otree)
        obj.tree = otree[0..i+1];

      assert(top.tree == otree);

      return top;
    }

  // Free an object and its thread
  void deleteObject(MonsterObject *obj)
    {
      assert(obj.cls is this);

      // Get the head object
      obj = obj.thread.topObj;

      // Shut down any active code in the thread
      obj.thread.setState(null, null);

      // Destruct the objects in reverse order
      foreach_reverse(ob; obj.thread.topObj.tree)
        ob.cls.returnObject(ob);

      // Put the thread back into the free list
      threads.remove(obj.thread);
    }


  /*******************************************************
   *                                                     *
   *     Misc. functions                                 *
   *                                                     *
   *******************************************************/

  /* For Manual classes. These are just ideas, not implemented yet
  void addNative(char[] name, dg_callback dg) {}
  void addNative(char[] name, fn_callback fn) {}

  // Not for manual classes, but intended for reloading a changed
  // file. It will replace the current class in the scope with a new
  // one - and all new objects created will be of the new type
  // (requires some work on vm.d and scope.d to get this to work). Old
  // objects keep the old class. An alternative is to convert the old
  // objects to the new class in some way, if possible.
  void reload() {}
  */

  // Will set the name of the class. Can only be called on manual
  // classes, and only once. Not implemented yet.
  void setName(char[] name) {assert(0);}

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

  // Get the global index of this class
  CIndex getIndex() { requireScope(); return gIndex; }
  char[] getName() { assert(name.str != ""); return name.str; }
  char[] toString() { return getName(); }


  /*******************************************************
   *                                                     *
   *     Private and lower-level members                 *
   *                                                     *
   *******************************************************/

  void reserveStatic(int length)
    {
      assert(!isResolved);
      assert(sdata.length == 0);

      sdSize += length;
    }

  AIndex insertStatic(int[] array, int elemSize)
    {
      assert(isResolved);

      // Allocate data, if it has not been done already.
      if(sdata.length == 0 && sdSize != 0)
        sdata.length = sdSize;

      assert(array.length <= sdSize,
             "Trying to allocate more than reserved size");

      // How much will be left after inserting this array?
      int newSize = sdSize - array.length;
      assert(newSize >= 0);

      int[] slice = sdata[$-sdSize..$-newSize];
      sdSize = newSize;

      // Copy the data
      slice[] = array[];

      ArrayRef *arf = arrays.createConst(slice, elemSize);
      return arf.getIndex();
    }

 private:


  /*******************************************************
   *                                                     *
   *     Private variables                               *
   *                                                     *
   *******************************************************/

  // The freelists used for allocation of objects and threads.
  ObjectList objects;
  ThreadList threads;

  int[] data; // Contains the initial object data segment
  int[] sdata; // Static data segment

  uint sdSize; // Number of ints reserved for the static data, that
               // have not yet been used.

  // Direct parents of this class
  MonsterClass parents[];
  Token parentNames[];

  VarDeclStatement[] vardecs;
  FuncDeclaration[] funcdecs;
  StateDeclaration[] statedecs;

  MC loadType = MC.None;

  // Native constructor type
  FuncType constType = FuncType.Native;
  union
  {
    dg_callback dg_const;
    fn_callback fn_const;
    c_callback c_const;
  }


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
      int[] data = new int[sc.getDataSize];
      int totSize = 0;

      foreach(VarDeclStatement vds; vardecs)
        foreach(VarDeclaration vd; vds.vars)
        {
          int size = vd.var.type.getSize();
          int[] val;
          totSize += size;

          // Does this variable have an initializer?
          if(vd.init !is null)
            {
              // And can it be evaluated at compile time?
              if(!vd.init.isCTime)
                fail("Expression " ~ vd.init.toString ~
                     " is not computable at compile time", vd.init.loc);

              val = vd.init.evalCTime();
            }
          // Use the default initializer.
          else val = vd.var.type.defaultInit();

          assert(val.length == size, "Size mismatch");

          data[vd.var.number..vd.var.number+size] = val[];
        }
      // Make sure the total size of the variables match the total size
      // requested by variables through addNewDataVar.
      assert(totSize == sc.getDataSize, "Data size mismatch in scope");

      return data;
    }

  // Get an object from this class (but do not assign a thread to it)
  MonsterObject *getObject()
    {
      requireCompile();

      MonsterObject *obj = objects.getNew();

      // Set the class
      obj.cls = this;

      // TODO: Better memory management here. I have been thinking
      // about a general freelist manager, that works with object
      // sizes rather than with templates. That would work with
      // objects of any size, and could also be used directly from C /
      // C++. If we have one for every size we might end up with a
      // whole lot freelists though. Maybe we can pool the sizes, for
      // example use one for 16 bytes, one for 64, 128, 256, 1k, 4k,
      // etc. We will have to make the system and do some statistics
      // to see what sizes are actually used. The entire structure can
      // reside inside it's own region.

      // Copy the data segment
      obj.data = data.dup;

      // Point to the static data segment
      obj.sdata = sdata;

      // Call the custom native constructor
      if(constType != FuncType.Native)
        {
          fstack.pushNConst(obj);
          if(constType == FuncType.NativeDDel)
            dg_const();
          else if(constType == FuncType.NativeDFunc)
            fn_const();
          else if(constType == FuncType.NativeCFunc)
            c_const();
          fstack.pop();
        }

      return obj;
    }

  // Delete an object belonging to this class
  void returnObject(MonsterObject *obj)
    {
      // Put it back into the freelist
      objects.remove(obj);
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

    if(usePath && !findFile(fname))
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
    auto fn = sc.findFunc(name);

    if(fn is null)
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
      // canParse() is not ment as a complete syntax test, only to be
      // enough to identify which Block parser to apply.
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
      else
        fail("Illegal type or declaration", toks);
    }

  // Converts a stream to tokens and parses it.
  void parse(Stream str, char[] fname, int bom)
    { 
      assert(!isParsed(), "parse() called on a parsed class " ~ name.str);

      assert(str !is null);

      TokenArray tokens = tokenizeStream(fname, str, bom);

      alias Block.isNext isNext;

      if(!isNext(tokens, TT.Class))
	fail("File must begin with a valid class statement");

      if(!isNext(tokens, TT.Identifier, name))
	fail("Class statement expected identifier", tokens);

      // Insert ourselves into the global scope. This will also
      // resolve forward references to this class, if any.
      global.insertClass(this);

      // Get the parent classes, if any
      if(isNext(tokens, TT.Colon))
        {
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
          // Find the class
          MonsterClass mc = global.findClass(pName);

          assert(mc !is null);
          assert(mc.isScoped);

          parents[i] = mc;

          // Direct self inheritance
          if(mc is this)
            fail("Class " ~ name.str ~ " cannot inherit from itself",
                 name.loc);

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

      // Resolve variable declarations. They will insert themselves
      // into the scope. TODO: Init values will have to be handled as
      // part of the body later.
      foreach(dec; vardecs)
	dec.resolve(sc);

      // Add function declarations to the scope.
      foreach(dec; funcdecs)
	sc.insertFunc(dec.fn);

      // Ditto for states.
      foreach(dec; statedecs)
	sc.insertState(dec.st);

      // Resolve function headers. Here too, the init values will have
      // to be moved to the body. We still need the parameter and
      // return types though.
      foreach(func; funcdecs)
	func.resolve(sc);

      // Set up the function and state lists
      functions.length = funcdecs.length;
      foreach(fn; funcdecs)
        functions[fn.fn.index] = fn.fn;

      states.length = statedecs.length;
      foreach(st; statedecs)
        states[st.st.index] = st.st;

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

      // Validate all variable types
      foreach(var; vardecs)
        var.validate();

      flags.set(CFlags.Resolved);
    }

  void compileBody()
    {
      assert(!isCompiled, getName() ~ " is already compiled");
 
      // Resolve the class body if it's not already done
      if(!isResolved) resolveBody();

      // Generate data segment and byte code for functions and
      // states. The result is stored in the respective objects.
      foreach(f; funcdecs) f.compile();
      foreach(s; statedecs) s.compile();

      // Set the data segment. TODO: Separate static data from
      // variables.
      data = getDataSegment();

      flags.set(CFlags.Compiled);
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
