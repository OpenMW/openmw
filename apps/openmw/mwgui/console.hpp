#ifndef MWGUI_CONSOLE_H
#define MWGUI_CONSOLE_H

#include <list>
#include <string>
#include <vector>

#include <components/compiler/errorhandler.hpp>
#include <components/compiler/output.hpp>
#include <components/compiler/extensions.hpp>

#include "../mwscript/compilercontext.hpp"
#include "../mwscript/interpretercontext.hpp"

#include "referenceinterface.hpp"
#include "windowbase.hpp"

namespace MWGui
{
    class Console : public WindowBase, private Compiler::ErrorHandler, public ReferenceInterface
    {
        public:
            /// Set the implicit object for script execution
            void setSelectedObject(const MWWorld::Ptr& object);

            MyGUI::EditBox* mCommandLine;
            MyGUI::EditBox* mHistory;

            typedef std::list<std::string> StringList;

            // History of previous entered commands
            StringList mCommandHistory;
            StringList::iterator mCurrent;
            std::string mEditString;

            Console(int w, int h, bool consoleOnlyScripts);

            void onOpen() override;

            void onResChange(int width, int height) override;

            // Print a message to the console, in specified color.
            void print(const std::string &msg, const std::string& color = "#FFFFFF");

            // These are pre-colored versions that you should use.

            /// Output from successful console command
            void printOK(const std::string &msg);

            /// Error message
            void printError(const std::string &msg);

            void execute (const std::string& command);

            void executeFile (const std::string& path);

            void updateSelectedObjectPtr(const MWWorld::Ptr& currentPtr, const MWWorld::Ptr& newPtr);

            void clear() override;

            void resetReference () override;

        protected:

            void onReferenceUnavailable() override;

        private:

            void keyPress(MyGUI::Widget* _sender,
                          MyGUI::KeyCode key,
                          MyGUI::Char _char);

            void acceptCommand(MyGUI::EditBox* _sender);

            std::string complete( std::string input, std::vector<std::string> &matches );

            Compiler::Extensions mExtensions;
            MWScript::CompilerContext mCompilerContext;
            std::vector<std::string> mNames;
            bool mConsoleOnlyScripts;

            bool compile (const std::string& cmd, Compiler::Output& output);

            /// Report error to the user.
            void report (const std::string& message, const Compiler::TokenLoc& loc, Type type) override;

            /// Report a file related error
            void report (const std::string& message, Type type) override;

            /// Write all valid identifiers and keywords into mNames and sort them.
            /// \note If mNames is not empty, this function is a no-op.
            /// \note The list may contain duplicates (if a name is a keyword and an identifier at the same
            /// time).
            void listNames();
  };
}
#endif
