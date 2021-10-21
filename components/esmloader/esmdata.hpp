#ifndef OPENMW_COMPONENTS_ESMLOADER_ESMDATA_H
#define OPENMW_COMPONENTS_ESMLOADER_ESMDATA_H

#include <components/esm/defs.hpp>

#include <string_view>
#include <vector>

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
}

namespace EsmLoader
{
    struct RefIdWithType
    {
        std::string_view mId;
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

    std::string_view getModel(const EsmData& content, std::string_view refId, ESM::RecNameInts type);

    ESM::Variant getGameSetting(const std::vector<ESM::GameSetting>& records, std::string_view id);
}

#endif
