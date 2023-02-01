#include "defines.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>

namespace Interpreter
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

    bool longerStr(std::string_view a, std::string_view b)
    {
        return a.length() > b.length();
    }

    static std::string fixDefinesReal(std::string_view text, bool dialogue, Context& context)
    {
        size_t start = 0;
        std::ostringstream retval;
        for (size_t i = 0; i < text.length(); ++i)
        {
            char eschar = text[i];
            if (eschar == '%' || eschar == '^')
            {
                retval << text.substr(start, i - start);
                std::string_view temp = text.substr(i + 1, 100);

                bool found = false;
                try
                {
                    if ((found = check(temp, "actionslideright", i, start)))
                    {
                        retval << context.getActionBinding("#{sRight}");
                    }
                    else if ((found = check(temp, "actionreadymagic", i, start)))
                    {
                        retval << context.getActionBinding("#{sReady_Magic}");
                    }
                    else if ((found = check(temp, "actionprevweapon", i, start)))
                    {
                        retval << context.getActionBinding("#{sPrevWeapon}");
                    }
                    else if ((found = check(temp, "actionnextweapon", i, start)))
                    {
                        retval << context.getActionBinding("#{sNextWeapon}");
                    }
                    else if ((found = check(temp, "actiontogglerun", i, start)))
                    {
                        retval << context.getActionBinding("#{sAuto_Run}");
                    }
                    else if ((found = check(temp, "actionslideleft", i, start)))
                    {
                        retval << context.getActionBinding("#{sLeft}");
                    }
                    else if ((found = check(temp, "actionreadyitem", i, start)))
                    {
                        retval << context.getActionBinding("#{sReady_Weapon}");
                    }
                    else if ((found = check(temp, "actionprevspell", i, start)))
                    {
                        retval << context.getActionBinding("#{sPrevSpell}");
                    }
                    else if ((found = check(temp, "actionnextspell", i, start)))
                    {
                        retval << context.getActionBinding("#{sNextSpell}");
                    }
                    else if ((found = check(temp, "actionrestmenu", i, start)))
                    {
                        retval << context.getActionBinding("#{sRestKey}");
                    }
                    else if ((found = check(temp, "actionmenumode", i, start)))
                    {
                        retval << context.getActionBinding("#{sInventory}");
                    }
                    else if ((found = check(temp, "actionactivate", i, start)))
                    {
                        retval << context.getActionBinding("#{sActivate}");
                    }
                    else if ((found = check(temp, "actionjournal", i, start)))
                    {
                        retval << context.getActionBinding("#{sJournal}");
                    }
                    else if ((found = check(temp, "actionforward", i, start)))
                    {
                        retval << context.getActionBinding("#{sForward}");
                    }
                    else if ((found = check(temp, "pccrimelevel", i, start)))
                    {
                        retval << context.getPCBounty();
                    }
                    else if ((found = check(temp, "actioncrouch", i, start)))
                    {
                        retval << context.getActionBinding("#{sCrouch_Sneak}");
                    }
                    else if ((found = check(temp, "actionjump", i, start)))
                    {
                        retval << context.getActionBinding("#{sJump}");
                    }
                    else if ((found = check(temp, "actionback", i, start)))
                    {
                        retval << context.getActionBinding("#{sBack}");
                    }
                    else if ((found = check(temp, "actionuse", i, start)))
                    {
                        retval << context.getActionBinding("#{sUse}");
                    }
                    else if ((found = check(temp, "actionrun", i, start)))
                    {
                        retval << context.getActionBinding("#{sRun}");
                    }
                    else if ((found = check(temp, "pcclass", i, start)))
                    {
                        retval << context.getPCClass();
                    }
                    else if ((found = check(temp, "pcrace", i, start)))
                    {
                        retval << context.getPCRace();
                    }
                    else if ((found = check(temp, "pcname", i, start)))
                    {
                        retval << context.getPCName();
                    }
                    else if ((found = check(temp, "cell", i, start)))
                    {
                        retval << context.getCurrentCellName();
                    }

                    else if (dialogue)
                    { // In Dialogue, not messagebox
                        if ((found = check(temp, "faction", i, start)))
                        {
                            retval << context.getNPCFaction();
                        }
                        else if ((found = check(temp, "nextpcrank", i, start)))
                        {
                            retval << context.getPCNextRank();
                        }
                        else if ((found = check(temp, "pcnextrank", i, start)))
                        {
                            retval << context.getPCNextRank();
                        }
                        else if ((found = check(temp, "pcrank", i, start)))
                        {
                            retval << context.getPCRank();
                        }
                        else if ((found = check(temp, "rank", i, start)))
                        {
                            retval << context.getNPCRank();
                        }

                        else if ((found = check(temp, "class", i, start)))
                        {
                            retval << context.getNPCClass();
                        }
                        else if ((found = check(temp, "race", i, start)))
                        {
                            retval << context.getNPCRace();
                        }
                        else if ((found = check(temp, "name", i, start)))
                        {
                            retval << context.getActorName();
                        }
                    }
                    else
                    { // In messagebox or book, not dialogue

                        /* empty outside dialogue */
                        if ((found = check(temp, "faction", i, start)))
                            ;
                        else if ((found = check(temp, "nextpcrank", i, start)))
                            ;
                        else if ((found = check(temp, "pcnextrank", i, start)))
                            ;
                        else if ((found = check(temp, "pcrank", i, start)))
                            ;
                        else if ((found = check(temp, "rank", i, start)))
                            ;

                        /* uses pc in messageboxes */
                        else if ((found = check(temp, "class", i, start)))
                        {
                            retval << context.getPCClass();
                        }
                        else if ((found = check(temp, "race", i, start)))
                        {
                            retval << context.getPCRace();
                        }
                        else if ((found = check(temp, "name", i, start)))
                        {
                            retval << context.getPCName();
                        }
                    }

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
                            if (global.length() > temp.length()) // Just in case there's a global with a huuuge name
                                temp = text.substr(i + 1, global.length());

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

    std::string fixDefinesDialog(const std::string& text, Context& context)
    {
        return fixDefinesReal(text, true, context);
    }

    std::string fixDefinesMsgBox(const std::string& text, Context& context)
    {
        return fixDefinesReal(text, false, context);
    }

    std::string fixDefinesBook(const std::string& text, Context& context)
    {
        return fixDefinesReal(text, false, context);
    }
}
