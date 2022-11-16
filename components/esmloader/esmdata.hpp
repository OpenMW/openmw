#ifndef OPENMW_COMPONENTS_ESMLOADER_ESMDATA_H
#define OPENMW_COMPONENTS_ESMLOADER_ESMDATA_H

#include <string_view>
#include <vector>

#include <components/esm/defs.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/loadacti.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loaddoor.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm3/loadstat.hpp>

namespace ESM
{
    struct Activator;
    struct Cell;
    struct Container;
    struct Door;
    struct GameSetting;
    struct Land;
    struct Static;
    class Variant;
    struct RefId;
}

namespace EsmLoader
{
    struct RefIdWithType
    {
        ESM::RefId mId;
        ESM::RecNameInts mType;
    };

    struct EsmData
    {
        std::vector<ESM::Activator> mActivators;
        std::vector<ESM::Cell> mCells;
        std::vector<ESM::Container> mContainers;
        std::vector<ESM::Door> mDoors;
        std::vector<ESM::GameSetting> mGameSettings;
        std::vector<ESM::Land> mLands;
        std::vector<ESM::Static> mStatics;
        std::vector<RefIdWithType> mRefIdTypes;

        EsmData() = default;
        EsmData(const EsmData&) = delete;
        EsmData(EsmData&&) = default;

        ~EsmData();
    };

    std::string_view getModel(const EsmData& content, const ESM::RefId& refId, ESM::RecNameInts type);

    ESM::Variant getGameSetting(const std::vector<ESM::GameSetting>& records, const ESM::RefId& id);
}

#endif
