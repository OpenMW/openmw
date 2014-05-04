#ifndef GAME_MWWORLD_GLOBALS_H
#define GAME_MWWORLD_GLOBALS_H

#include <vector>
#include <string>
#include <map>

#include <stdint.h>

#include <components/interpreter/types.hpp>
#include <components/esm/variant.hpp>

namespace ESM
{
    class ESMWriter;
    class ESMReader;
}

namespace MWWorld
{
    class ESMStore;

    class Globals
    {
        private:

            typedef std::map<std::string, ESM::Variant> Collection;

            Collection mVariables; // type, value

            Collection::const_iterator find (const std::string& name) const;

            Collection::iterator find (const std::string& name);

        public:

            const ESM::Variant& operator[] (const std::string& name) const;

            ESM::Variant& operator[] (const std::string& name);

            char getType (const std::string& name) const;
            ///< If there is no global variable with this name, ' ' is returned.

            void fill (const MWWorld::ESMStore& store);
            ///< Replace variables with variables from \a store with default values.

            int countSavedGameRecords() const;

            void write (ESM::ESMWriter& writer) const;

            bool readRecord (ESM::ESMReader& reader, int32_t type);
            ///< Records for variables that do not exist are dropped silently.
            ///
            /// \return Known type?

    };
}

#endif
