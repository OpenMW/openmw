#include "defines.hpp"

#include <sstream>
#include <string>
#include <vector>

#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>

namespace Interpreter{

    bool check(const std::string& str, const std::string& escword, unsigned int* i, unsigned int* start)
    {
        bool retval = str.find(escword) == 0;
        if(retval){
            (*i) += escword.length();
            (*start) = (*i) + 1;
        }
        return retval;
    }

    std::vector<std::string> globals;

    bool longerStr(const std::string& a, const std::string& b)
    {
        return a.length() > b.length();
    }

    std::string fixDefinesReal(std::string text, bool dialogue, Context& context)
    {
        unsigned int start = 0;
        std::ostringstream retval;
        for(unsigned int i = 0; i < text.length(); i++)
        {
            char eschar = text[i];
            if(eschar == '%' || eschar == '^')
            {
                retval << text.substr(start, i - start);
                std::string temp = Misc::StringUtils::lowerCase(text.substr(i+1, 100));
                
                bool found = false;
                try
                {
                    if(     (found = check(temp, "actionslideright", &i, &start))){
                        retval << context.getActionBinding("#{sRight}");
                    }
                    else if((found = check(temp, "actionreadymagic", &i, &start))){
                        retval << context.getActionBinding("#{sReady_Magic}");
                    }
                    else if((found = check(temp, "actionprevweapon", &i, &start))){
                        retval << context.getActionBinding("#{sPrevWeapon}");
                    }
                    else if((found = check(temp, "actionnextweapon", &i, &start))){
                        retval << context.getActionBinding("#{sNextWeapon}");
                    }
                    else if((found = check(temp, "actiontogglerun", &i, &start))){
                        retval << context.getActionBinding("#{sAuto_Run}");
                    }
                    else if((found = check(temp, "actionslideleft", &i, &start))){
                        retval << context.getActionBinding("#{sLeft}");
                    }
                    else if((found = check(temp, "actionreadyitem", &i, &start))){
                        retval << context.getActionBinding("#{sReady_Weapon}");
                    }
                    else if((found = check(temp, "actionprevspell", &i, &start))){
                        retval << context.getActionBinding("#{sPrevSpell}");
                    }
                    else if((found = check(temp, "actionnextspell", &i, &start))){
                        retval << context.getActionBinding("#{sNextSpell}");
                    }
                    else if((found = check(temp, "actionrestmenu", &i, &start))){
                        retval << context.getActionBinding("#{sRestKey}");
                    }
                    else if((found = check(temp, "actionmenumode", &i, &start))){
                        retval << context.getActionBinding("#{sInventory}");
                    }
                    else if((found = check(temp, "actionactivate", &i, &start))){
                        retval << context.getActionBinding("#{sActivate}");
                    }
                    else if((found = check(temp, "actionjournal", &i, &start))){
                        retval << context.getActionBinding("#{sJournal}");
                    }
                    else if((found = check(temp, "actionforward", &i, &start))){
                        retval << context.getActionBinding("#{sForward}");
                    }
                    else if((found = check(temp, "pccrimelevel", &i, &start))){
                        retval << context.getPCBounty();
                    }
                    else if((found = check(temp, "actioncrouch", &i, &start))){
                        retval << context.getActionBinding("#{sCrouch_Sneak}");
                    }
                    else if((found = check(temp, "actionjump", &i, &start))){
                        retval << context.getActionBinding("#{sJump}");
                    }
                    else if((found = check(temp, "actionback", &i, &start))){
                        retval << context.getActionBinding("#{sBack}");
                    }
                    else if((found = check(temp, "actionuse", &i, &start))){
                        retval << context.getActionBinding("#{sUse}");
                    }
                    else if((found = check(temp, "actionrun", &i, &start))){
                        retval << context.getActionBinding("#{sRun}");
                    }
                    else if((found = check(temp, "pcclass", &i, &start))){
                        retval << context.getPCClass();
                    }
                    else if((found = check(temp, "pcrace", &i, &start))){
                        retval << context.getPCRace();
                    }
                    else if((found = check(temp, "pcname", &i, &start))){
                        retval << context.getPCName();
                    }
                    else if((found = check(temp, "cell", &i, &start))){
                        retval << context.getCurrentCellName();
                    }

                    else if(dialogue) { // In Dialogue, not messagebox
                        if(     (found = check(temp, "faction", &i, &start))){
                            retval << context.getNPCFaction();
                        }
                        else if((found = check(temp, "nextpcrank", &i, &start))){
                            retval << context.getPCNextRank();
                        }
                        else if((found = check(temp, "pcnextrank", &i, &start))){
                            retval << context.getPCNextRank();
                        }
                        else if((found = check(temp, "pcrank", &i, &start))){
                            retval << context.getPCRank();
                        }
                        else if((found = check(temp, "rank", &i, &start))){
                            retval << context.getNPCRank();
                        }

                        else if((found = check(temp, "class", &i, &start))){
                            retval << context.getNPCClass();
                        }
                        else if((found = check(temp, "race", &i, &start))){
                            retval << context.getNPCRace();
                        }
                        else if((found = check(temp, "name", &i, &start))){
                            retval << context.getActorName();
                        }
                    }
                    else { // In messagebox or book, not dialogue

                        /* empty outside dialogue */
                        if(     (found = check(temp, "faction", &i, &start)));
                        else if((found = check(temp, "nextpcrank", &i, &start)));
                        else if((found = check(temp, "pcnextrank", &i, &start)));
                        else if((found = check(temp, "pcrank", &i, &start)));
                        else if((found = check(temp, "rank", &i, &start)));

                        /* uses pc in messageboxes */
                        else if((found = check(temp, "class", &i, &start))){
                            retval << context.getPCClass();
                        }
                        else if((found = check(temp, "race", &i, &start))){
                            retval << context.getPCRace();
                        }
                        else if((found = check(temp, "name", &i, &start))){
                            retval << context.getPCName();
                        }
                    }

                    /* Not a builtin, try global variables */
                    if(!found){
                        /* if list of globals is empty, grab it and sort it by descending string length */
                        if(globals.empty()){
                            globals = context.getGlobals();
                            sort(globals.begin(), globals.end(), longerStr);
                        }

                        for(unsigned int j = 0; j < globals.size(); j++){
                            if(globals[j].length() > temp.length()){ // Just in case there's a global with a huuuge name
                                temp = text.substr(i+1, globals[j].length());
                                transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
                            }

                            found = check(temp, globals[j], &i, &start);
                            if(found){
                                char type = context.getGlobalType(globals[j]);

                                switch(type){
                                    case 's': retval << context.getGlobalShort(globals[j]);  break;
                                    case 'l': retval << context.getGlobalLong(globals[j]); break;
                                    case 'f': retval << context.getGlobalFloat(globals[j]); break;
                                }
                                break;
                            }
                        }
                    }
                }
                catch (std::exception& e)
                {
                    Log(Debug::Error) << "Error: Failed to replace escape character, with the following error: " << e.what();
                    Log(Debug::Error) << "Full text below:\n" << text;
                }

                // Not found, or error
                if(!found){
                    /* leave unmodified */
                    i += 1;
                    start = i;
                    retval << eschar;
                }
            }
        }
        retval << text.substr(start, text.length() - start);
        return retval.str ();
    }

    std::string fixDefinesDialog(const std::string& text, Context& context){
        return fixDefinesReal(text, true, context);
    }

    std::string fixDefinesMsgBox(const std::string& text, Context& context){
        return fixDefinesReal(text, false, context);
    }

    std::string fixDefinesBook(const std::string& text, Context& context){
        return fixDefinesReal(text, false, context);
    }
}
