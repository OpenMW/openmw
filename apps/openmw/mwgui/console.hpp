#ifndef MWGUI_CONSOLE_H
#define MWGUI_CONSOLE_H

#include <openengine/gui/layout.hpp>
#include <list>
#include <string>
#include <vector>

#include <components/compiler/errorhandler.hpp>
#include <components/compiler/lineparser.hpp>
#include <components/compiler/scanner.hpp>
#include <components/compiler/locals.hpp>
#include <components/compiler/output.hpp>
#include <components/compiler/extensions.hpp>
#include <components/interpreter/interpreter.hpp>

#include "../mwscript/compilercontext.hpp"
#include "../mwscript/interpretercontext.hpp"

#include "referenceinterface.hpp"

namespace MWGui
{
  class Console : private OEngine::GUI::Layout, private Compiler::ErrorHandler, public ReferenceInterface
  {
    private:

        Compiler::Extensions mExtensions;
        MWScript::CompilerContext mCompilerContext;
        std::vector<std::string> mNames;
        bool mConsoleOnlyScripts;

        bool compile (const std::string& cmd, Compiler::Output& output);

        /// Report error to the user.
        virtual void report (const std::string& message, const Compiler::TokenLoc& loc, Type type);

        /// Report a file related error
        virtual void report (const std::string& message, Type type);

        void listNames();
        ///< Write all valid identifiers and keywords into mNames and sort them.
        /// \note If mNames is not empty, this function is a no-op.
        /// \note The list may contain duplicates (if a name is a keyword and an identifier at the same
        /// time).

    public:

        void setSelectedObject(const MWWorld::Ptr& object);
        ///< Set the implicit object for script execution

    protected:

        virtual void onReferenceUnavailable();


    public:
    MyGUI::EditBox* command;
    MyGUI::EditBox* history;

    typedef std::list<std::string> StringList;

    // History of previous entered commands
    StringList command_history;
    StringList::iterator current;
    std::string editString;

    Console(int w, int h, bool consoleOnlyScripts);

    void enable();

    void disable();

    void setFont(const std::string &fntName);

    void onResChange(int width, int height);

    void clearHistory();

    // Print a message to the console. Messages may contain color
    // code, eg. "#FFFFFF this is white".
    void print(const std::string &msg);

    // These are pre-colored versions that you should use.

    /// Output from successful console command
    void printOK(const std::string &msg);

    /// Error message
    void printError(const std::string &msg);

    void execute (const std::string& command);

    void executeFile (const std::string& command);

  private:

    void keyPress(MyGUI::Widget* _sender,
                  MyGUI::KeyCode key,
                  MyGUI::Char _char);

    void acceptCommand(MyGUI::EditBox* _sender);

    std::string complete( std::string input, std::vector<std::string> &matches );
  };
}
#endif
