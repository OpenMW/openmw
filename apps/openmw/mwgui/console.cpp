#include "console.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_LayerManager.h>

#include <filesystem>
#include <fstream>
#include <regex>

#include <components/compiler/exception.hpp>
#include <components/compiler/extensions0.hpp>
#include <components/compiler/lineparser.hpp>
#include <components/compiler/locals.hpp>
#include <components/compiler/scanner.hpp>
#include <components/files/conversion.hpp>
#include <components/interpreter/interpreter.hpp>
#include <components/misc/utf8stream.hpp>
#include <components/settings/values.hpp>

#include "apps/openmw/mwgui/textcolours.hpp"

#include "../mwscript/extensions.hpp"
#include "../mwscript/interpretercontext.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

namespace
{
    bool isWhitespace(MyGUI::UString::code_point c)
    {
        return c == ' ' || c == '\t';
    }
}

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
            std::istringstream input;

            Compiler::Scanner scanner(*this, input, mCompilerContext.getExtensions());

            scanner.listKeywords(mNames);

            // identifier
            const MWWorld::ESMStore& esmStore = *MWBase::Environment::get().getESMStore();
            std::vector<ESM::RefId> ids;
            for (const auto* store : esmStore)
            {
                store->listIdentifier(ids);
                for (auto id : ids)
                {
                    if (id.is<ESM::StringRefId>())
                        mNames.push_back(id.getRefIdString());
                }
                ids.clear();
            }

            // exterior cell names and editor IDs aren't technically identifiers,
            // but since the COC function accepts them, we should list them too
            for (auto it = esmStore.get<ESM::Cell>().extBegin(); it != esmStore.get<ESM::Cell>().extEnd(); ++it)
            {
                if (!it->mName.empty())
                    mNames.push_back(it->mName);
            }

            for (const auto& cell : esmStore.get<ESM4::Cell>())
            {
                if (!cell.mEditorId.empty())
                    mNames.push_back(cell.mEditorId);
            }

            // sort
            std::sort(mNames.begin(), mNames.end());

            // remove duplicates
            mNames.erase(std::unique(mNames.begin(), mNames.end()), mNames.end());
        }
    }

    Console::Console(int w, int h, bool consoleOnlyScripts, Files::ConfigurationManager& cfgMgr)
        : WindowBase("openmw_console.layout")
        , mCaseSensitiveSearch(false)
        , mRegExSearch(false)
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
        getWidget(mCaseSensitiveToggleButton, "button_CaseSensitive");
        getWidget(mRegExSearchToggleButton, "button_RegExSearch");

        // Set up the command line box
        mCommandLine->eventEditSelectAccept += newDelegate(this, &Console::acceptCommand);
        mCommandLine->eventKeyButtonPressed += newDelegate(this, &Console::commandBoxKeyPress);

        // Set up the search term box
        mSearchTerm->eventEditSelectAccept += newDelegate(this, &Console::acceptSearchTerm);
        mNextButton->eventMouseButtonClick += newDelegate(this, &Console::findNextOccurrence);
        mPreviousButton->eventMouseButtonClick += newDelegate(this, &Console::findPreviousOccurrence);
        mCaseSensitiveToggleButton->eventMouseButtonClick += newDelegate(this, &Console::toggleCaseSensitiveSearch);
        mRegExSearchToggleButton->eventMouseButtonClick += newDelegate(this, &Console::toggleRegExSearch);

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

    void Console::executeFile(const std::filesystem::path& path)
    {
        std::ifstream stream(path);

        if (!stream.is_open())
        {
            printError("Failed to open script file \"" + Files::pathToUnicodeString(path)
                + "\": " + std::generic_category().message(errno));
            return;
        }

        std::string line;
        while (std::getline(stream, line))
            execute(line);
    }

    void Console::clear()
    {
        resetReference();
    }

    void Console::commandBoxKeyPress(MyGUI::Widget* /*sender*/, MyGUI::KeyCode key, MyGUI::Char /*value*/)
    {
        if (MyGUI::InputManager::getInstance().isControlPressed())
        {
            if (key == MyGUI::KeyCode::W)
            {
                auto caption = mCommandLine->getOnlyText();
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
                    caption.erase(max, length);
                    mCommandLine->setOnlyText(caption);
                    mCommandLine->setTextCursor(max);
                }
            }
            else if (key == MyGUI::KeyCode::U)
            {
                if (mCommandLine->getTextCursor() > 0)
                {
                    auto text = mCommandLine->getOnlyText();
                    text.erase(0, mCommandLine->getTextCursor());
                    mCommandLine->setOnlyText(text);
                    mCommandLine->setTextCursor(0);
                }
            }
        }
        else if (key == MyGUI::KeyCode::Tab && mConsoleMode.empty())
        {
            std::vector<std::string> matches;
            listNames();
            std::string oldCaption = mCommandLine->getOnlyText();
            std::string newCaption = complete(mCommandLine->getOnlyText(), matches);
            mCommandLine->setOnlyText(newCaption);

            // List candidates if repeatedly pressing tab
            if (oldCaption == newCaption && !matches.empty())
            {
                int i = 0;
                printOK({});
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
                mCommandLine->setOnlyText(*mCurrent);
            }
        }
        else if (key == MyGUI::KeyCode::ArrowDown)
        {
            if (mCurrent != mCommandHistory.end())
            {
                ++mCurrent;

                if (mCurrent != mCommandHistory.end())
                    mCommandLine->setOnlyText(*mCurrent);
                else
                    // Restore the edit string
                    mCommandLine->setOnlyText(mEditString);
            }
        }
    }

    void Console::acceptCommand(MyGUI::EditBox* /*sender*/)
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
        mCommandLine->setCaption({});

        execute(cm);
    }

    void Console::toggleCaseSensitiveSearch(MyGUI::Widget* /*sender*/)
    {
        mCaseSensitiveSearch = !mCaseSensitiveSearch;

        // Reset console search highlight position search parameters have changed
        mCurrentOccurrenceIndex = std::string::npos;

        // Adjust color to reflect toggled status
        const TextColours& textColours{ MWBase::Environment::get().getWindowManager()->getTextColours() };
        mCaseSensitiveToggleButton->setTextColour(mCaseSensitiveSearch ? textColours.link : textColours.normal);
    }

    void Console::toggleRegExSearch(MyGUI::Widget* /*sender*/)
    {
        mRegExSearch = !mRegExSearch;

        // Reset console search highlight position search parameters have changed
        mCurrentOccurrenceIndex = std::string::npos;

        // Adjust color to reflect toggled status
        const TextColours& textColours{ MWBase::Environment::get().getWindowManager()->getTextColours() };
        mRegExSearchToggleButton->setTextColour(mRegExSearch ? textColours.link : textColours.normal);

        // RegEx searches are always case sensitive
        mCaseSensitiveSearch = mRegExSearch;

        // Dim case sensitive and set disabled if regex search toggled on, restore when toggled off
        mCaseSensitiveToggleButton->setTextColour(mCaseSensitiveSearch ? textColours.linkPressed : textColours.normal);
        mCaseSensitiveToggleButton->setEnabled(!mRegExSearch);
    }

    void Console::acceptSearchTerm(MyGUI::EditBox* /*sender*/)
    {
        const std::string& searchTerm = mSearchTerm->getOnlyText();

        if (searchTerm.empty())
        {
            return;
        }

        std::string newSearchTerm = mCaseSensitiveSearch ? searchTerm : Utf8Stream::lowerCaseUtf8(searchTerm);

        // If new search term reset position, otherwise continue from current position
        if (newSearchTerm != mCurrentSearchTerm)
        {
            mCurrentSearchTerm = std::move(newSearchTerm);
            mCurrentOccurrenceIndex = std::string::npos;
        }

        findNextOccurrence(nullptr);
    }

    enum class Console::SearchDirection
    {
        Forward,
        Reverse
    };

    void Console::findNextOccurrence(MyGUI::Widget* /*sender*/)
    {
        findOccurrence(SearchDirection::Forward);
    }

    void Console::findPreviousOccurrence(MyGUI::Widget* /*sender*/)
    {
        findOccurrence(SearchDirection::Reverse);
    }

    void Console::findOccurrence(const SearchDirection direction)
    {
        if (mCurrentSearchTerm.empty())
        {
            return;
        }

        const auto historyText{ mCaseSensitiveSearch ? mHistory->getOnlyText().asUTF8()
                                                     : Utf8Stream::lowerCaseUtf8(mHistory->getOnlyText().asUTF8()) };

        // Setup default search range
        size_t firstIndex{ 0 };
        size_t lastIndex{ historyText.length() };

        // If this isn't the first search, adjust the range based on the previous occurrence.
        if (mCurrentOccurrenceIndex != std::string::npos)
        {
            if (direction == SearchDirection::Forward)
            {
                firstIndex = mCurrentOccurrenceIndex + mCurrentOccurrenceLength;
            }
            else if (direction == SearchDirection::Reverse)
            {
                lastIndex = mCurrentOccurrenceIndex;
            }
        }

        findInHistoryText(historyText, direction, firstIndex, lastIndex);

        // If the last search did not find anything AND...
        if (mCurrentOccurrenceIndex == std::string::npos)
        {
            if (direction == SearchDirection::Forward && firstIndex != 0)
            {
                // ... We didn't start at the beginning, we apply the search to the other half of the text.
                findInHistoryText(historyText, direction, 0, firstIndex);
            }
            else if (direction == SearchDirection::Reverse && lastIndex != historyText.length())
            {
                // ... We didn't search to the end, we apply the search to the other half of the text.
                findInHistoryText(historyText, direction, lastIndex, historyText.length());
            }
        }

        // Only scroll & select if we actually found something
        if (mCurrentOccurrenceIndex != std::string::npos)
        {
            markOccurrence(mCurrentOccurrenceIndex, mCurrentOccurrenceLength);
        }
        else
        {
            markOccurrence(0, 0);
        }
    }

    void Console::findInHistoryText(const std::string& historyText, const SearchDirection direction,
        const size_t firstIndex, const size_t lastIndex)
    {
        if (lastIndex <= firstIndex)
        {
            mCurrentOccurrenceIndex = std::string::npos;
            mCurrentOccurrenceLength = 0;
            return;
        }

        if (mRegExSearch)
        {
            findWithRegex(historyText, direction, firstIndex, lastIndex);
        }
        else
        {
            findWithStringSearch(historyText, direction, firstIndex, lastIndex);
        }
    }

    void Console::findWithRegex(const std::string& historyText, const SearchDirection direction,
        const size_t firstIndex, const size_t lastIndex)
    {
        // Search text for regex match in given interval
        const std::regex pattern{ mCurrentSearchTerm };
        std::sregex_iterator match{ (historyText.cbegin() + firstIndex), (historyText.cbegin() + lastIndex), pattern };
        const std::sregex_iterator end{};

        // If reverse search get last result in interval
        if (direction == SearchDirection::Reverse)
        {
            std::sregex_iterator lastMatch{ end };
            while (match != end)
            {
                lastMatch = match;
                ++match;
            }
            match = lastMatch;
        }

        // If regex match is found in text, set new current occurrence values
        if (match != end)
        {
            mCurrentOccurrenceIndex = match->position() + firstIndex;
            mCurrentOccurrenceLength = match->length();
        }
        else
        {
            mCurrentOccurrenceIndex = std::string::npos;
            mCurrentOccurrenceLength = 0;
        }
    }

    void Console::findWithStringSearch(const std::string& historyText, const SearchDirection direction,
        const size_t firstIndex, const size_t lastIndex)
    {
        // Search in given text interval for search term
        const size_t substringLength = lastIndex - firstIndex;
        const std::string_view historyTextView((historyText.c_str() + firstIndex), substringLength);
        if (direction == SearchDirection::Forward)
        {
            mCurrentOccurrenceIndex = historyTextView.find(mCurrentSearchTerm);
        }
        else
        {
            mCurrentOccurrenceIndex = historyTextView.rfind(mCurrentSearchTerm);
        }

        // If search term is found in text, set new current occurrence values
        if (mCurrentOccurrenceIndex != std::string::npos)
        {
            mCurrentOccurrenceIndex += firstIndex;
            mCurrentOccurrenceLength = mCurrentSearchTerm.length();
        }
        else
        {
            mCurrentOccurrenceLength = 0;
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
        bool hasFrontQuote = false;

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
                hasFrontQuote = true;
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
                hasFrontQuote = false;
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
        if (tmp.empty())
        {
            matches = mNames;
            return input;
        }

        /* Iterate through the vector. */
        for (std::string& name : mNames)
        {
            bool stringDifferent = false;

            /* Is the string shorter than the input string? If yes skip it. */
            if (name.length() < tmp.length())
                continue;

            /* Is the beginning of the string different from the input string? If yes skip it. */
            for (std::string::iterator iter = tmp.begin(), iter2 = name.begin(); iter < tmp.end(); ++iter, ++iter2)
            {
                if (Misc::StringUtils::toLower(*iter) != Misc::StringUtils::toLower(*iter2))
                {
                    stringDifferent = true;
                    break;
                }
            }

            if (stringDifferent)
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
                if (!hasFrontQuote)
                    output += '"';
                return output.append(matches.front() + std::string("\" "));
            }
            else if (hasFrontQuote)
            {
                return output.append(matches.front() + std::string("\" "));
            }
            else
            {
                return output.append(matches.front() + std::string(" "));
            }
        }

        /* Check if all matching strings match further than input. If yes complete to this match. */
        size_t i = tmp.length();

        for (auto iter = matches.front().begin() + tmp.length(); iter < matches.front().end(); ++iter, ++i)
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
        std::string title = "#{OMWEngine:ConsoleWindow}";
        if (!mConsoleMode.empty())
            title = mConsoleMode + " " + title;
        if (!mPtr.isEmpty())
            title.append(" (" + mPtr.getCellRef().getRefId().toDebugString() + ")");
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
        const size_t retrievalLimit = Settings::general().mConsoleHistoryBufferSize;

        // Read the previous session's commands from the file
        if (retrievalLimit > 0)
        {
            std::ifstream historyFile(filePath);
            std::string line;
            while (std::getline(historyFile, line))
            {
                // Truncate the list if it exceeds the retrieval limit
                if (mCommandHistory.size() >= retrievalLimit)
                    mCommandHistory.pop_front();
                mCommandHistory.push_back(line);
            }
            historyFile.close();
        }

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
