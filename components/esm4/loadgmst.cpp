#include "loadgmst.hpp"

#include <stdexcept>

#include "reader.hpp"

namespace ESM4
{
    namespace
    {
        GameSetting::Data readData(FormId formId, std::string_view editorId, Reader& reader)
        {
            if (editorId.empty())
                throw std::runtime_error("Unknown ESM4 GMST (" + formId.toString() + ") data type: editor id is empty");
            const char type = editorId[0];
            switch (type)
            {
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
                    reader.getZString(value);
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
        mFormId = reader.hdr().record.getFormId();
        reader.adjustFormId(mFormId);
        mFlags = reader.hdr().record.flags;

        while (reader.getSubRecordHeader())
        {
            const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
            switch (subHdr.typeId)
            {
                case ESM4::SUB_EDID:
                    reader.getZString(mEditorId);
                    break;
                case ESM4::SUB_DATA:
                    mData = readData(mFormId, mEditorId, reader);
                    break;
                default:
                    throw std::runtime_error(
                        "Unknown ESM4 GMST (" + mFormId.toString() + ") subrecord " + ESM::printName(subHdr.typeId));
            }
        }
    }

}
