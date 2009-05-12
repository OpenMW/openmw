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

class Console : public Layout
{
  MyGUI::EditPtr command;
  MyGUI::EditPtr history;

public:
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

    if(cm == "big")
      history->setFontName("youtube");

    history->addText(cm + "\n");
    history->addText("this is a fake output result\n");
    command->setCaption("");
  }
};

Console *cons;

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
