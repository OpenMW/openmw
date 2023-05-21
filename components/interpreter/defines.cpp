#include "defines.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>

namespace
{

    bool check(std::string_view str, std::string_view escword, size_t& i, size_t& start)
    {
        bool found = Misc::StringUtils::ciStartsWith(str, escword);
        if (found)
        {
            i += escword.length();
            start = i + 1;
        }
        return found;
    }

    std::vector<std::string> globals;
    const std::initializer_list<std::tuple<std::string_view, std::string_view>> sActionBindings{
        { "actionslideright", "#{sRight}" },
        { "actionreadymagic", "#{sReady_Magic}" },
        { "actionprevweapon", "#{sPrevWeapon}" },
        { "actionnextweapon", "#{sNextWeapon}" },
        { "actiontogglerun", "#{sAuto_Run}" },
        { "actionslideleft", "#{sLeft}" },
        { "actionreadyitem", "#{sReady_Weapon}" },
        { "actionprevspell", "#{sPrevSpell}" },
        { "actionnextspell", "#{sNextSpell}" },
        { "actionrestmenu", "#{sRestKey}" },
        { "actionmenumode", "#{sInventory}" },
        { "actionactivate", "#{sActivate}" },
        { "actionjournal", "#{sJournal}" },
        { "actionforward", "#{sForward}" },
        { "actioncrouch", "#{sCrouch_Sneak}" },
        { "actionjump", "#{sJump}" },
        { "actionback", "#{sBack}" },
        { "actionuse", "#{sUse}" },
        { "actionrun", "#{sRun}" },
    };
    using ContextMethod = std::string_view (Interpreter::Context::*)() const;
    const std::initializer_list<std::tuple<std::string_view, std::pair<ContextMethod, ContextMethod>>> sContextMethods{
        { "nextpcrank", { &Interpreter::Context::getPCNextRank, nullptr } },
        { "pcnextrank", { &Interpreter::Context::getPCNextRank, nullptr } },
        { "faction", { &Interpreter::Context::getNPCFaction, nullptr } },
        { "pcclass", { &Interpreter::Context::getPCClass, &Interpreter::Context::getPCClass } },
        { "pcname", { &Interpreter::Context::getPCName, &Interpreter::Context::getPCName } },
        { "pcrace", { &Interpreter::Context::getPCRace, &Interpreter::Context::getPCRace } },
        { "pcrank", { &Interpreter::Context::getPCRank, nullptr } },
        { "class", { &Interpreter::Context::getNPCClass, &Interpreter::Context::getPCClass } },
        { "cell", { &Interpreter::Context::getCurrentCellName, &Interpreter::Context::getCurrentCellName } },
        { "race", { &Interpreter::Context::getNPCRace, &Interpreter::Context::getPCRace } },
        { "rank", { &Interpreter::Context::getNPCRank, nullptr } },
        { "name", { &Interpreter::Context::getActorName, &Interpreter::Context::getPCName } },
    };

    bool longerStr(std::string_view a, std::string_view b)
    {
        return a.length() > b.length();
    }

    bool findReplacement(std::string_view temp, size_t& i, size_t& start, Interpreter::Context& context,
        std::ostringstream& retval, bool dialogue)
    {
        for (const auto& [name, binding] : sActionBindings)
        {
            if (check(temp, name, i, start))
            {
                retval << context.getActionBinding(binding);
                return true;
            }
        }
        if (check(temp, "pccrimelevel", i, start))
        {
            retval << context.getPCBounty();
            return true;
        }
        for (const auto& [name, methods] : sContextMethods)
        {
            const auto& method = dialogue ? methods.first : methods.second;
            if (check(temp, name, i, start))
            {
                if (method) // Not all variables are available outside of dialogue
                    retval << (context.*method)();
                return true;
            }
        }
        return false;
    }

    std::string fixDefinesReal(std::string_view text, bool dialogue, Interpreter::Context& context)
    {
        size_t start = 0;
        std::ostringstream retval;
        for (size_t i = 0; i < text.length(); ++i)
        {
            char eschar = text[i];
            if (eschar == '%' || eschar == '^')
            {
                retval << text.substr(start, i - start);
                std::string_view temp = text.substr(i + 1);

                bool found = false;
                try
                {
                    found = findReplacement(temp, i, start, context, retval, dialogue);
                    /* Not a builtin, try global variables */
                    if (!found)
                    {
                        /* if list of globals is empty, grab it and sort it by descending string length */
                        if (globals.empty())
                        {
                            globals = context.getGlobals();
                            sort(globals.begin(), globals.end(), longerStr);
                        }

                        for (const std::string& global : globals)
                        {
                            found = check(temp, global, i, start);
                            if (found)
                            {
                                char type = context.getGlobalType(global);

                                switch (type)
                                {
                                    case 's':
                                        retval << context.getGlobalShort(global);
                                        break;
                                    case 'l':
                                        retval << context.getGlobalLong(global);
                                        break;
                                    case 'f':
                                        retval << context.getGlobalFloat(global);
                                        break;
                                }
                                break;
                            }
                        }
                    }
                }
                catch (std::exception& e)
                {
                    Log(Debug::Error) << "Error: Failed to replace escape character, with the following error: "
                                      << e.what();
                    Log(Debug::Error) << "Full text below:\n" << text;
                }

                // Not found, or error
                if (!found)
                {
                    /* leave unmodified */
                    i += 1;
                    start = i;
                    retval << eschar;
                }
            }
        }
        retval << text.substr(start, text.length() - start);
        return retval.str();
    }

}

namespace Interpreter
{
    std::string fixDefinesDialog(std::string_view text, Context& context)
    {
        return fixDefinesReal(text, true, context);
    }

    std::string fixDefinesMsgBox(std::string_view text, Context& context)
    {
        return fixDefinesReal(text, false, context);
    }

    std::string fixDefinesBook(std::string_view text, Context& context)
    {
        return fixDefinesReal(text, false, context);
    }
}
