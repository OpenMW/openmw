#include "loadgmst.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

#include "reader.hpp"

namespace ESM4
{
    namespace
    {
        GameSetting::Data readData(ESM::FormId formId, std::string_view editorId, Reader& reader)
        {
            if (editorId.empty())
            {
                reader.skipSubRecordData();
                return std::monostate{};
            }
            const char type = editorId[0];
            switch (type)
            {
                case 'b':
                {
                    std::uint32_t value = 0;
                    reader.get(value);
                    return value != 0;
                }
                case 'i':
                {
                    std::int32_t value = 0;
                    reader.get(value);
                    return value;
                }
                case 'f':
                {
                    float value = 0;
                    reader.get(value);
                    return value;
                }
                case 's':
                {
                    std::string value;
                    reader.getLocalizedString(value);
                    return value;
                }
                case 'u':
                {
                    std::uint32_t value = 0;
                    reader.get(value);
                    return value;
                }
                default:
                    throw std::runtime_error(
                        "Unsupported ESM4 GMST (" + formId.toString() + ") data type: " + std::string(editorId));
            }
        }
    }

    void GameSetting::load(Reader& reader)
    {
        mId = reader.getFormIdFromHeader();
        mFlags = reader.hdr().record.flags;

        while (reader.getSubRecordHeader())
        {
            const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
            switch (subHdr.typeId)
            {
                case ESM::fourCC("EDID"):
                    reader.getZString(mEditorId);
                    break;
                case ESM::fourCC("DATA"):
                    mData = readData(mId, mEditorId, reader);
                    break;
                default:
                    throw std::runtime_error(
                        "Unknown ESM4 GMST (" + mId.toString() + ") subrecord " + ESM::printName(subHdr.typeId));
            }
        }
    }

}
