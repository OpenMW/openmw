#ifndef GAME_MWWORLD_GLOBALS_H
#define GAME_MWWORLD_GLOBALS_H

#include <vector>
#include <string>
#include <map>

#include <components/interpreter/types.hpp>

namespace MWWorld
{
    class ESMStore;

    class Globals
    {
        public:
        
            union Data
            {
                Interpreter::Type_Float mFloat;
                Interpreter::Type_Float mLong; // Why Morrowind, why? :(
                Interpreter::Type_Float mShort;
            };
        
            typedef std::map<std::string, std::pair<char, Data> > Collection;
        
        private:
        
            Collection mVariables; // type, value
        
            Collection::const_iterator find (const std::string& name) const;

            Collection::iterator find (const std::string& name);
        
        public:
        
            Globals (const MWWorld::ESMStore& store);
        
            const Data& operator[] (const std::string& name) const;

            Data& operator[] (const std::string& name);
            
            void setInt (const std::string& name, int value);
            ///< Set value independently from real type.
            
            void setFloat (const std::string& name, float value);
            ///< Set value independently from real type.
            
            int getInt (const std::string& name) const;
            ///< Get value independently from real type.
                
            float getFloat (const std::string& name) const;
            ///< Get value independently from real type.
            
            char getType (const std::string& name) const;
            ///< If there is no global variable with this name, ' ' is returned.

            std::vector<std::string> getGlobals () const;
    };
}

#endif
