#ifndef OPENMW_COMPONENTS_ESM4_LOADGMST_H
#define OPENMW_COMPONENTS_ESM4_LOADGMST_H

#include <cstdint>
#include <string>
#include <variant>

#include "formid.hpp"

namespace ESM4
{
    class Reader;

    struct GameSetting
    {
        using Data = std::variant<float, std::int32_t, std::string>;

        FormId mFormId; // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details
        std::string mEditorId;
        Data mData;

        void load(Reader& reader);
    };
}

#endif // OPENMW_COMPONENTS_ESM4_LOADGMST_H
