/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (bindings.d) is part of the OpenMW package.

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

module gui.bindings;

extern(C):

// GUI functions. Under development. The corresponding C++ functions
// are in cpp_mygui.cpp

typedef void* WidgetPtr;
void gui_setupGUI(int debugOut);
void gui_toggleGui();
void gui_setCellName(char *str);

// Console stuff
void gui_toggleConsole();
void gui_setConsoleFont(char*);
void gui_clearConsole();

// Get the widget type, as a string
char *gui_widgetType(WidgetPtr);

// Get the guiMode flag
int *gui_getGuiModePtr();

// Get the height or width of a widget. If the argument is null,
// return the size of the screen.
int gui_getHeight(WidgetPtr);
int gui_getWidth(WidgetPtr);

// Set various properties of a given widget
void gui_setCaption(WidgetPtr, char*);
void gui_setNeedMouseFocus(WidgetPtr, int);
void gui_setTextColor(WidgetPtr, float,float,float);
void gui_setCoord(WidgetPtr, int,int,int,int);

// Various ways to get or create widgets
WidgetPtr gui_loadLayout(char *file, char *prefix, WidgetPtr parent);
WidgetPtr gui_getChild(WidgetPtr, char*);
WidgetPtr gui_createText(char *skin, int x, int y, int w, int h, char *layer);
