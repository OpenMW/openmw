#include "defines.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace Interpreter{

    bool Check(const std::string str, const std::string escword, unsigned int* i, unsigned int* start){
        bool retval = str.find(escword) == 0;
        if(retval){
            (*i) += escword.length();
            (*start) = (*i) + 1;
        }
        return retval;
    }

    std::vector<std::string> globals;

    bool longerStr(const std::string a, const std::string b){
        return a.length() > b.length();
    }

    std::string fixDefinesReal(std::string text, char eschar, bool isBook, Context& context){

        unsigned int start = 0;
        std::ostringstream retval;
        for(unsigned int i = 0; i < text.length(); i++){
            if(text[i] == eschar){
                retval << text.substr(start, i - start);
                std::string temp = text.substr(i+1, 100);
                transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
                
                bool found;
            
                if(     (found = Check(temp, "actionslideright", &i, &start))){
                    retval << context.getActionBinding("#{sRight}");
                }
                else if((found = Check(temp, "actionreadymagic", &i, &start))){
                    retval << context.getActionBinding("#{sReady_Magic}");
                }
                else if((found = Check(temp, "actionprevweapon", &i, &start))){
                    retval << "PLACEHOLDER_ACTION_PREV_WEAPON";
                }
                else if((found = Check(temp, "actionnextweapon", &i, &start))){
                    retval << "PLACEHOLDER_ACTION_PREV_WEAPON";
                }
                else if((found = Check(temp, "actiontogglerun", &i, &start))){
                    retval << context.getActionBinding("#{sAuto_Run}");
                }
                else if((found = Check(temp, "actionslideleft", &i, &start))){
                    retval << context.getActionBinding("#{sLeft}");
                }
                else if((found = Check(temp, "actionreadyitem", &i, &start))){
                    retval << context.getActionBinding("#{sReady_Weapon}");
                }
                else if((found = Check(temp, "actionprevspell", &i, &start))){
                    retval << "PLACEHOLDER_ACTION_PREV_SPELL";
                }
                else if((found = Check(temp, "actionnextspell", &i, &start))){
                    retval << "PLACEHOLDER_ACTION_NEXT_SPELL";
                }
                else if((found = Check(temp, "actionrestmenu", &i, &start))){
                    retval << context.getActionBinding("#{sRestKey}");
                }
                else if((found = Check(temp, "actionmenumode", &i, &start))){
                    retval << context.getActionBinding("#{sJournal}");
                }
                else if((found = Check(temp, "actionactivate", &i, &start))){
                    retval << context.getActionBinding("#{sActivate}");
                }
                else if((found = Check(temp, "actionjournal", &i, &start))){
                    retval << context.getActionBinding("#{sJournal}");
                }
                else if((found = Check(temp, "actionforward", &i, &start))){
                    retval << context.getActionBinding("#{sForward}");
                }
                else if((found = Check(temp, "pccrimelevel", &i, &start))){
                    retval << context.getPCBounty();
                }
                else if((found = Check(temp, "actioncrouch", &i, &start))){
                    retval << context.getActionBinding("#{sCrouch_Sneak}");
                }
                else if((found = Check(temp, "actionjump", &i, &start))){
                    retval << context.getActionBinding("#{sJump}");
                }
                else if((found = Check(temp, "actionback", &i, &start))){
                    retval << context.getActionBinding("#{sBack}");
                }
                else if((found = Check(temp, "actionuse", &i, &start))){
                    retval << "PLACEHOLDER_ACTION_USE";
                }
                else if((found = Check(temp, "actionrun", &i, &start))){
                    retval << "PLACEHOLDER_ACTION_RUN";
                }
                else if((found = Check(temp, "pcclass", &i, &start))){
                    retval << context.getPCClass();
                }
                else if((found = Check(temp, "pcrace", &i, &start))){
                    retval << context.getPCRace();
                }
                else if((found = Check(temp, "pcname", &i, &start))){
                    retval << context.getPCName();
                }
                else if((found = Check(temp, "cell", &i, &start))){
                    retval << context.getCurrentCellName();
                }

                else if(eschar == '%' && !isBook) { // In Dialogue, not messagebox
                    if(     (found = Check(temp, "faction", &i, &start))){
                        retval << context.getNPCFaction();
                    }
                    else if((found = Check(temp, "nextpcrank", &i, &start))){
                        retval << context.getPCNextRank();
                    }
                    else if((found = Check(temp, "pcnextrank", &i, &start))){
                        retval << context.getPCNextRank();
                    }
                    else if((found = Check(temp, "pcrank", &i, &start))){
                        retval << context.getPCRank();
                    }
                    else if((found = Check(temp, "rank", &i, &start))){
                        retval << context.getNPCRank();
                    }
                    
                    else if((found = Check(temp, "class", &i, &start))){
                        retval << context.getNPCClass();
                    }
                    else if((found = Check(temp, "race", &i, &start))){
                        retval << context.getNPCRace();
                    }
                    else if((found = Check(temp, "name", &i, &start))){
                        retval << context.getNPCName();
                    }
                }
                else { // In messagebox or book, not dialogue

                    /* empty outside dialogue */
                    if(     (found = Check(temp, "faction", &i, &start)));
                    else if((found = Check(temp, "nextpcrank", &i, &start)));
                    else if((found = Check(temp, "pcnextrank", &i, &start)));
                    else if((found = Check(temp, "pcrank", &i, &start)));
                    else if((found = Check(temp, "rank", &i, &start)));
                    
                    /* uses pc in messageboxes */
                    else if((found = Check(temp, "class", &i, &start))){
                        retval << context.getPCClass();
                    }
                    else if((found = Check(temp, "race", &i, &start))){
                        retval << context.getPCRace();
                    }
                    else if((found = Check(temp, "name", &i, &start))){
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
                            std::string temp = text.substr(i+1, globals[j].length());
                            transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
                        }

                        if((found = Check(temp, globals[j], &i, &start))){
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
                
                /* Not found */
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

    std::string fixDefinesDialog(std::string text, Context& context){
        return fixDefinesReal(text, '%', false, context);
    }

    std::string fixDefinesMsgBox(std::string text, Context& context){
        return fixDefinesReal(text, '^', false, context);
    }

    std::string fixDefinesBook(std::string text, Context& context){
        return fixDefinesReal(text, '%', true, context);
    }
}
