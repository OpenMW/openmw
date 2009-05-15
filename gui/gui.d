/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2009  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (gui.d) is part of the OpenMW package.

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

module gui.gui;

import monster.monster;
import monster.vm.mclass;
import monster.modules.console;
import gui.bindings;
import bullet.bindings;
import std.string;
import std.stdio;

// Widget class and gui module
MonsterClass
  gmc,      // GUI module
  wid_mc,   // Widget
  but_mc,   // Button
  tex_mc,   // StaticText
  img_mc,   // StaticImage
  pro_mc,   // Progress
  win_mc;   // Window

MWidget[WidgetPtr] gui_store;

class MWidget
{
  WidgetPtr widget;
  MonsterObject *mo;

  // Used for layouts
  bool isLayout;
  char[] prefix;

  // For layouts
  this(char[] layout, WidgetPtr parent = null)
  {
    assert(layout.length);

    isLayout = true;
    prefix = format("%s", cast(void*)this);

    widget = gui_loadLayout(layout.toStringz(), prefix.toStringz(), parent);
    if(widget is null)
      fail("Layout " ~ layout ~ " is empty");

    this();
  }

  // For normal widgets
  this(WidgetPtr w)
  {
    isLayout = false;
    widget = w;
    assert(widget !is null);
    this();
  }

  private this()
  {
    assert(widget !is null);

    // Create the object from the right class
    mo = toClass(widget).createObject();
    // But store the pointer in the Widget's pointer slot
    mo.getExtra(wid_mc).obj = this;
    // Also store a lookup for later
    gui_store[widget] = this;
  }

  MWidget getChild(char[] name)
  {
    if(prefix.length)
      name = prefix ~ name;

    // Get the child widget
    auto pt = gui_getChild(widget, name.toStringz());

    if(pt is null)
      fail("Widget has no child named " ~ name);

    // Return the MWidget
    return get(pt);
  }

  // Return the MonsterClass corresponding to a given widget.
  private static MonsterClass toClass(WidgetPtr p)
    {
      switch(widgetType(p))
        {
          /*
        case "Button": return but_mc;
        case "StaticText": return tex_mc;
        case "StaticImage": return img_mc;
        case "Progress": return pro_mc;
        case "Window": return win_mc;
          */
        default:
          // Use "Widget" for all unimplemented types
        case "Widget": return wid_mc;
        }
    }

  // Get the MWidget (and associatied MonsterObject) corresponding to
  // a given Widget.
  static MWidget get(WidgetPtr wid)
    {
      // First, check if the instance exists
      auto p = wid in gui_store;
      if(p) return *p;

      // No MWidget exists. We have to create one.
      return new MWidget(wid);
    }
};

char[] widgetType(WidgetPtr p)
{
  return toString(gui_widgetType(p));
}

MWidget getMWOwner(MonsterObject *mo)
{
  return (cast(MWidget)mo.getExtra(wid_mc).obj);
}
MWidget getMWOwner()
{
  return getMWOwner(params.obj());
}
WidgetPtr getOwner(MonsterObject *mo)
{
  return getMWOwner(mo).widget;
}
WidgetPtr getOwner()
{
  return getMWOwner().widget;
}

// Widget functions
void setCaption()
{
  AIndex[] args = stack.popAArray();

  char[] res;

  foreach(AIndex ind; args)
    res ~= format("%s", arrays.getRef(ind).carr);

  gui_setCaption(getOwner(), toStringz(res));
}
void setNeedMouseFocus()
{ gui_setNeedMouseFocus(getOwner(), stack.popBool); }
void setTextColor()
{
  float b = stack.popFloat();
  float g = stack.popFloat();
  float r = stack.popFloat();
  gui_setTextColor(getOwner(), r,g,b);
}
void setCoord()
{
  int h = stack.popInt();
  int w = stack.popInt();
  int y = stack.popInt();
  int x = stack.popInt();
  gui_setCoord(getOwner(), x,y,w,h);
}

