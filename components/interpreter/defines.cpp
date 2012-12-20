#include "defines.hpp"

#include <iostream>
#include <algorithm>
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

    std::string fixDefinesReal(std::string text, char eschar, Context& context){

        unsigned int start = 0;
        std::string retval = "";
        for(unsigned int i = 0; i < text.length(); i++){
            if(text[i] == eschar){
                retval += text.substr(start, i - start);
                std::string temp = text.substr(i+1, 100);
                transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
                
                bool found;
            
                if(     (found = Check(temp, "actionslideright", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_SLIDE_RIGHT";
                }
                else if((found = Check(temp, "actionreadymagic", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_READY_MAGIC";
                }
                else if((found = Check(temp, "actionprevweapon", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_PREV_WEAPON";
                }
                else if((found = Check(temp, "actionnextweapon", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_PREV_WEAPON";
                }
                else if((found = Check(temp, "actiontogglerun", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_TOGGLE_RUN";
                }
                else if((found = Check(temp, "actionslideleft", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_TOGGLE_RUN";
                }
                else if((found = Check(temp, "actionreadyitem", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_READY_ITEM";
                }
                else if((found = Check(temp, "actionprevspell", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_PREV_SPELL";
                }
                else if((found = Check(temp, "actionnextspell", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_NEXT_SPELL";
                }
                else if((found = Check(temp, "actionrestmenu", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_REST_MENU";
                }
                else if((found = Check(temp, "actionmenumode", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_MENU_MODE";
                }
                else if((found = Check(temp, "actionactivate", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_ACTIVATE";
                }
                else if((found = Check(temp, "actionjournal", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_JOURNAL";
                }
                else if((found = Check(temp, "actionforward", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_FORWARD";
                }
                else if((found = Check(temp, "pccrimelevel", &i, &start))){
                    retval += "PLACEHOLDER_PC_CRIME_LEVEL";
                }
                else if((found = Check(temp, "actioncrouch", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_CROUCH";
                }
                else if((found = Check(temp, "actionjump", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_JUMP";
                }
                else if((found = Check(temp, "actionback", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_BACK";
                }
                else if((found = Check(temp, "actionuse", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_USE";
                }
                else if((found = Check(temp, "actionrun", &i, &start))){
                    retval += "PLACEHOLDER_ACTION_RUN";
                }
                else if((found = Check(temp, "pcclass", &i, &start))){
                    retval += "PLACEHOLDER_PC_CLASS";
                }
                else if((found = Check(temp, "pcrace", &i, &start))){
                    retval += "PLACEHOLDER_PC_RACE";
                }
                else if((found = Check(temp, "pcname", &i, &start))){
                    retval += "PLACEHOLDER_PC_NAME";    
                }
                else if((found = Check(temp, "cell", &i, &start))){
                    retval += "PLACEHOLDER_CELL";
                }

                else if(eschar == '%'){ // In Dialogue, not messagebox
                    if(     (found = Check(temp, "faction", &i, &start))){
                        retval += "PLACEHOLDER_FACTION";
                    }
                    else if((found = Check(temp, "nextpcrank", &i, &start))){
                        retval += "PLACEHOLDER_NEXT_PC_RANK";
                    }
                    else if((found = Check(temp, "pcnextrank", &i, &start))){
                        retval += "PLACEHOLDER_PC_NEXT_RANK";
                    }
                    else if((found = Check(temp, "pcrank", &i, &start))){
                        retval += "PLACEHOLDER_PC_RANK";
                    }
                    else if((found = Check(temp, "rank", &i, &start))){
                        retval += "PLACEHOLDER_RANK";
                    }
                    
                    else if((found = Check(temp, "class", &i, &start))){
                        retval += "PLACEHOLDER_CLASS";
                    }
                    else if((found = Check(temp, "race", &i, &start))){
                        retval += "PLACEHOLDER_RACE";
                    }
                    else if((found = Check(temp, "name", &i, &start))){
                        retval += "PLACEHOLDER_NAME";
                    }
                }
                else if(eschar == '^') { // In messagebox, not dialogue

                    /* empty in messageboxes */
                    if(     (found = Check(temp, "faction", &i, &start)));
                    else if((found = Check(temp, "nextpcrank", &i, &start)));
                    else if((found = Check(temp, "pcnextrank", &i, &start)));
                    else if((found = Check(temp, "pcrank", &i, &start)));
                    else if((found = Check(temp, "rank", &i, &start)));
                    
                    /* uses pc in messageboxes */
                    else if((found = Check(temp, "class", &i, &start))){
                        retval += "PLACEHOLDER_CLASS";
                    }
                    else if((found = Check(temp, "race", &i, &start))){
                        retval += "PLACEHOLDER_RACE";
                    }
                    else if((found = Check(temp, "name", &i, &start))){
                        retval += "PLACEHOLDER_NAME";
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
                        if((found = Check(temp, globals[j], &i, &start))){
                            char type = context.getGlobalType(globals[j]);

                            switch(type){
                                case 's': retval += std::to_string(context.getGlobalShort(globals[j]));  break;
                                case 'l': retval += std::to_string(context.getGlobalLong(globals[j])); break; 
                                case 'f': retval += std::to_string(context.getGlobalFloat(globals[j])); break;
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
                    retval += eschar;
                }
            }
        }
        retval += text.substr(start, text.length() - start);
        return retval;
    }

    std::string fixDefinesDialog(std::string text, Context& context){
        return fixDefinesReal(text, '%', context);
    }

    std::string fixDefinesMsgBox(std::string text, Context& context){
        return fixDefinesReal(text, '^', context);
    }
}
