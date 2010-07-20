#ifndef MWGUI_CONSOLE_H
#define MWGUI_CONSOLE_H

#include <openengine/gui/layout.hpp>
#include <list>
#include <string>

namespace MWGui
{
  class Console : private OEngine::GUI::Layout
  {
  public:
    MyGUI::EditPtr command;
    MyGUI::EditPtr history;

    typedef std::list<std::string> StringList;

    // History of previous entered commands
    StringList command_history;
    StringList::iterator current;
    std::string editString;

    Console(int w, int h)
      : Layout("openmw_console_layout.xml")
    {
      setCoord(10,10, w-10, h/2);

      getWidget(command, "edit_Command");
      getWidget(history, "list_History");

      // Set up the command line box
      command->eventEditSelectAccept =
        newDelegate(this, &Console::acceptCommand);
      command->eventKeyButtonPressed =
        newDelegate(this, &Console::keyPress);

      // Set up the log window
      history->setOverflowToTheLeft(true);
      history->setEditStatic(true);
      history->setVisibleVScroll(true);
    }

    void enable()
    {
      setVisible(true);

      // Give keyboard focus to the combo box whenever the console is
      // turned on
      MyGUI::InputManager::getInstance().setKeyFocusWidget(command);
    }

    void disable()
    {
      setVisible(false);
    }

    void setFont(const std::string &fntName)
    {
      history->setFontName(fntName);
      command->setFontName(fntName);
    }

    void clearHistory()
    {
      history->setCaption("");
    }

    // Print a message to the console. Messages may contain color
    // code, eg. "#FFFFFF this is white".
    void print(const std::string &msg)
    { history->addText(msg); }

    // These are pre-colored versions that you should use.

    /// Output from successful console command
    void printOK(const std::string &msg)
    {  print("#FF00FF" + msg + "\n"); }

    /// Error message
    void printError(const std::string &msg)
    {  print("#FF2222" + msg + "\n"); }

  private:

    void keyPress(MyGUI::WidgetPtr _sender,
                  MyGUI::KeyCode key,
                  MyGUI::Char _char)
    {
      if(command_history.empty()) return;

      // Traverse history with up and down arrows
      if(key == MyGUI::KeyCode::ArrowUp)
        {
          // If the user was editing a string, store it for later
          if(current == command_history.end())
            editString = command->getCaption();

          if(current != command_history.begin())
            {
              current--;
              command->setCaption(*current);
            }
        }
      else if(key == MyGUI::KeyCode::ArrowDown)
        {
          if(current != command_history.end())
            {
              current++;

              if(current != command_history.end())
                command->setCaption(*current);
              else
                // Restore the edit string
                command->setCaption(editString);
            }
        }
    }

    void acceptCommand(MyGUI::EditPtr _sender)
    {
      const std::string &cm = command->getCaption();
      if(cm.empty()) return;

      // Add the command to the history, and set the current pointer to
      // the end of the list
      command_history.push_back(cm);
      current = command_history.end();
      editString.clear();

      // Log the command
      print("#FFFFFF> " + cm + "\n");

      /* NOTE: This is where the console command should be
         handled.

         The console command is in the string 'cm'. Output from the
         command should be put back into the console with the
         printOK() or printError() functions.
       */
      printOK("OK - echoing line " + cm);

      command->setCaption("");
    }
  };
}
#endif
