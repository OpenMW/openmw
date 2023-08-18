#ifndef OPENMW_COMPONENTS_ESM4_LOADGMST_H
#define OPENMW_COMPONENTS_ESM4_LOADGMST_H

#include <cstdint>
#include <string>
#include <variant>

#include <components/esm/defs.hpp>
#include <components/esm/formid.hpp>

namespace ESM4
{
    class Reader;

    struct GameSetting
    {
        using Data = std::variant<std::monostate, bool, float, std::int32_t, std::string, std::uint32_t>;

        ESM::FormId mId; // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details
        std::string mEditorId;
        Data mData;

        void load(Reader& reader);
        static constexpr ESM::RecNameInts sRecordId = ESM::RecNameInts::REC_GMST4;
    };
}

#endif // OPENMW_COMPONENTS_ESM4_LOADGMST_H
