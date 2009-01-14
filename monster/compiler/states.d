/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (states.d) is part of the Monster script language
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

module monster.compiler.states;

import monster.compiler.scopes;
import monster.compiler.assembler;
import monster.compiler.tokenizer;
import monster.compiler.linespec;
import monster.compiler.statement;
import monster.vm.mclass;
import monster.vm.error;

import monster.util.aa;

import std.stdio;

struct State
{
  Token name;
  int index;

  // Labels in this scope.
  HashTable!(char[], StateLabel*) labels;
  StateLabel* labelList[];

  StateScope sc; // Scope for this state
  MonsterClass owner; // Class where this state was defined

  // State declaration - used to resolve forward references. Should
  // not be kept around when compilation is finished.
  StateDeclaration stateDec;

  ubyte[] bcode;
  LineSpec[] lines;

  StateLabel* findLabel(char[] name)
    {
      StateLabel *lb;
      if(labels.inList(name, lb))
        return lb;
      return null;
    }

  // Look up a label in this state, or register a forward reference if
  // the state hasn't been resolved yet.
  void registerGoto(char[] label, LabelUser lu)
    {
      StateLabel *sl;

      assert(lu !is null);

      if( labels.inList(label, sl) )
        lu.setLabel(sl.ls);
      else
        {
          if(stateDec is null)
            {
              // The state has been resolved, and the label was not
              // found. Let lu handle the error message.
              lu.setLabel(null);
              assert(0);
            }

          with(stateDec)
            {
              // The state is not resolved yet, so create a forward
              // reference to this label.
              Forward *fw;

              // Get the pointer to the Forward struct in the AA, or a
              // new one if none existed.
              forwards.insertEdit(label, fw);

              // Add the reference to the list
              fw.lus ~= lu;
            }
        }
    }
}

struct StateLabel
{
  Token name;
  uint offs;
  uint index; // Index used to represent this label in byte code
  LabelStatement ls; // TODO: Remove this later?
}

// Simple struct used for representing a label and its state in one
// value.
struct StateLabelPair
{
  State *state;
  StateLabel *label;
}

// Handles declaration of states at the class scope. Uses a code block
// for state contents.
class StateDeclaration : Statement
{
  State *st;

  CodeBlock code;

  static struct Forward
    { LabelUser lus[]; }

  HashTable!(char[], Forward) forwards;

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.State);
    }

  void parse(ref TokenArray toks)
    {
      st = new State;
      st.stateDec = this;

      if(!isNext(toks, TT.State))
	assert(0, "Internal error in StateDeclaration");

      if(!isNext(toks, TT.Identifier, st.name))
	fail("Expected state name identifier", toks);

      // Create a code block, and tell it (the parameter) that it is a
      // state block.
      code = new CodeBlock(true);
      code.parse(toks);
    }

  // Resolve the state. Besides resolving the code we have to resolve
  // any forward references to labels within the state afterwards.
  void resolve(Scope last)
    {
      assert(st !is null);
      // Create a state scope. The scope will help enforce special
      // rules, such as allowing idle functions and disallowing
      // variable declarations.
      st.sc = new StateScope(last, st);
      st.owner = st.sc.getClass();

      // Resolve the interior of the code block
      assert(code !is null);
      code.resolve(st.sc);

      // Go through the forward list and resolve everything
      foreach(char[] label, Forward fd; forwards)
        {
          StateLabel *sl;
          LabelStatement ls;
          if(st.labels.inList(label, sl))
            {
              assert(sl !is null);
              ls = sl.ls;
            }
          else
            ls = null; // Give a null to setLabel and let it handle
                       // the error message.

          // Loop through the label users
          foreach(LabelUser lu; fd.lus)
            lu.setLabel(ls);

          // setLabel should have thrown an error at this point
          assert(ls !is null);
        }

      // Clear the forwards list
      forwards.reset();

      // At this point the State no longer needs to refer to us. Set
      // the stateDec reference to null. This is also a signal to
      // State.registerGoto that the state has been resolved, and no
      // further forward references will be accepted.
      st.stateDec = null;

      // After the code has been resolved, all labels should now be
      // registered in the 'labels' list. We must assign a number to
      // each label for later reference. We also set up the labelList
      // which can be used to look up the labels directly.
      int cnt = 0;
      st.labelList.length = st.labels.length;
      foreach(char[] name, StateLabel *sl; st.labels)
        {
          assert(sl !is null);
          assert(name == sl.name.str, "label name mismatch");
          sl.index = cnt++;
          st.labelList[sl.index] = sl;
        }
    }

  // Compile it as a function.
  void compile()
    {
      // No forward references must be inserted after the state has
      // been resolved.
      assert(forwards.length == 0);

      tasm.newFunc();
      code.compile();

      // Table used to fetch the offset for the labels in this state.
      uint offsets[];

      offsets.length = st.labels.length;

      // Assemble the code and get the offsets
      st.bcode = tasm.assemble(st.lines, offsets);

      // Store the offsets in the label statements themselves, for
      // later use
      int cnt = 0;
      foreach(StateLabel* ls; st.labels)
        {
          ls.offs = offsets[ls.index];
        }
    }

  char[] toString()
    {
      return
	"State declaration: " ~
	st.name.str ~ "\n" ~
	code.toString();
    }
}
