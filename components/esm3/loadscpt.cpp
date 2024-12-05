#include "loadscpt.hpp"

#include <algorithm>
#include <numeric>
#include <optional>
#include <sstream>

#include <components/debug/debuglog.hpp>
#include <components/misc/concepts.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    namespace
    {
        struct SCHD
        {
            /// Data from script-precompling in the editor.
            /// \warning Do not use them. OpenCS currently does not precompile scripts.
            std::uint32_t mNumShorts = 0;
            std::uint32_t mNumLongs = 0;
            std::uint32_t mNumFloats = 0;
            std::uint32_t mScriptDataSize = 0;
            std::uint32_t mStringTableSize = 0;
        };

        template <Misc::SameAsWithoutCvref<SCHD> T>
        void decompose(T&& v, const auto& f)
        {
            f(v.mNumShorts, v.mNumLongs, v.mNumFloats, v.mScriptDataSize, v.mStringTableSize);
        }

        void loadVarNames(const SCHD& header, std::vector<std::string>& varNames, ESMReader& esm)
        {
            uint32_t s = header.mStringTableSize;

            std::vector<char> tmp(s);
            // not using getHExact, vanilla doesn't seem to mind unused bytes at the end
            esm.getSubHeader();
            uint32_t left = esm.getSubSize();
            if (left < s)
                esm.fail("SCVR string list is smaller than specified");
            esm.getExact(tmp.data(), s);
            if (left > s)
                esm.skip(left - s); // skip the leftover junk

            // Set up the list of variable names
            varNames.resize(header.mNumShorts + header.mNumLongs + header.mNumFloats);

            // The tmp buffer is a null-byte separated string list, we
            // just have to pick out one string at a time.
            if (tmp.empty())
            {
                if (!varNames.empty())
                    Log(Debug::Warning) << "SCVR with no variable names";

                return;
            }

            // Support '\r' terminated strings like vanilla.  See Bug #1324.
            std::replace(tmp.begin(), tmp.end(), '\r', '\0');
            // Avoid heap corruption
            if (tmp.back() != '\0')
            {
                tmp.push_back('\0');
                std::stringstream ss;
                ss << "Malformed string table";
                ss << "\n  File: " << esm.getName();
                ss << "\n  Record: " << esm.getContext().recName.toStringView();
                ss << "\n  Subrecord: "
                   << "SCVR";
                ss << "\n  Offset: 0x" << std::hex << esm.getFileOffset();
                Log(Debug::Verbose) << ss.str();
            }

            const char* str = tmp.data();
            const char* const tmpEnd = tmp.data() + tmp.size();
            for (size_t i = 0; i < varNames.size(); i++)
            {
                varNames[i] = std::string(str);
                str += varNames[i].size() + 1;
                if (str >= tmpEnd)
                {
                    if (str > tmpEnd)
                    {
                        // SCVR subrecord is unused and variable names are determined
                        // from the script source, so an overflow is not fatal.
                        std::stringstream ss;
                        ss << "String table overflow";
                        ss << "\n  File: " << esm.getName();
                        ss << "\n  Record: " << esm.getContext().recName.toStringView();
                        ss << "\n  Subrecord: "
                           << "SCVR";
                        ss << "\n  Offset: 0x" << std::hex << esm.getFileOffset();
                        Log(Debug::Verbose) << ss.str();
                    }
                    // Get rid of empty strings in the list.
                    varNames.resize(i + 1);
                    break;
                }
            }
        }
    }

    void Script::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mVarNames.clear();

        std::optional<SCHD> header;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("SCHD"):
                {
                    esm.getSubHeader();
                    mId = esm.getMaybeFixedRefIdSize(32);
                    esm.getComposite(header.emplace());
                    mNumShorts = header->mNumShorts;
                    mNumLongs = header->mNumLongs;
                    mNumFloats = header->mNumFloats;
                    break;
                }
                case fourCC("SCVR"):
                    if (!header.has_value())
                        esm.fail("SCVR is placed before SCHD record");
                    loadVarNames(*header, mVarNames, esm);
                    break;
                case fourCC("SCDT"):
                {
                    if (!header.has_value())
                        esm.fail("SCDT is placed before SCHD record");

                    // compiled script
                    esm.getSubHeader();
                    uint32_t subSize = esm.getSubSize();

                    if (subSize != header->mScriptDataSize)
                    {
                        std::stringstream ss;
                        ss << "Script data size defined in SCHD subrecord does not match size of SCDT subrecord";
                        ss << "\n  File: " << esm.getName();
                        ss << "\n  Offset: 0x" << std::hex << esm.getFileOffset();
                        Log(Debug::Verbose) << ss.str();
                    }

                    mScriptData.resize(subSize);
                    esm.getExact(mScriptData.data(), mScriptData.size());
                    break;
                }
                case fourCC("SCTX"):
                    mScriptText = esm.getHString();
                    break;
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!header.has_value())
            esm.fail("Missing SCHD subrecord");
    }

    void Script::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.startSubRecord("SCHD");
        esm.writeMaybeFixedSizeRefId(mId, 32);
        esm.writeComposite(SCHD{
            .mNumShorts = mNumShorts,
            .mNumLongs = mNumLongs,
            .mNumFloats = mNumFloats,
            .mScriptDataSize = static_cast<std::uint32_t>(mScriptData.size()),
            .mStringTableSize = computeScriptStringTableSize(mVarNames),
        });
        esm.endRecord("SCHD");

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        if (!mVarNames.empty())
        {
            esm.startSubRecord("SCVR");
            for (const std::string& v : mVarNames)
                esm.writeHCString(v);
            esm.endRecord("SCVR");
        }

        esm.startSubRecord("SCDT");
        esm.write(reinterpret_cast<const char*>(mScriptData.data()), mScriptData.size());
        esm.endRecord("SCDT");

        esm.writeHNOString("SCTX", mScriptText);
    }

    void Script::blank()
    {
        mRecordFlags = 0;
        mNumShorts = 0;
        mNumLongs = 0;
        mNumFloats = 0;

        mVarNames.clear();
        mScriptData.clear();
        const std::string& stringId = mId.getRefIdString();
        if (stringId.find("::") != std::string::npos)
            mScriptText = "Begin \"" + stringId + "\"\n\nEnd " + stringId + "\n";
        else
            mScriptText = "Begin " + stringId + "\n\nEnd " + stringId + "\n";
    }

    std::uint32_t computeScriptStringTableSize(const std::vector<std::string>& varNames)
    {
        return std::accumulate(varNames.begin(), varNames.end(), static_cast<std::uint32_t>(0),
            [](std::uint32_t r, const std::string& v) { return r + 1 + static_cast<std::uint32_t>(v.size()); });
    }
}
