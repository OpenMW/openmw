#include "importer.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>

#include <osg/ImageUtils>
#include <osgDB/ReadFile>

#include <components/esm/defs.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include <components/esm3/player.hpp>
#include <components/esm3/savedgame.hpp>

#include <components/esm3/cellid.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/esm3/loadweap.hpp>

#include <components/misc/constants.hpp>

#include <components/toutf8/toutf8.hpp>

#include "importercontext.hpp"

#include "converter.hpp"

namespace
{

    void writeScreenshot(const ESM::Header& fileHeader, ESM::SavedGame& out)
    {
        if (fileHeader.mSCRS.size() != 128 * 128 * 4)
        {
            std::cerr << "Error: unexpected screenshot size " << std::endl;
            return;
        }

        osg::ref_ptr<osg::Image> image(new osg::Image);
        image->allocateImage(128, 128, 1, GL_RGB, GL_UNSIGNED_BYTE);

        // need to convert pixel format from BGRA to RGB as the jpg readerwriter doesn't support it otherwise
        auto it = fileHeader.mSCRS.begin();
        for (int y = 0; y < 128; ++y)
        {
            for (int x = 0; x < 128; ++x)
            {
                assert(image->data(x, y));
                *(image->data(x, y) + 2) = *it++;
                *(image->data(x, y) + 1) = *it++;
                *image->data(x, y) = *it++;
                ++it; // skip alpha
            }
        }

        image->flipVertical();

        std::stringstream ostream;

        osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension("jpg");
        if (!readerwriter)
        {
            std::cerr << "Error: can't write screenshot: no jpg readerwriter found" << std::endl;
            return;
        }

        osgDB::ReaderWriter::WriteResult result = readerwriter->writeImage(*image, ostream);
        if (!result.success())
        {
            std::cerr << "Error: can't write screenshot: " << result.message() << " code " << result.status()
                      << std::endl;
            return;
        }

        std::string data = ostream.str();
        out.mScreenshot = std::vector<char>(data.begin(), data.end());
    }

}

namespace ESSImport
{

    Importer::Importer(
        const std::filesystem::path& essfile, const std::filesystem::path& outfile, const std::string& encoding)
        : mEssFile(essfile)
        , mOutFile(outfile)
        , mEncoding(encoding)
    {
    }

    struct File
    {
        struct Subrecord
        {
            std::string mName;
            size_t mFileOffset;
            std::vector<unsigned char> mData;
        };

        struct Record
        {
            std::string mName;
            size_t mFileOffset;
            std::vector<Subrecord> mSubrecords;
        };

        std::vector<Record> mRecords;
    };

    void read(const std::filesystem::path& filename, File& file)
    {
        ESM::ESMReader esm;
        esm.open(filename);

        while (esm.hasMoreRecs())
        {
            ESM::NAME n = esm.getRecName();
            esm.getRecHeader();

            File::Record rec;
            rec.mName = n.toString();
            rec.mFileOffset = esm.getFileOffset();
            while (esm.hasMoreSubs())
            {
                File::Subrecord sub;
                esm.getSubName();
                esm.getSubHeader();
                sub.mFileOffset = esm.getFileOffset();
                sub.mName = esm.retSubName().toString();
                sub.mData.resize(esm.getSubSize());
                esm.getExact(sub.mData.data(), sub.mData.size());
                rec.mSubrecords.push_back(std::move(sub));
            }
            file.mRecords.push_back(std::move(rec));
        }
    }

