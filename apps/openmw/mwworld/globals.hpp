#ifndef GAME_MWWORLD_GLOBALS_H
#define GAME_MWWORLD_GLOBALS_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <components/esm3/loadglob.hpp>
#include <components/misc/strings/algorithm.hpp>

#include "globalvariablename.hpp"

namespace ESM
{
    class ESMWriter;
    class ESMReader;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class ESMStore;

    class Globals
    {
    private:
        using Collection = std::map<ESM::RefId, ESM::Global, std::less<>>;

        Collection mVariables; // type, value

        Collection::const_iterator find(std::string_view name) const;

        Collection::iterator find(std::string_view name);

    public:
        static constexpr GlobalVariableName sDaysPassed{ "dayspassed" };
        static constexpr GlobalVariableName sGameHour{ "gamehour" };
        static constexpr GlobalVariableName sDay{ "day" };
        static constexpr GlobalVariableName sMonth{ "month" };
        static constexpr GlobalVariableName sYear{ "year" };
        static constexpr GlobalVariableName sTimeScale{ "timescale" };
        static constexpr GlobalVariableName sCharGenState{ "chargenstate" };
        static constexpr GlobalVariableName sPCHasCrimeGold{ "pchascrimegold" };
        static constexpr GlobalVariableName sPCHasGoldDiscount{ "pchasgolddiscount" };
        static constexpr GlobalVariableName sCrimeGoldDiscount{ "crimegolddiscount" };
        static constexpr GlobalVariableName sCrimeGoldTurnIn{ "crimegoldturnin" };
        static constexpr GlobalVariableName sPCHasTurnIn{ "pchasturnin" };
        static constexpr GlobalVariableName sPCKnownWerewolf{ "pcknownwerewolf" };
        static constexpr GlobalVariableName sWerewolfClawMult{ "werewolfclawmult" };
        static constexpr GlobalVariableName sPCRace{ "pcrace" };

        const ESM::Variant& operator[](GlobalVariableName name) const;

        ESM::Variant& operator[](GlobalVariableName name);

        char getType(GlobalVariableName name) const;
        ///< If there is no global variable with this name, ' ' is returned.

        void fill(const MWWorld::ESMStore& store);
        ///< Replace variables with variables from \a store with default values.

        size_t countSavedGameRecords() const;

        void write(ESM::ESMWriter& writer, Loading::Listener& progress) const;

        bool readRecord(ESM::ESMReader& reader, uint32_t type);
        ///< Records for variables that do not exist are dropped silently.
        ///
        /// \return Known type?
    };
}

#endif
