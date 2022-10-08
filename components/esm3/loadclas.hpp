#ifndef OPENMW_ESM_CLAS_H
#define OPENMW_ESM_CLAS_H

#include <string>

#include "components/esm/defs.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Character class definitions
     */
    struct Class
    {
        constexpr static RecNameInts sRecordId = REC_CLAS;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Class"; }

        enum Specialization
        {
            Combat = 0,
            Magic = 1,
            Stealth = 2
        };

        static const Specialization sSpecializationIds[3];
        static const char* sGmstSpecializationIds[3];

        struct CLDTstruct
        {
            int mAttribute[2]; // Attributes that get class bonus
            int mSpecialization; // 0 = Combat, 1 = Magic, 2 = Stealth
            int mSkills[5][2]; // Minor and major skills.
            int mIsPlayable; // 0x0001 - Playable class
            int mServices;

            int& getSkill(int index, bool major);
            ///< Throws an exception for invalid values of \a index.

            int getSkill(int index, bool major) const;
            ///< Throws an exception for invalid values of \a index.
        }; // 60 bytes

        unsigned int mRecordFlags;
        std::string mId, mName, mDescription;
        CLDTstruct mData;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID/index).
    };
}
#endif
