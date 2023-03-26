#ifndef OPENMW_ESM_DIAL_H
#define OPENMW_ESM_DIAL_H

#include <list>
#include <map>
#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"
#include "components/esm3/infoorder.hpp"

#include "loadinfo.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Dialogue topic and journal entries. The actual data is contained in
     * the INFO records following the DIAL.
     */

    struct Dialogue
    {
        using InfoContainer = std::list<DialInfo>;

        constexpr static RecNameInts sRecordId = REC_DIAL;
        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Dialogue"; }

        enum Type : std::int8_t
        {
            Topic = 0,
            Voice = 1,
            Greeting = 2,
            Persuasion = 3,
            Journal = 4,
            Unknown = -1 // Used for deleted dialogues
        };

        RefId mId;
        std::string mStringId;
        Type mType;
        InfoContainer mInfo;
        InfoOrder<DialInfo> mInfoOrder;

        // Parameters: Info ID, (Info iterator, Deleted flag)
        typedef std::map<ESM::RefId, std::pair<InfoContainer::iterator, bool>> LookupMap;

        void load(ESMReader& esm, bool& isDeleted);
        ///< Loads all sub-records of Dialogue record
        void loadId(ESMReader& esm);
        ///< Loads NAME sub-record of Dialogue record
        void loadData(ESMReader& esm, bool& isDeleted);
        ///< Loads all sub-records of Dialogue record, except NAME sub-record

        void save(ESMWriter& esm, bool isDeleted = false) const;

        /// Remove all INFOs that are deleted
        void setUp();

        /// Read the next info record
        void readInfo(ESMReader& esm);

        void blank();
        ///< Set record to default state (does not touch the ID and does not change the type).
    };
}
#endif
