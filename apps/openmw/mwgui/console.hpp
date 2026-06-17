#ifndef MWGUI_CONSOLE_H
#define MWGUI_CONSOLE_H

#include <list>
#include <string>
#include <vector>

#include <components/compiler/errorhandler.hpp>
#include <components/compiler/extensions.hpp>
#include <components/compiler/output.hpp>
#include <components/files/configurationmanager.hpp>

#include "../mwbase/windowmanager.hpp"

#include "../mwscript/compilercontext.hpp"

#include "referenceinterface.hpp"
#include "windowbase.hpp"

namespace MWGui
{
    class Console : public WindowBase, private Compiler::ErrorHandler, public ReferenceInterface
    {
    public:
        /// Set the implicit object for script execution
        void setSelectedObject(const MWWorld::Ptr& object);
        MWWorld::Ptr getSelectedObject() const { return mPtr; }

        MyGUI::EditBox* mCommandLine;
        MyGUI::EditBox* mHistory;
        MyGUI::EditBox* mSearchTerm;
        MyGUI::Button* mNextButton;
        MyGUI::Button* mPreviousButton;
        MyGUI::Button* mCaseSensitiveToggleButton;
        MyGUI::Button* mRegExSearchToggleButton;

        typedef std::list<std::string> StringList;

        // History of previous entered commands
        StringList mCommandHistory;
        StringList::iterator mCurrent;
        std::string mEditString;
        std::ofstream mCommandHistoryFile;

        Console(int w, int h, bool consoleOnlyScripts, Files::ConfigurationManager& cfgMgr);
        ~Console();

        void onOpen() override;

        // Print a message to the console, in specified color.
        void print(const std::string& msg, std::string_view color = MWBase::WindowManager::sConsoleColor_Default);

        // These are pre-colored versions that you should use.

        /// Output from successful console command
        void printOK(const std::string& msg);

        /// Error message
        void printError(const std::string& msg);

        void execute(const std::string& command);

        void executeFile(const std::filesystem::path& path);

        void updateSelectedObjectPtr(const MWWorld::Ptr& currentPtr, const MWWorld::Ptr& newPtr);

        void onFrame(float dt) override { checkReferenceAvailable(); }
        void clear() override;

        void resetReference() override;

        const std::string& getConsoleMode() const { return mConsoleMode; }
        void setConsoleMode(std::string_view mode);

    protected:
        void onReferenceUnavailable() override;

    private:
        std::string mConsoleMode;

        void updateConsoleTitle();

        void commandBoxKeyPress(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char value);
        void acceptCommand(MyGUI::EditBox* sender);

        enum class SearchDirection;
        void toggleCaseSensitiveSearch(MyGUI::Widget* sender);
        void toggleRegExSearch(MyGUI::Widget* sender);
        void acceptSearchTerm(MyGUI::EditBox* sender);
        void findNextOccurrence(MyGUI::Widget* sender);
        void findPreviousOccurrence(MyGUI::Widget* sender);
        void findOccurrence(SearchDirection direction);
        void findInHistoryText(
            const std::string& historyText, SearchDirection direction, size_t firstIndex, size_t lastIndex);
        void findWithRegex(
            const std::string& historyText, SearchDirection direction, size_t firstIndex, size_t lastIndex);
        void findWithStringSearch(
            const std::string& historyText, SearchDirection direction, size_t firstIndex, size_t lastIndex);
        void markOccurrence(size_t textPosition, size_t length);
        size_t mCurrentOccurrenceIndex = std::string::npos;
        size_t mCurrentOccurrenceLength = 0;
        std::string mCurrentSearchTerm;
        bool mCaseSensitiveSearch;
        bool mRegExSearch;

        std::string complete(std::string input, std::vector<std::string>& matches);

        Compiler::Extensions mExtensions;
        MWScript::CompilerContext mCompilerContext;
        std::vector<std::string> mNames;

        bool mConsoleOnlyScripts;
        Files::ConfigurationManager& mCfgMgr;
        bool compile(const std::string& cmd, Compiler::Output& output);

        /// Report error to the user.
        void report(const std::string& message, const Compiler::TokenLoc& loc, Type type) override;

        /// Report a file related error
        void report(const std::string& message, Type type) override;

        /// Write all valid identifiers and keywords into mNames and sort them.
        /// \note If mNames is not empty, this function is a no-op.
        /// \note The list may contain duplicates (if a name is a keyword and an identifier at the same
        /// time).
        void listNames();

        void initConsoleHistory();
    };
}
#endif
