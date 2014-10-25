#ifndef OPENMW_ESM_DIAL_H
#define OPENMW_ESM_DIAL_H

#include <string>
#include <list>
#include <map>

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
    static unsigned int sRecordId;

    enum Type
    {
        Topic = 0,
        Voice = 1,
        Greeting = 2,
        Persuasion = 3,
        Journal = 4,
        Deleted = -1
    };

    std::string mId;
    signed char mType;

    typedef std::list<DialInfo> InfoContainer;

    typedef std::map<std::string, InfoContainer::iterator> LookupMap;

    InfoContainer mInfo;

    // This is only used during the loading phase to speed up DialInfo merging.
    LookupMap mLookup;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    /// Remove all INFOs marked as QS_Deleted from mInfos.
    void clearDeletedInfos();

    /// Read the next info record
    /// @param merge Merge with existing list, or just push each record to the end of the list?
    void readInfo (ESM::ESMReader& esm, bool merge);

    void blank();
    ///< Set record to default state (does not touch the ID and does not change the type).
};
}
#endif
