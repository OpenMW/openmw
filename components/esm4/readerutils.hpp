#ifndef OPENMW_COMPONENTS_ESM4_READERUTILS
#define OPENMW_COMPONENTS_ESM4_READERUTILS

#include "grouptype.hpp"
#include "reader.hpp"

namespace ESM4
{
    struct ReaderUtils
    {

        /* RecordInvocable must be an invocable, takes an ESM4::Reader& as input, and outputs a boolean that indicates
        if the record was read or ignored. Will be invoked for every record

        GroupInvocable's invocable must take a ESM4::Reader& as input, doesn't need to output anything. Will be invoked
        for every group*/
        template <typename RecordInvocable, typename GroupInvocable>
        static void readAll(ESM4::Reader& reader, RecordInvocable&& recordInvocable, GroupInvocable&& groupInvocable)
        {
            while (reader.hasMoreRecs())
            {
                reader.exitGroupCheck();
                if (!readItem(reader, recordInvocable, groupInvocable))
                    break;
            }
        }

        template <typename RecordInvocable>
        static void readRecord(ESM4::Reader& reader, RecordInvocable&& recordInvocable)
        {
            if (!recordInvocable(reader))
                reader.skipRecordData();
        }

        template <typename RecordInvocable, typename GroupInvocable>
        static bool readGroup(ESM4::Reader& reader, RecordInvocable&& recordInvocable, GroupInvocable&& groupInvocable)
        {
            const ESM4::RecordHeader& header = reader.hdr();

            groupInvocable(reader);

            switch (static_cast<ESM4::GroupType>(header.group.type))
            {
                case ESM4::Grp_RecordType:
                case ESM4::Grp_InteriorCell:
                case ESM4::Grp_InteriorSubCell:
                case ESM4::Grp_ExteriorCell:
                case ESM4::Grp_ExteriorSubCell:
                    reader.enterGroup();
                    return readItem(reader, recordInvocable, groupInvocable);
                case ESM4::Grp_WorldChild:
                case ESM4::Grp_CellChild:
                case ESM4::Grp_TopicChild:
                case ESM4::Grp_CellPersistentChild:
                case ESM4::Grp_CellTemporaryChild:
                case ESM4::Grp_CellVisibleDistChild:
                    reader.adjustGRUPFormId();
                    reader.enterGroup();
                    if (!reader.hasMoreRecs())
                        return false;
                    return readItem(reader, recordInvocable, groupInvocable);
            }

            reader.skipGroup();

            return true;
        }

        template <typename RecordInvocable, typename GroupInvocable>
        static bool readItem(ESM4::Reader& reader, RecordInvocable&& recordInvocable, GroupInvocable&& groupInvocable)
        {
            if (!reader.getRecordHeader() || !reader.hasMoreRecs())
                return false;

            const ESM4::RecordHeader& header = reader.hdr();

            if (header.record.typeId == ESM4::REC_GRUP)
                return readGroup(reader, recordInvocable, groupInvocable);

            readRecord(reader, recordInvocable);
            return true;
        }
    };
}

#endif // OPENMW_COMPONENTS_ESM4_READERUTILS
