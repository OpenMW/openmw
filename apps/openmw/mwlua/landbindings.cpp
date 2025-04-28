#include "landbindings.hpp"

#include <apps/openmw/mwlua/object.hpp>
#include <apps/openmw/mwworld/cellstore.hpp>
#include <apps/openmw/mwworld/worldmodel.hpp>

#include <components/esmterrain/storage.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWLua
{
    sol::table initCoreLandBindings(const Context& context)
    {
        sol::state_view& lua = context.mLua->sol();
        sol::table landApi(lua, sol::create);

        // Constants
        landApi["RANGE"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, ESM::RangeType>({
            { "Self", ESM::RT_Self },
            { "Touch", ESM::RT_Touch },
            { "Target", ESM::RT_Target },
        }));

        landApi["getHeightAt"] = [](const osg::Vec3f& pos, sol::object cellOrName) {
            ESM::RefId worldspace;
            if (cellOrName.is<GCell>())
                worldspace = cellOrName.as<GCell>().mStore->getCell()->getWorldSpace();
            else if (cellOrName.is<std::string_view>() && !cellOrName.as<std::string_view>().empty())
                worldspace = MWBase::Environment::get()
                                 .getWorldModel()
                                 ->getCell(cellOrName.as<std::string_view>())
                                 .getCell()
                                 ->getWorldSpace();
            else
                worldspace = ESM::Cell::sDefaultWorldspaceId;

            const float cellSize = ESM::getCellSize(worldspace);
            int cellX = static_cast<int>(std::floor(pos.x() / cellSize));
            int cellY = static_cast<int>(std::floor(pos.y() / cellSize));

            auto store = MWBase::Environment::get().getESMStore();
            auto landStore = store->get<ESM::Land>();
            auto land = landStore.search(cellX, cellY);
            const ESM::Land::LandData* landData = nullptr;
            if (land != nullptr)
            {
                landData = land->getLandData(ESM::Land::DATA_VHGT);
                if (landData != nullptr)
                {
                    // Ensure data is loaded if necessary
                    land->loadData(ESM::Land::DATA_VHGT);
                    landData = land->getLandData(ESM::Land::DATA_VHGT);
                }
            }
            if (landData == nullptr)
            {
                // If we failed to load data, return the default height
                return static_cast<float>(ESM::Land::DEFAULT_HEIGHT);
            }
            return ESMTerrain::Storage::getHeightAt(landData->mHeights, landData->sLandSize, pos, cellSize);
        };

        landApi["getLandTextureAt"] = [lua = context.mLua](const osg::Vec3f& pos, sol::object cellOrName) {
            sol::variadic_results values;
            ESM::RefId worldspace;
            if (cellOrName.is<GCell>())
                worldspace = cellOrName.as<GCell>().mStore->getCell()->getWorldSpace();
            else if (cellOrName.is<std::string_view>() && !cellOrName.as<std::string_view>().empty())
                worldspace = MWBase::Environment::get()
                                 .getWorldModel()
                                 ->getCell(cellOrName.as<std::string_view>())
                                 .getCell()
                                 ->getWorldSpace();
            else
                worldspace = ESM::Cell::sDefaultWorldspaceId;

            const float cellSize = ESM::getCellSize(worldspace);

            int cellX = static_cast<int>(std::floor(pos.x() / cellSize));
            int cellY = static_cast<int>(std::floor(pos.y() / cellSize));

            auto store = MWBase::Environment::get().getESMStore();
            // We need to read land twice. Once to get the amount of texture samples per cell edge, and the second time
            // to get the actual data
            auto landStore = store->get<ESM::Land>();
            auto land = landStore.search(cellX, cellY);
            const ESM::Land::LandData* landData = nullptr;
            if (land != nullptr)
            {
                landData = land->getLandData(ESM::Land::DATA_VTEX);
                if (landData != nullptr)
                {
                    // Ensure data is loaded if necessary
                    land->loadData(ESM::Land::DATA_VTEX);
                    landData = land->getLandData(ESM::Land::DATA_VTEX);
                }
            }
            if (landData == nullptr)
            {
                // If we fail to preload land data, return, we need to be able to get *any* land to know how to correct
                // the position used to sample terrain
                return values;
            }

            const osg::Vec3f correctedPos
                = ESMTerrain::Storage::getTextureCorrectedWorldPos(pos, landData->sLandTextureSize, cellSize);
            int correctedCellX = static_cast<int>(std::floor(correctedPos.x() / cellSize));
            int correctedCellY = static_cast<int>(std::floor(correctedPos.y() / cellSize));
            auto correctedLand = landStore.search(correctedCellX, correctedCellY);
            const ESM::Land::LandData* correctedLandData = nullptr;
            if (correctedLand != nullptr)
            {
                correctedLandData = correctedLand->getLandData(ESM::Land::DATA_VTEX);
                if (correctedLandData != nullptr)
                {
                    // Ensure data is loaded if necessary
                    land->loadData(ESM::Land::DATA_VTEX);
                    correctedLandData = correctedLand->getLandData(ESM::Land::DATA_VTEX);
                }
            }
            if (correctedLandData == nullptr)
            {
                return values;
            }

            // We're passing in sLandTextureSize, NOT sLandSize like with getHeightAt
            const ESMTerrain::UniqueTextureId textureId
                = ESMTerrain::Storage::getLandTextureAt(correctedLandData->mTextures, correctedLand->getPlugin(),
                    correctedLandData->sLandTextureSize, correctedPos, cellSize);

            // Need to check for 0, 0 so that we can safely subtract 1 later, as per documentation on UniqueTextureId
            if (textureId.first != 0)
            {
                values.push_back(sol::make_object<uint16_t>(lua->sol(), textureId.first - 1));
                values.push_back(sol::make_object<int>(lua->sol(), textureId.second));

                auto textureStore = store->get<ESM::LandTexture>();
                const std::string* textureString = textureStore.search(textureId.first - 1, textureId.second);
                if (textureString)
                {
                    values.push_back(sol::make_object<std::string>(lua->sol(), *textureString));
                }
            }

            return values;
        };

        return LuaUtil::makeReadOnly(landApi);
    }
}