    void Importer::compare()
    {
        // data that always changes (and/or is already fully decoded) should be blacklisted
        std::set<std::pair<std::string, std::string>> blacklist;
        blacklist.insert(std::make_pair("GLOB", "FLTV")); // gamehour
        blacklist.insert(std::make_pair("REFR", "DATA")); // player position
        blacklist.insert(std::make_pair("CELL", "NAM8")); // fog of war
        blacklist.insert(std::make_pair("GAME", "GMDT")); // weather data, current time always changes
        blacklist.insert(std::make_pair("CELL", "DELE")); // first 3 bytes are uninitialized

        // this changes way too often, name suggests some renderer internal data?
        blacklist.insert(std::make_pair("CELL", "ND3D"));
        blacklist.insert(std::make_pair("REFR", "ND3D"));

        File file1;
        read(mEssFile, file1);
        File file2;
        read(mOutFile, file2); // todo rename variable

        // FIXME: use max(size1, size2)
        for (size_t i = 0; i < file1.mRecords.size(); ++i)
        {
            File::Record rec = file1.mRecords[i];

            if (i >= file2.mRecords.size())
            {
                std::ios::fmtflags f(std::cout.flags());
                std::cout << "Record in file1 not present in file2: (1) 0x" << std::hex << rec.mFileOffset << std::endl;
                std::cout.flags(f);
                return;
            }

            File::Record rec2 = file2.mRecords[i];

            if (rec.mName != rec2.mName)
            {
                std::ios::fmtflags f(std::cout.flags());
                std::cout << "Different record name at (2) 0x" << std::hex << rec2.mFileOffset << std::endl;
                std::cout.flags(f);
                return; // TODO: try to recover
            }

            // FIXME: use max(size1, size2)
            for (size_t j = 0; j < rec.mSubrecords.size(); ++j)
            {
                File::Subrecord sub = rec.mSubrecords[j];

                if (j >= rec2.mSubrecords.size())
                {
                    std::ios::fmtflags f(std::cout.flags());
                    std::cout << "Subrecord in file1 not present in file2: (1) 0x" << std::hex << sub.mFileOffset
                              << std::endl;
                    std::cout.flags(f);
                    return;
                }

                File::Subrecord sub2 = rec2.mSubrecords[j];

                if (sub.mName != sub2.mName)
                {
                    std::ios::fmtflags f(std::cout.flags());
                    std::cout << "Different subrecord name (" << rec.mName << "." << sub.mName << " vs. " << sub2.mName
                              << ") at (1) 0x" << std::hex << sub.mFileOffset << " (2) 0x" << sub2.mFileOffset
                              << std::endl;
                    std::cout.flags(f);
                    break; // TODO: try to recover
                }

                if (sub.mData != sub2.mData)
                {
                    if (blacklist.find(std::make_pair(rec.mName, sub.mName)) != blacklist.end())
                        continue;

                    std::ios::fmtflags f(std::cout.flags());

                    std::cout << "Different subrecord data for " << rec.mName << "." << sub.mName << " at (1) 0x"
                              << std::hex << sub.mFileOffset << " (2) 0x" << sub2.mFileOffset << std::endl;

                    std::cout << "Data 1:" << std::endl;
                    for (size_t k = 0; k < sub.mData.size(); ++k)
                    {
                        bool different = false;
                        if (k >= sub2.mData.size() || sub2.mData[k] != sub.mData[k])
                            different = true;

                        if (different)
                            std::cout << "\033[033m";
                        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)sub.mData[k] << " ";
                        if (different)
                            std::cout << "\033[0m";
                    }
                    std::cout << std::endl;

                    std::cout << "Data 2:" << std::endl;
                    for (size_t k = 0; k < sub2.mData.size(); ++k)
                    {
                        bool different = false;
                        if (k >= sub.mData.size() || sub.mData[k] != sub2.mData[k])
                            different = true;

                        if (different)
                            std::cout << "\033[033m";
                        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)sub2.mData[k] << " ";
                        if (different)
                            std::cout << "\033[0m";
                    }
                    std::cout << std::endl;
                    std::cout.flags(f);
                }
            }
        }
    }

    void Importer::run()
    {
        ToUTF8::Utf8Encoder encoder(ToUTF8::calculateEncoding(mEncoding));
        ESM::ESMReader esm;
        esm.open(mEssFile);
        esm.setEncoder(&encoder);

        Context context;

        const ESM::Header& header = esm.getHeader();
        context.mPlayerCellName = header.mGameData.mCurrentCell.toString();

        const unsigned int recREFR = ESM::fourCC("REFR");
        const unsigned int recPCDT = ESM::fourCC("PCDT");
        const unsigned int recFMAP = ESM::fourCC("FMAP");
        const unsigned int recKLST = ESM::fourCC("KLST");
        const unsigned int recSTLN = ESM::fourCC("STLN");
        const unsigned int recGAME = ESM::fourCC("GAME");
        const unsigned int recJOUR = ESM::fourCC("JOUR");
        const unsigned int recSPLM = ESM::fourCC("SPLM");

        std::map<unsigned int, std::unique_ptr<Converter>> converters;
        converters[ESM::REC_GLOB] = std::make_unique<ConvertGlobal>();
        converters[ESM::REC_BOOK] = std::make_unique<ConvertBook>();
        converters[ESM::REC_NPC_] = std::make_unique<ConvertNPC>();
        converters[ESM::REC_CREA] = std::make_unique<ConvertCREA>();
        converters[ESM::REC_NPCC] = std::make_unique<ConvertNPCC>();
        converters[ESM::REC_CREC] = std::make_unique<ConvertCREC>();
        converters[recREFR] = std::make_unique<ConvertREFR>();
        converters[recPCDT] = std::make_unique<ConvertPCDT>();
        converters[recFMAP] = std::make_unique<ConvertFMAP>();
        converters[recKLST] = std::make_unique<ConvertKLST>();
        converters[recSTLN] = std::make_unique<ConvertSTLN>();
        converters[recGAME] = std::make_unique<ConvertGAME>();
        converters[ESM::REC_CELL] = std::make_unique<ConvertCell>();
        converters[ESM::REC_ALCH] = std::make_unique<DefaultConverter<ESM::Potion>>();
        converters[ESM::REC_CLAS] = std::make_unique<ConvertClass>();
        converters[ESM::REC_SPEL] = std::make_unique<DefaultConverter<ESM::Spell>>();
        converters[ESM::REC_ARMO] = std::make_unique<DefaultConverter<ESM::Armor>>();
        converters[ESM::REC_WEAP] = std::make_unique<DefaultConverter<ESM::Weapon>>();
        converters[ESM::REC_CLOT] = std::make_unique<DefaultConverter<ESM::Clothing>>();
        converters[ESM::REC_ENCH] = std::make_unique<DefaultConverter<ESM::Enchantment>>();
        converters[ESM::REC_WEAP] = std::make_unique<DefaultConverter<ESM::Weapon>>();
        converters[ESM::REC_LEVC] = std::make_unique<DefaultConverter<ESM::CreatureLevList>>();
        converters[ESM::REC_LEVI] = std::make_unique<DefaultConverter<ESM::ItemLevList>>();
        converters[ESM::REC_CNTC] = std::make_unique<ConvertCNTC>();
        converters[ESM::REC_FACT] = std::make_unique<ConvertFACT>();
        converters[ESM::REC_INFO] = std::make_unique<ConvertINFO>();
        converters[ESM::REC_DIAL] = std::make_unique<ConvertDIAL>();
        converters[ESM::REC_QUES] = std::make_unique<ConvertQUES>();
        converters[recJOUR] = std::make_unique<ConvertJOUR>();
        converters[ESM::REC_SCPT] = std::make_unique<ConvertSCPT>();
        converters[ESM::REC_PROJ] = std::make_unique<ConvertPROJ>();
        converters[recSPLM] = std::make_unique<ConvertSPLM>();

        // TODO:
        // - REGN (weather in certain regions?)
        // - VFXM
        // - SPLM (active spell effects)

        std::set<unsigned int> unknownRecords;

        for (const auto& converter : converters)
        {
            converter.second->setContext(context);
        }

        while (esm.hasMoreRecs())
        {
            ESM::NAME n = esm.getRecName();
            esm.getRecHeader();

            auto it = converters.find(n.toInt());
            if (it != converters.end())
            {
                it->second->read(esm);
            }
            else
            {
                if (unknownRecords.insert(n.toInt()).second)
                {
                    std::ios::fmtflags f(std::cerr.flags());
                    std::cerr << "Error: unknown record " << n.toString() << " (0x" << std::hex << esm.getFileOffset()
                              << ")" << std::endl;
                    std::cerr.flags(f);
                }

                esm.skipRecord();
            }
        }

        ESM::ESMWriter writer;

        writer.setFormatVersion(ESM::CurrentSaveGameFormatVersion);

        std::ofstream stream(mOutFile, std::ios::out | std::ios::binary);
        // all unused
        writer.setVersion(0);
        writer.setType(0);
        writer.setAuthor("");
        writer.setDescription("");
        writer.setRecordCount(0);

        for (const auto& master : header.mMaster)
            writer.addMaster(master.name, 0); // not using the size information anyway -> use value of 0

        writer.save(stream);

        ESM::SavedGame profile;
        for (const auto& master : header.mMaster)
        {
            profile.mContentFiles.push_back(master.name);
        }
        profile.mDescription = esm.getDesc();
        profile.mInGameTime.mDay = context.mDay;
        profile.mInGameTime.mGameHour = context.mHour;
        profile.mInGameTime.mMonth = context.mMonth;
        profile.mInGameTime.mYear = context.mYear;
        profile.mTimePlayed = 0;
        profile.mPlayerCellName = context.mPlayerCellName;
        if (context.mPlayerBase.mClass == "NEWCLASSID_CHARGEN")
            profile.mPlayerClassName = context.mCustomPlayerClassName;
        else
            profile.mPlayerClassId = context.mPlayerBase.mClass;
        profile.mPlayerLevel = context.mPlayerBase.mNpdt.mLevel;
        profile.mPlayerName = header.mGameData.mPlayerName.toString();

        writeScreenshot(header, profile);

        writer.startRecord(ESM::REC_SAVE);
        profile.save(writer);
        writer.endRecord(ESM::REC_SAVE);

        // Writing order should be Dynamic Store -> Cells -> Player,
        // so that references to dynamic records can be recognized when loading
        for (auto it = converters.begin(); it != converters.end(); ++it)
        {
            if (it->second->getStage() != 0)
                continue;
            it->second->write(writer);
        }

        writer.startRecord(ESM::REC_NPC_);
        context.mPlayerBase.mId = ESM::RefId::stringRefId("Player");
        context.mPlayerBase.save(writer);
        writer.endRecord(ESM::REC_NPC_);

        for (auto it = converters.begin(); it != converters.end(); ++it)
        {
            if (it->second->getStage() != 1)
                continue;
            it->second->write(writer);
        }

        writer.startRecord(ESM::REC_PLAY);
        ESM::CellId cellId = ESM::CellId::extractFromRefId(context.mPlayer.mCellId);
        int cellX = static_cast<int>(std::floor(context.mPlayer.mObject.mPosition.pos[0] / Constants::CellSizeInUnits));
        int cellY = static_cast<int>(std::floor(context.mPlayer.mObject.mPosition.pos[1] / Constants::CellSizeInUnits));

        context.mPlayer.mCellId = ESM::Cell::generateIdForCell(cellId.mPaged, cellId.mWorldspace, cellX, cellY);
        context.mPlayer.save(writer);
        writer.endRecord(ESM::REC_PLAY);

        writer.startRecord(ESM::REC_ACTC);
        writer.writeHNT("COUN", context.mNextActorId);
        writer.endRecord(ESM::REC_ACTC);

        // Stage 2 requires cell references to be written / actors IDs assigned
        for (auto it = converters.begin(); it != converters.end(); ++it)
        {
            if (it->second->getStage() != 2)
                continue;
            it->second->write(writer);
        }

        writer.startRecord(ESM::REC_DIAS);
        context.mDialogueState.save(writer);
        writer.endRecord(ESM::REC_DIAS);

        writer.startRecord(ESM::REC_INPU);
        context.mControlsState.save(writer);
        writer.endRecord(ESM::REC_INPU);
    }

}
