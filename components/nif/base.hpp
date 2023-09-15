/// This file holds the main classes of NIF Records used by everything else.
#ifndef OPENMW_COMPONENTS_NIF_BASE_HPP
#define OPENMW_COMPONENTS_NIF_BASE_HPP

#include "recordptr.hpp"

namespace Nif
{
    struct File;
    struct Record;
    struct Stream;

    // An extra data record. All the extra data connected to an object form a linked list.
    struct Extra : public Record
    {
        std::string mName;
        ExtraPtr mNext; // Next extra data record in the list
        uint32_t mRecordSize{ 0u };

        void read(NIFStream* nif) override;
        void post(Reader& nif) override { mNext.post(nif); }
    };

    struct NiTimeController : public Record
    {
        enum Flags
        {
            Flag_Active = 0x8
        };

        enum ExtrapolationMode
        {
            Cycle = 0,
            Reverse = 2,
            Constant = 4,
            Mask = 6
        };

        NiTimeControllerPtr mNext;
        uint16_t mFlags;
        float mFrequency, mPhase;
        float mTimeStart, mTimeStop;
        NiObjectNETPtr mTarget;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;

        bool isActive() const { return mFlags & Flag_Active; }
        ExtrapolationMode extrapolationMode() const { return static_cast<ExtrapolationMode>(mFlags & Mask); }
    };

    /// Abstract object that has a name, extra data and controllers
    struct NiObjectNET : public Record
    {
        std::string mName;
        ExtraPtr mExtra;
        ExtraList mExtraList;
        NiTimeControllerPtr mController;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;

        // Collect extra records attached to the object
        ExtraList getExtraList() const;
    };

}
#endif
