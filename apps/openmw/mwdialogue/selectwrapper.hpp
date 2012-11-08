#ifndef GAME_MWDIALOGUE_SELECTWRAPPER_H
#define GAME_MWDIALOGUE_SELECTWRAPPER_H

#include <components/esm/loadinfo.hpp>

namespace MWDialogue
{
    class SelectWrapper
    {
            const ESM::DialInfo::SelectStruct& mSelect;
    
        public:
        
            enum Function
            {
                Function_None,
                Function_Journal,
                Function_Item
            };
            
            enum Type
            {
                Type_None,
                Type_Integer,
                Type_Numeric,
                Type_Boolean
            };
    
        public:
        
            SelectWrapper (const ESM::DialInfo::SelectStruct& select);
            
            Function getFunction() const;
            
            Type getType() const;
            
            bool IsInverted() const;
            
            bool selectCompare (int value) const;

            bool selectCompare (float value) const;
            
            bool selectCompare (bool value) const;
            
            std::string getName() const;
            ///< Return case-smashed name.
    };
}

#endif