void get()
{
  // Get the owner MWidget
  auto mw = getMWOwner();
  // Get the child
  mw = mw.getChild(stack.popString8());
  // Push the object
  stack.pushObject(mw.mo);
}

// TODO: These are all written to be used with the 'gui' module. Later
// they should be part of Widget, and create child widgets. When used
// with 'gui' they create root widgets.

void loadLayout()
{
  MWidget mw = new MWidget(stack.popString8());
  stack.pushObject(mw.mo);
}

void text()
{
  char[] layer = stack.popString8();
  int h = stack.popInt();
  int w = stack.popInt();
  int y = stack.popInt();
  int x = stack.popInt();
  char[] skin = stack.popString8();
  WidgetPtr ptr = gui_createText(skin.toStringz(),
                                 x,y,w,h,
                                 layer.toStringz());
  assert(widgetType(ptr) == "StaticText");
  MWidget mw = new MWidget(ptr);
  stack.pushObject(mw.mo);
}

void getWidth()
{
  stack.pushInt(gui_getWidth(null));
}

void getHeight()
{
  stack.pushInt(gui_getHeight(null));
}

void initGUI(bool debugOut)
{
  // Load the GUI system
  gui_setupGUI(debugOut);
}

void startGUI()
{
  gui_showHUD();

  // Run GUI scripts
  // Create the HUD and windows
  vm.run("makegui.mn");

  // Run the fps ticker
  vm.run("fpsticker.mn");
}

void setupGUIScripts()
{
  vm.addPath("mscripts/guiscripts/");
  vm.addPath("mscripts/guiscripts/module/");
  gmc = vm.load("gui", "gui.mn");
  wid_mc = vm.load("Widget", "widget.mn");
  /*
  but_mc = new MonsterClass("Button", "button.mn");
  tex_mc = new MonsterClass("Text", "text.mn");
  img_mc = new MonsterClass("Image", "image.mn");
  pro_mc = new MonsterClass("Progress", "progress.mn");
  win_mc = new MonsterClass("Window", "window.mn");
  */

  wid_mc.bind("setCaption", &setCaption);
  wid_mc.bind("setNeedMouseFocus", &setNeedMouseFocus);
  wid_mc.bind("setTextColor", &setTextColor);
  wid_mc.bind("setCoord", &setCoord);
  wid_mc.bind("get", &get);

  gmc.bind("text", &text);
  gmc.bind("loadLayout", &loadLayout);
  gmc.bind("getWidth", &getWidth);
  gmc.bind("getHeight", &getHeight);

  // Set up the console
  auto cmc = vm.load("Console");
  auto cmo = cmc.createObject;
  cons = new Console(cmo);

  // Bind native functions
  cmc.bind("walk", { bullet_walk(); cons.putln("Walk mode enabled");});
  cmc.bind("fly", { bullet_fly(); cons.putln("Fly mode enabled");});
  cmc.bind("ghost", { bullet_ghost(); cons.putln("Ghost mode enabled");});
  cmc.bind("setfont",
  {
    char[] fnt = stack.popString8();
    gui_setConsoleFont(toStringz(fnt));
    cons.putln("Setting font " ~ fnt);
  });
  cmc.bind("clear", { gui_clearConsole(); });
  cmc.bind("exit", { exitProgram(); });
  cmc.bind("wireframe", { cons.putln("Wireframe mode not implemented yet"); });
}

Console cons;

// Checked from input/events.d. Shouldn't really be here, but it's a
// workaround for a DMD import issue.
bool doExit = false;
void exitProgram()
{
  doExit = true;
}

// Some glue code that will go away later when we use the C++
// interface to Monster directly.
extern(C):
int console_input(char* str)
{
  char[] dstr = toString(str);
  return cons.input(dstr);
}

char* console_output()
{
  char[] dstr = cons.output();
  return toStringz(dstr);
}
