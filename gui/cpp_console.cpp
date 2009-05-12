/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (cpp_console.cpp) is part of the OpenMW package.

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

// These are defined in gui/gui.d. At some point later we will just
// use the C++ bindings included with Monster, but these don't cover
// the console stuff yet.
enum
  {
    CR_OK = 1,      // Command was executed
    CR_ERROR = 2,   // An error occured
    CR_MORE = 3,    // More input is needed
    CR_EMPTY = 4    // The line had no effect
  };

extern "C" int32_t console_input(const char* command);
extern "C" char* console_output();

class Console : public Layout
{
public:
  MyGUI::EditPtr command;
  MyGUI::EditPtr history;

  Console()
    : Layout("openmw_console_layout.xml")
  {
    setCoord(0,0,
             mWindow->getWidth()*2/3, mWindow->getHeight()/2);

    getWidget(command, "edit_Command");
    getWidget(history, "list_History");

    // Set up the command line combobox
    command->eventEditSelectAccept =
      newDelegate(this, &Console::acceptCommand);

    // Set up the log window
    history->setOverflowToTheLeft(true);
    history->setEditStatic(true);
    history->setVisibleVScroll(true);
  }

  void takeFocus()
  {
    // Give keyboard focus to the combo box whenever the console is
    // turned on
    MyGUI::InputManager::getInstance().setKeyFocusWidget(command);
  }

  void acceptCommand(MyGUI::EditPtr _sender)
  {
    const Ogre::UTFString &cm = command->getCaption();
    if(cm.empty()) return;

    history->addText("#FFFFFF" + cm + "\n");

    int res = console_input(cm.asUTF8_c_str());
    Ogre::UTFString out = console_output();

    if(res == CR_OK)
      history->addText("#FF00FF" + out);
    else if(res == CR_ERROR)
      history->addText("#FF2222" + out);
    else if(res == CR_MORE)
      history->addText("#1111FF... more input needed");

  exit:
    command->setCaption("");
  }
};

Console *cons;

extern "C" void gui_setConsoleFont(const char* fntName)
{
  cons->history->setFontName(fntName);
}

extern "C" void gui_clearConsole()
{
  cons->history->setCaption("");
}

extern "C" void gui_toggleConsole()
{
  if(consoleMode)
    {
      leaveGui();
      if(cons)
        cons->setVisible(false);
    }
  else
    {
      enterGui();
      if(cons)
        {
          cons->setVisible(true);
          cons->takeFocus();
        }
    }

  consoleMode = !consoleMode;
}
