#include "load.hpp"
#include "esmdata.hpp"
#include "lessbyid.hpp"
#include "record.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm/cellref.hpp>
#include <components/esm/defs.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/loadacti.hpp>
#include <components/esm/loadcell.hpp>
#include <components/esm/loadcont.hpp>
#include <components/esm/loaddoor.hpp>
#include <components/esm/loadgmst.hpp>
#include <components/esm/loadland.hpp>
#include <components/esm/loadstat.hpp>
#include <components/files/collections.hpp>
#include <components/files/multidircollection.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/stringops.hpp>

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace EsmLoader
{
    namespace
    {
        struct GetKey
        {
            template <class T>
            decltype(auto) operator()(const T& v) const
            {
                return (v.mId);
            }

            const ESM::CellId& operator()(const ESM::Cell& v) const
            {
                return v.mCellId;
            }

            std::pair<int, int> operator()(const ESM::Land& v) const
            {
                return std::pair(v.mX, v.mY);
            }

            template <class T>
            decltype(auto) operator()(const Record<T>& v) const
            {
                return (*this)(v.mValue);
            }
        };

        struct CellRecords
        {
            Records<ESM::Cell> mValues;
            std::map<std::string, std::size_t> mByName;
            std::map<std::pair<int, int>, std::size_t> mByPosition;
        };

        template <class T, class = std::void_t<>>
        struct HasId : std::false_type {};

        template <class T>
        struct HasId<T, std::void_t<decltype(T::mId)>> : std::true_type {};

        template <class T>
        constexpr bool hasId = HasId<T>::value;

        template <class T>
        auto loadRecord(ESM::ESMReader& reader, Records<T>& records)
            -> std::enable_if_t<hasId<T>>
        {
            T record;
            bool deleted = false;
            record.load(reader, deleted);
            Misc::StringUtils::lowerCaseInPlace(record.mId);
            if (Misc::ResourceHelpers::isHiddenMarker(record.mId))
                return;
            records.emplace_back(deleted, std::move(record));
        }

        template <class T>
        auto loadRecord(ESM::ESMReader& reader, Records<T>& records)
            -> std::enable_if_t<!hasId<T>>
        {
            T record;
            bool deleted = false;
            record.load(reader, deleted);
            records.emplace_back(deleted, std::move(record));
        }

        void loadRecord(ESM::ESMReader& reader, CellRecords& records)
        {
            ESM::Cell record;
            bool deleted = false;
            record.loadNameAndData(reader, deleted);
            Misc::StringUtils::lowerCaseInPlace(record.mName);

            if ((record.mData.mFlags & ESM::Cell::Interior) != 0)
            {
                const auto it = records.mByName.find(record.mName);
                if (it == records.mByName.end())
                {
                    record.loadCell(reader, true);
                    records.mByName.emplace_hint(it, record.mName, records.mValues.size());
                    records.mValues.emplace_back(deleted, std::move(record));
                }
                else
                {
                    Record<ESM::Cell>& old = records.mValues[it->second];
                    old.mValue.mData = record.mData;
                    old.mValue.loadCell(reader, true);
                }
            }
            else
            {
                const std::pair<int, int> position(record.mData.mX, record.mData.mY);
                const auto it = records.mByPosition.find(position);
                if (it == records.mByPosition.end())
                {
                    record.loadCell(reader, true);
                    records.mByPosition.emplace_hint(it, position, records.mValues.size());
                    records.mValues.emplace_back(deleted, std::move(record));
                }
                else
                {
                    Record<ESM::Cell>& old = records.mValues[it->second];
                    old.mValue.mData = record.mData;
                    old.mValue.loadCell(reader, true);
                }
            }
        }

        struct ShallowContent
        {
            Records<ESM::Activator> mActivators;
            CellRecords mCells;
            Records<ESM::Container> mContainers;
            Records<ESM::Door> mDoors;
            Records<ESM::GameSetting> mGameSettings;
            Records<ESM::Land> mLands;
            Records<ESM::Static> mStatics;
        };

        void loadRecord(const Query& query, const ESM::NAME& name, ESM::ESMReader& reader, ShallowContent& content)
        {
            switch (name.toInt())
            {
                case ESM::REC_ACTI:
                    if (query.mLoadActivators)
                        return loadRecord(reader, content.mActivators);
                    break;
                case ESM::REC_CELL:
                    if (query.mLoadCells)
                        return loadRecord(reader, content.mCells);
                    break;
                case ESM::REC_CONT:
                    if (query.mLoadContainers)
                        return loadRecord(reader, content.mContainers);
                    break;
                case ESM::REC_DOOR:
                    if (query.mLoadDoors)
                        return loadRecord(reader, content.mDoors);
                    break;
                case ESM::REC_GMST:
                    if (query.mLoadGameSettings)
                        return loadRecord(reader, content.mGameSettings);
                    break;
                case ESM::REC_LAND:
                    if (query.mLoadLands)
                        return loadRecord(reader, content.mLands);
                    break;
                case ESM::REC_STAT:
                    if (query.mLoadStatics)
                        return loadRecord(reader, content.mStatics);
                    break;
            }

            reader.skipRecord();
        }

        ESM::ESMReader loadEsm(const Query& query, ESM::ESMReader& reader, ShallowContent& content)
        {
            Log(Debug::Info) << "Loading ESM file " << reader.getName();

            while (reader.hasMoreRecs())
            {
                const ESM::NAME recName = reader.getRecName();
                reader.getRecHeader();
                loadRecord(query, recName, reader, content);
            }

            return reader;
        }

        ShallowContent shallowLoad(const Query& query, const std::vector<std::string>& contentFiles,
            const Files::Collections& fileCollections, std::vector<ESM::ESMReader>& readers,
            ToUTF8::Utf8Encoder* encoder)
        {
            ShallowContent result;

            for (std::size_t i = 0; i < contentFiles.size(); ++i)
            {
                const std::string &file = contentFiles[i];
                const Files::MultiDirCollection& collection = fileCollections.getCollection(boost::filesystem::path(file).extension().string());

                ESM::ESMReader& reader = readers[i];
                reader.setEncoder(encoder);
                reader.setIndex(static_cast<int>(i));
                reader.setGlobalReaderList(&readers);
                reader.open(collection.getPath(file).string());

                loadEsm(query, readers[i], result);
            }

            return result;
        }

        struct WithType
        {
            ESM::RecNameInts mType;

            template <class T>
            RefIdWithType operator()(const T& v) const { return {v.mId, mType}; }
        };

        template <class T>
        void addRefIdsTypes(const std::vector<T>& values, std::vector<RefIdWithType>& refIdsTypes)
        {
            std::transform(values.begin(), values.end(), std::back_inserter(refIdsTypes),
                           WithType {static_cast<ESM::RecNameInts>(T::sRecordId)});
        }

        void addRefIdsTypes(EsmData& content)
        {
            content.mRefIdTypes.reserve(
                content.mActivators.size()
                + content.mContainers.size()
                + content.mDoors.size()
                + content.mStatics.size()
            );

            addRefIdsTypes(content.mActivators, content.mRefIdTypes);
            addRefIdsTypes(content.mContainers, content.mRefIdTypes);
            addRefIdsTypes(content.mDoors, content.mRefIdTypes);
            addRefIdsTypes(content.mStatics, content.mRefIdTypes);

            std::sort(content.mRefIdTypes.begin(), content.mRefIdTypes.end(), LessById {});
        }

        std::vector<ESM::Cell> prepareCellRecords(Records<ESM::Cell>& records)
        {
            std::vector<ESM::Cell> result;
            for (Record<ESM::Cell>& v : records)
                if (!v.mDeleted)
                    result.emplace_back(std::move(v.mValue));
            return result;
        }
    }

    EsmData loadEsmData(const Query& query, const std::vector<std::string>& contentFiles,
        const Files::Collections& fileCollections, std::vector<ESM::ESMReader>& readers, ToUTF8::Utf8Encoder* encoder)
    {
        Log(Debug::Info) << "Loading ESM data...";

        ShallowContent content = shallowLoad(query, contentFiles, fileCollections, readers, encoder);

        std::ostringstream loaded;

        if (query.mLoadActivators)
            loaded << ' ' << content.mActivators.size() << " activators,";
        if (query.mLoadCells)
            loaded << ' ' << content.mCells.mValues.size() << " cells,";
        if (query.mLoadContainers)
            loaded << ' ' << content.mContainers.size() << " containers,";
        if (query.mLoadDoors)
            loaded << ' ' << content.mDoors.size() << " doors,";
        if (query.mLoadGameSettings)
            loaded << ' ' << content.mGameSettings.size() << " game settings,";
        if (query.mLoadLands)
            loaded << ' ' << content.mLands.size() << " lands,";
        if (query.mLoadStatics)
            loaded << ' ' << content.mStatics.size() << " statics,";

        Log(Debug::Info) << "Loaded" << loaded.str();

        EsmData result;

        if (query.mLoadActivators)
            result.mActivators = prepareRecords(content.mActivators, GetKey {});
        if (query.mLoadCells)
            result.mCells = prepareCellRecords(content.mCells.mValues);
        if (query.mLoadContainers)
            result.mContainers = prepareRecords(content.mContainers, GetKey {});
        if (query.mLoadDoors)
            result.mDoors = prepareRecords(content.mDoors, GetKey {});
        if (query.mLoadGameSettings)
            result.mGameSettings = prepareRecords(content.mGameSettings, GetKey {});
        if (query.mLoadLands)
            result.mLands = prepareRecords(content.mLands, GetKey {});
        if (query.mLoadStatics)
            result.mStatics = prepareRecords(content.mStatics, GetKey {});

        addRefIdsTypes(result);

        std::ostringstream prepared;

        if (query.mLoadActivators)
            prepared << ' ' << result.mActivators.size() << " unique activators,";
        if (query.mLoadCells)
            prepared << ' ' << result.mCells.size() << " unique cells,";
        if (query.mLoadContainers)
            prepared << ' ' << result.mContainers.size() << " unique containers,";
        if (query.mLoadDoors)
            prepared << ' ' << result.mDoors.size() << " unique doors,";
        if (query.mLoadGameSettings)
            prepared << ' ' << result.mGameSettings.size() << " unique game settings,";
        if (query.mLoadLands)
            prepared << ' ' << result.mLands.size() << " unique lands,";
        if (query.mLoadStatics)
            prepared << ' ' << result.mStatics.size() << " unique statics,";

        Log(Debug::Info) << "Prepared" << prepared.str();

        return result;
    }
}
