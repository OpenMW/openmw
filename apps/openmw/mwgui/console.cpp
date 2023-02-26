#include "console.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_LayerManager.h>

#include <filesystem>
#include <fstream>

#include <components/compiler/exception.hpp>
#include <components/compiler/extensions0.hpp>
#include <components/compiler/lineparser.hpp>
#include <components/compiler/locals.hpp>
#include <components/compiler/scanner.hpp>
#include <components/interpreter/interpreter.hpp>
#include <components/misc/utf8stream.hpp>
#include <components/settings/settings.hpp>

#include "../mwscript/extensions.hpp"
#include "../mwscript/interpretercontext.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWGui
{
    class ConsoleInterpreterContext : public MWScript::InterpreterContext
    {
        Console& mConsole;

    public:
        ConsoleInterpreterContext(Console& console, MWWorld::Ptr reference);

        void report(const std::string& message) override;
    };

    ConsoleInterpreterContext::ConsoleInterpreterContext(Console& console, MWWorld::Ptr reference)
        : MWScript::InterpreterContext(reference.isEmpty() ? nullptr : &reference.getRefData().getLocals(), reference)
        , mConsole(console)
    {
    }

    void ConsoleInterpreterContext::report(const std::string& message)
    {
        mConsole.printOK(message);
    }

    bool Console::compile(const std::string& cmd, Compiler::Output& output)
    {
        try
        {
            ErrorHandler::reset();

            std::istringstream input(cmd + '\n');

            Compiler::Scanner scanner(*this, input, mCompilerContext.getExtensions());

            Compiler::LineParser parser(
                *this, mCompilerContext, output.getLocals(), output.getLiterals(), output.getCode(), true);

            scanner.scan(parser);

            return isGood();
        }
        catch (const Compiler::SourceException&)
        {
            // error has already been reported via error handler
        }
        catch (const std::exception& error)
        {
            printError(std::string("Error: ") + error.what());
        }

        return false;
    }

    void Console::report(const std::string& message, const Compiler::TokenLoc& loc, Type type)
    {
        std::ostringstream error;
        error << "column " << loc.mColumn << " (" << loc.mLiteral << "):";

        printError(error.str());
        printError((type == ErrorMessage ? "error: " : "warning: ") + message);
    }

    void Console::report(const std::string& message, Type type)
    {
        printError((type == ErrorMessage ? "error: " : "warning: ") + message);
    }

    void Console::listNames()
    {
        if (mNames.empty())
        {
            // keywords
            std::istringstream input("");

            Compiler::Scanner scanner(*this, input, mCompilerContext.getExtensions());

            scanner.listKeywords(mNames);

            // identifier
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            std::vector<ESM::RefId> ids;
            for (MWWorld::ESMStore::iterator it = store.begin(); it != store.end(); ++it)
            {
                (*it)->listIdentifier(ids);
                for (auto id : ids)
                {
                    mNames.push_back(id.getRefIdString());
                }
                ids.clear();
            }

            // exterior cell names aren't technically identifiers, but since the COC function accepts them,
            // we should list them too
            for (MWWorld::Store<ESM::Cell>::iterator it = store.get<ESM::Cell>().extBegin();
                 it != store.get<ESM::Cell>().extEnd(); ++it)
            {
                if (!it->mName.empty())
                    mNames.push_back(it->mName);
            }

            // sort
            std::sort(mNames.begin(), mNames.end());

            // remove duplicates
            mNames.erase(std::unique(mNames.begin(), mNames.end()), mNames.end());
        }
    }

    Console::Console(int w, int h, bool consoleOnlyScripts, Files::ConfigurationManager& cfgMgr)
        : WindowBase("openmw_console.layout")
        , mCompilerContext(MWScript::CompilerContext::Type_Console)
        , mConsoleOnlyScripts(consoleOnlyScripts)
        , mCfgMgr(cfgMgr)
    {
        setCoord(10, 10, w - 10, h / 2);

        getWidget(mCommandLine, "edit_Command");
        getWidget(mHistory, "list_History");
        getWidget(mSearchTerm, "edit_SearchTerm");
        getWidget(mNextButton, "button_Next");
        getWidget(mPreviousButton, "button_Previous");

        // Set up the command line box
        mCommandLine->eventEditSelectAccept += newDelegate(this, &Console::acceptCommand);
        mCommandLine->eventKeyButtonPressed += newDelegate(this, &Console::commandBoxKeyPress);

        // Set up the search term box
        mSearchTerm->eventEditSelectAccept += newDelegate(this, &Console::acceptSearchTerm);
        mNextButton->eventMouseButtonClick += newDelegate(this, &Console::findNextOccurrence);
        mPreviousButton->eventMouseButtonClick += newDelegate(this, &Console::findPreviousOccurence);

        // Set up the log window
        mHistory->setOverflowToTheLeft(true);

        // compiler
        Compiler::registerExtensions(mExtensions, mConsoleOnlyScripts);
        mCompilerContext.setExtensions(&mExtensions);

        // command history file
        initConsoleHistory();
    }

    Console::~Console()
    {
        if (mCommandHistoryFile && mCommandHistoryFile.is_open())
            mCommandHistoryFile.close();
    }

    void Console::onOpen()
    {
        // Give keyboard focus to the combo box whenever the console is
        // turned on and place it over other widgets
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCommandLine);
        MyGUI::LayerManager::getInstance().upLayerItem(mMainWidget);
    }

    void Console::print(const std::string& msg, std::string_view color)
    {
        mHistory->addText(std::string(color) + MyGUI::TextIterator::toTagsString(msg));
    }

    void Console::printOK(const std::string& msg)
    {
        print(msg + "\n", MWBase::WindowManager::sConsoleColor_Success);
    }

    void Console::printError(const std::string& msg)
    {
        print(msg + "\n", MWBase::WindowManager::sConsoleColor_Error);
    }

    void Console::execute(const std::string& command)
    {
        // Log the command
        if (mConsoleMode.empty())
            print("> " + command + "\n");
        else
            print(mConsoleMode + " " + command + "\n");

        if (!mConsoleMode.empty() || (command.size() >= 3 && std::string_view(command).substr(0, 3) == "lua"))
        {
            MWBase::Environment::get().getLuaManager()->handleConsoleCommand(mConsoleMode, command, mPtr);
            return;
        }

        Compiler::Locals locals;
        if (!mPtr.isEmpty())
        {
            const ESM::RefId& script = mPtr.getClass().getScript(mPtr);
            if (!script.empty())
                locals = MWBase::Environment::get().getScriptManager()->getLocals(script);
        }
        Compiler::Output output(locals);

        if (compile(command + "\n", output))
        {
            try
            {
                ConsoleInterpreterContext interpreterContext(*this, mPtr);
                Interpreter::Interpreter interpreter;
                MWScript::installOpcodes(interpreter, mConsoleOnlyScripts);
                const Interpreter::Program program = output.getProgram();
                interpreter.run(program, interpreterContext);
            }
            catch (const std::exception& error)
            {
                printError(std::string("Error: ") + error.what());
            }
        }
    }

    void Console::executeFile(const std::string& path)
    {
        std::ifstream stream((std::filesystem::path(path)));

        if (!stream.is_open())
            printError("failed to open file: " + path);
        else
        {
            std::string line;

            while (std::getline(stream, line))
                execute(line);
        }
    }

    void Console::clear()
    {
        resetReference();
    }

    bool isWhitespace(char c)
    {
        return c == ' ' || c == '\t';
    }

    void Console::commandBoxKeyPress(MyGUI::Widget* _sender, MyGUI::KeyCode key, MyGUI::Char _char)
    {
        if (MyGUI::InputManager::getInstance().isControlPressed())
        {
            if (key == MyGUI::KeyCode::W)
            {
                const auto& caption = mCommandLine->getCaption();
                if (caption.empty())
                    return;
                size_t max = mCommandLine->getTextCursor();
                while (max > 0 && (isWhitespace(caption[max - 1]) || caption[max - 1] == '>'))
                    max--;
                while (max > 0 && !isWhitespace(caption[max - 1]) && caption[max - 1] != '>')
                    max--;
                size_t length = mCommandLine->getTextCursor() - max;
                if (length > 0)
                {
                    auto text = caption;
                    text.erase(max, length);
                    mCommandLine->setCaption(text);
                    mCommandLine->setTextCursor(max);
                }
            }
            else if (key == MyGUI::KeyCode::U)
            {
                if (mCommandLine->getTextCursor() > 0)
                {
                    auto text = mCommandLine->getCaption();
                    text.erase(0, mCommandLine->getTextCursor());
                    mCommandLine->setCaption(text);
                    mCommandLine->setTextCursor(0);
                }
            }
        }
        else if (key == MyGUI::KeyCode::Tab && mConsoleMode.empty())
        {
            std::vector<std::string> matches;
            listNames();
            std::string oldCaption = mCommandLine->getCaption();
            std::string newCaption = complete(mCommandLine->getOnlyText(), matches);
            mCommandLine->setCaption(newCaption);

            // List candidates if repeatedly pressing tab
            if (oldCaption == newCaption && !matches.empty())
            {
                int i = 0;
                printOK("");
                for (std::string& match : matches)
                {
                    if (i == 50)
                        break;

                    printOK(match);
                    i++;
                }
            }
        }

        if (mCommandHistory.empty())
            return;

        // Traverse history with up and down arrows
        if (key == MyGUI::KeyCode::ArrowUp)
        {
            // If the user was editing a string, store it for later
            if (mCurrent == mCommandHistory.end())
                mEditString = mCommandLine->getOnlyText();

            if (mCurrent != mCommandHistory.begin())
            {
                --mCurrent;
                mCommandLine->setCaption(*mCurrent);
            }
        }
        else if (key == MyGUI::KeyCode::ArrowDown)
        {
            if (mCurrent != mCommandHistory.end())
            {
                ++mCurrent;

                if (mCurrent != mCommandHistory.end())
                    mCommandLine->setCaption(*mCurrent);
                else
                    // Restore the edit string
                    mCommandLine->setCaption(mEditString);
            }
        }
    }

    void Console::acceptCommand(MyGUI::EditBox* _sender)
    {
        const std::string& cm = mCommandLine->getOnlyText();
        if (cm.empty())
            return;

        // Add the command to the history, and set the current pointer to
        // the end of the list
        if (mCommandHistory.empty() || mCommandHistory.back() != cm)
        {
            mCommandHistory.push_back(cm);

            if (mCommandHistoryFile && mCommandHistoryFile.good())
                mCommandHistoryFile << cm << std::endl;
        }
        mCurrent = mCommandHistory.end();
        mEditString.clear();
        mHistory->setTextCursor(mHistory->getTextLength());

        // Reset the command line before the command execution.
        // It prevents the re-triggering of the acceptCommand() event for the same command
        // during the actual command execution
        mCommandLine->setCaption("");

        execute(cm);
    }

    void Console::acceptSearchTerm(MyGUI::EditBox* _sender)
    {
        const std::string& searchTerm = mSearchTerm->getOnlyText();

        if (searchTerm.empty())
        {
            return;
        }

        mCurrentSearchTerm = Utf8Stream::lowerCaseUtf8(searchTerm);
        mCurrentOccurrence = std::string::npos;

        findNextOccurrence(nullptr);
    }

    void Console::findNextOccurrence(MyGUI::Widget* _sender)
    {
        if (mCurrentSearchTerm.empty())
        {
            return;
        }

        const auto historyText = Utf8Stream::lowerCaseUtf8(mHistory->getOnlyText().asUTF8());

        // Search starts at the beginning
        size_t startIndex = 0;

        // If this is not the first search, we start right AFTER the last occurrence.
        if (mCurrentOccurrence != std::string::npos && historyText.length() - mCurrentOccurrence > 1)
        {
            startIndex = mCurrentOccurrence + 1;
        }

        mCurrentOccurrence = historyText.find(mCurrentSearchTerm, startIndex);

        // If the last search did not find anything AND we didn't start at
        // the beginning, we repeat the search one time for wrapping around the text.
        if (mCurrentOccurrence == std::string::npos && startIndex != 0)
        {
            mCurrentOccurrence = historyText.find(mCurrentSearchTerm);
        }

        // Only scroll & select if we actually found something
        if (mCurrentOccurrence != std::string::npos)
        {
            markOccurrence(mCurrentOccurrence, mCurrentSearchTerm.length());
        }
        else
        {
            markOccurrence(0, 0);
        }
    }

    void Console::findPreviousOccurence(MyGUI::Widget* _sender)
    {
        if (mCurrentSearchTerm.empty())
        {
            return;
        }

        const auto historyText = Utf8Stream::lowerCaseUtf8(mHistory->getOnlyText().asUTF8());

        // Search starts at the end
        size_t startIndex = historyText.length();

        // If this is not the first search, we start right BEFORE the last occurrence.
        if (mCurrentOccurrence != std::string::npos && mCurrentOccurrence > 1)
        {
            startIndex = mCurrentOccurrence - 1;
        }

        mCurrentOccurrence = historyText.rfind(mCurrentSearchTerm, startIndex);

        // If the last search did not find anything AND we didn't start at
        // the end, we repeat the search one time for wrapping around the text.
        if (mCurrentOccurrence == std::string::npos && startIndex != historyText.length())
        {
            mCurrentOccurrence = historyText.rfind(mCurrentSearchTerm, historyText.length());
        }

        // Only scroll & select if we actually found something
        if (mCurrentOccurrence != std::string::npos)
        {
            markOccurrence(mCurrentOccurrence, mCurrentSearchTerm.length());
        }
        else
        {
            markOccurrence(0, 0);
        }
    }

    void Console::markOccurrence(const size_t textPosition, const size_t length)
    {
        if (textPosition == 0 && length == 0)
        {
            mHistory->setTextSelection(0, 0);
            mHistory->setVScrollPosition(mHistory->getVScrollRange());
            return;
        }

        const auto historyText = mHistory->getOnlyText();
        const size_t upperLimit = std::min(historyText.length(), textPosition);

        // Since MyGUI::EditBox.setVScrollPosition() works on pixels instead of text positions
        // we need to calculate the actual pixel position by counting lines.
        size_t lineNumber = 0;
        for (size_t i = 0; i < upperLimit; i++)
        {
            if (historyText[i] == '\n')
            {
                lineNumber++;
            }
        }

        // Make some space before the actual result
        if (lineNumber >= 2)
        {
            lineNumber -= 2;
        }

        mHistory->setTextSelection(textPosition, textPosition + length);
        mHistory->setVScrollPosition(mHistory->getFontHeight() * lineNumber);
    }

    std::string Console::complete(std::string input, std::vector<std::string>& matches)
    {
        std::string output = input;
        std::string tmp = input;
        bool has_front_quote = false;

        /* Does the input string contain things that don't have to be completed? If yes erase them. */

        /* Erase a possible call to an explicit reference. */
        size_t explicitPos = tmp.find("->");
        if (explicitPos != std::string::npos)
        {
            tmp.erase(0, explicitPos + 2);
        }

        /* Are there quotation marks? */
        if (tmp.find('"') != std::string::npos)
        {
            int numquotes = 0;
            for (std::string::iterator it = tmp.begin(); it < tmp.end(); ++it)
            {
                if (*it == '"')
                    numquotes++;
            }

            /* Is it terminated?*/
            if (numquotes % 2)
            {
                tmp.erase(0, tmp.rfind('"') + 1);
                has_front_quote = true;
            }
            else
            {
                size_t pos;
                if ((((pos = tmp.rfind(' ')) != std::string::npos)) && (pos > tmp.rfind('"')))
                {
                    tmp.erase(0, tmp.rfind(' ') + 1);
                }
                else
                {
                    tmp.clear();
                }
                has_front_quote = false;
            }
        }
        /* No quotation marks. Are there spaces?*/
        else
        {
            size_t rpos;
            if ((rpos = tmp.rfind(' ')) != std::string::npos)
            {
                if (rpos == 0)
                {
                    tmp.clear();
                }
                else
                {
                    tmp.erase(0, rpos + 1);
                }
            }
        }

        /* Erase the input from the output string so we can easily append the completed form later. */
        output.erase(output.end() - tmp.length(), output.end());

        /* Is there still something in the input string? If not just display all commands and return the unchanged
         * input. */
        if (tmp.length() == 0)
        {
            matches = mNames;
            return input;
        }

        /* Iterate through the vector. */
        for (std::string& name : mNames)
        {
            bool string_different = false;

            /* Is the string shorter than the input string? If yes skip it. */
            if (name.length() < tmp.length())
                continue;

            /* Is the beginning of the string different from the input string? If yes skip it. */
            for (std::string::iterator iter = tmp.begin(), iter2 = name.begin(); iter < tmp.end(); ++iter, ++iter2)
            {
                if (Misc::StringUtils::toLower(*iter) != Misc::StringUtils::toLower(*iter2))
                {
                    string_different = true;
                    break;
                }
            }

            if (string_different)
                continue;

            /* The beginning of the string matches the input string, save it for the next test. */
            matches.push_back(name);
        }

        /* There are no matches. Return the unchanged input. */
        if (matches.empty())
        {
            return input;
        }

        /* Only one match. We're done. */
        if (matches.size() == 1)
        {
            /* Adding quotation marks when the input string started with a quotation mark or has spaces in it*/
            if ((matches.front().find(' ') != std::string::npos))
            {
                if (!has_front_quote)
                    output.append(std::string("\""));
                return output.append(matches.front() + std::string("\" "));
            }
            else if (has_front_quote)
            {
                return output.append(matches.front() + std::string("\" "));
            }
            else
            {
                return output.append(matches.front() + std::string(" "));
            }
        }

        /* Check if all matching strings match further than input. If yes complete to this match. */
        int i = tmp.length();

        for (std::string::iterator iter = matches.front().begin() + tmp.length(); iter < matches.front().end();
             ++iter, ++i)
        {
            for (std::string& match : matches)
            {
                if (Misc::StringUtils::toLower(match[i]) != Misc::StringUtils::toLower(*iter))
                {
                    /* Append the longest match to the end of the output string*/
                    output.append(matches.front().substr(0, i));
                    return output;
                }
            }
        }

        /* All keywords match with the shortest. Append it to the output string and return it. */
        return output.append(matches.front());
    }

    void Console::onResChange(int width, int height)
    {
        setCoord(10, 10, width - 10, height / 2);
    }

    void Console::updateSelectedObjectPtr(const MWWorld::Ptr& currentPtr, const MWWorld::Ptr& newPtr)
    {
        if (mPtr == currentPtr)
            mPtr = newPtr;
    }

    void Console::setSelectedObject(const MWWorld::Ptr& object)
    {
        if (!object.isEmpty())
        {
            if (object == mPtr)
                mPtr = MWWorld::Ptr();
            else
                mPtr = object;
            // User clicked on an object. Restore focus to the console command line.
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCommandLine);
        }
        else
            mPtr = MWWorld::Ptr();
        updateConsoleTitle();
    }

    void Console::updateConsoleTitle()
    {
        std::string title = "#{sConsoleTitle}";
        if (!mConsoleMode.empty())
            title = mConsoleMode + " " + title;
        if (!mPtr.isEmpty())
            title.append(" (" + mPtr.getCellRef().getRefId().getRefIdString() + ")");
        setTitle(title);
    }

    void Console::setConsoleMode(std::string_view mode)
    {
        mConsoleMode = std::string(mode);
        updateConsoleTitle();
    }

    void Console::onReferenceUnavailable()
    {
        setSelectedObject(MWWorld::Ptr());
    }

    void Console::resetReference()
    {
        ReferenceInterface::resetReference();
        setSelectedObject(MWWorld::Ptr());
    }

    void Console::initConsoleHistory()
    {
        const auto filePath = mCfgMgr.getUserConfigPath() / "console_history.txt";
        const size_t retrievalLimit = Settings::Manager::getSize("console history buffer size", "General");

        std::ifstream historyFile(filePath);
        std::string line;

        // Read the previous session's commands from the file
        while (std::getline(historyFile, line))
        {
            // Truncate the list if it exceeds the retrieval limit
            if (mCommandHistory.size() >= retrievalLimit)
                mCommandHistory.pop_front();
            mCommandHistory.push_back(line);
        }

        historyFile.close();

        mCurrent = mCommandHistory.end();
        try
        {
            mCommandHistoryFile.exceptions(std::fstream::failbit | std::fstream::badbit);
            mCommandHistoryFile.open(filePath, std::ios_base::trunc);

            //  Update the history file
            for (const auto& histObj : mCommandHistory)
                mCommandHistoryFile << histObj << std::endl;
            mCommandHistoryFile.close();

            mCommandHistoryFile.open(filePath, std::ios_base::app);
        }
        catch (const std::ios_base::failure& e)
        {
            Log(Debug::Error) << "Error: Failed to write to console history file " << filePath << " : " << e.what()
                              << " : " << std::generic_category().message(errno);
        }
    }
}
