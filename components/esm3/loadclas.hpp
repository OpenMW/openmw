#ifndef OPENMW_ESM_CLAS_H
#define OPENMW_ESM_CLAS_H

#include <array>
#include <cstdint>
#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

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

        static const std::string_view sGmstSpecializationIds[3];
        static const std::array<std::string_view, 3> specializationIndexToLuaId;

        struct CLDTstruct
        {
            std::array<int32_t, 2> mAttribute; // Attributes that get class bonus
            int32_t mSpecialization; // 0 = Combat, 1 = Magic, 2 = Stealth
            std::array<std::array<int32_t, 2>, 5> mSkills; // Minor and major skills.
            int32_t mIsPlayable; // 0x0001 - Playable class
            int32_t mServices;

            int32_t& getSkill(int index, bool major);
            ///< Throws an exception for invalid values of \a index.

            int32_t getSkill(int index, bool major) const;
            ///< Throws an exception for invalid values of \a index.
        }; // 60 bytes

        uint32_t mRecordFlags;
        std::string mName, mDescription;
        RefId mId;
        CLDTstruct mData;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID/index).
    };
}
#endif
