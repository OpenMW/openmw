#include "landbindings.hpp"

#include <apps/openmw/mwlua/object.hpp>
#include <apps/openmw/mwworld/cellstore.hpp>
#include <apps/openmw/mwworld/worldmodel.hpp>

#include <components/esm/util.hpp>
#include <components/esmterrain/storage.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"

namespace
{
    osg::Vec3f getTextureCorrectedWorldPos(
        const osg::Vec3f& uncorrectedWorldPos, const int textureSize, const float cellSize)
    {
        // the offset is [-0.25, +0.25] of a single texture's size
        // TODO: verify whether or not this works in TES4 and beyond
        float offset = (cellSize / textureSize) * 0.25;
        return uncorrectedWorldPos + osg::Vec3f{ -offset, +offset, 0.0f };
    }

    // Takes in a corrected world pos to match the visuals.
    ESMTerrain::UniqueTextureId getTextureAt(const std::span<const std::uint16_t> landData, const int plugin,
        const int textureSize, const osg::Vec3f& correctedWorldPos, const float cellSize)
    {
        int cellX = static_cast<int>(std::floor(correctedWorldPos.x() / cellSize));
        int cellY = static_cast<int>(std::floor(correctedWorldPos.y() / cellSize));

        // Normalized position in the cell
        float nX = (correctedWorldPos.x() - (cellX * cellSize)) / cellSize;
        float nY = (correctedWorldPos.y() - (cellY * cellSize)) / cellSize;

        int startX = static_cast<int>(nX * textureSize);
        int startY = static_cast<int>(nY * textureSize);

        assert(startX < ESM::Land::LAND_TEXTURE_SIZE);
        assert(startY < ESM::Land::LAND_TEXTURE_SIZE);

        const std::uint16_t tex = landData[startY * ESM::Land::LAND_TEXTURE_SIZE + startX];
        if (tex == 0)
            return { 0, 0 }; // vtex 0 is always the base texture, regardless of plugin

        return { tex, plugin };
    }

    const ESM::RefId worldspaceAt(sol::object cellOrName)
    {
        const MWWorld::Cell* cell = nullptr;
        if (cellOrName.is<MWLua::GCell>())
            cell = cellOrName.as<MWLua::GCell>().mStore->getCell();
        else if (cellOrName.is<MWLua::LCell>())
            cell = cellOrName.as<MWLua::LCell>().mStore->getCell();
        else if (cellOrName.is<std::string_view>() && !cellOrName.as<std::string_view>().empty())
            cell = MWBase::Environment::get().getWorldModel()->getCell(cellOrName.as<std::string_view>()).getCell();
        if (cell == nullptr)
            throw std::runtime_error("Invalid cell");
        else if (!cell->isExterior())
            throw std::runtime_error("Cell cannot be interior");

        return cell->getWorldSpace();
    }

    bool fillLandData(const MWWorld::Store<ESM::Land>& landStore, const osg::Vec3f& pos, const float cellSize,
        const ESM::Land*& land, const ESM::Land::LandData*& landData)
    {
        int cellX = static_cast<int>(std::floor(pos.x() / cellSize));
        int cellY = static_cast<int>(std::floor(pos.y() / cellSize));

        land = landStore.search(cellX, cellY);

        if (land != nullptr)
            landData = land->getLandData(ESM::Land::DATA_VTEX);

        // If we fail to preload land data, return, we need to be able to get *any* land to know how to correct
        // the position used to sample terrain
        if (landData == nullptr)
            return false;

        return true;
    }
}

namespace MWLua
{
    sol::table initCoreLandBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table landApi(lua, sol::create);

        landApi["getHeightAt"] = [](const osg::Vec3f& pos, sol::object cellOrName) {
            ESM::RefId worldspace = worldspaceAt(cellOrName);
            return MWBase::Environment::get().getWorld()->getTerrainHeightAt(pos, worldspace);
        };

        landApi["getTextureAt"] = [lua = lua](const osg::Vec3f& pos, sol::object cellOrName) {
            sol::variadic_results values;
            MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
            const MWWorld::Store<ESM::Land>& landStore = store.get<ESM::Land>();

            const float cellSize = ESM::getCellSize(worldspaceAt(cellOrName));
            // We need to read land twice. Once to get the amount of texture samples per cell edge, and the second time
            // to get the actual data
            // This is because the visual land textures are offset with regards to quads that are rendered for terrain.
            // To properly calculate that offset, we need to know how many texture samples exist per cell edge,
            // as it differs between tes3 and tes4. It's equal -
            // Once we know the value, we will calculate the offset and retrieve a sample again, this time
            // with the offset taken into account.
            const ESM::Land* land = nullptr;
            const ESM::Land::LandData* landData = nullptr;

            if (!fillLandData(landStore, pos, cellSize, land, landData))
                return values;

            // Use landData to get amount of sampler per cell edge (sLandTextureSize)
            // and then get the corrected position that will map to the rendered texture
            const osg::Vec3f correctedPos = getTextureCorrectedWorldPos(pos, landData->sLandTextureSize, cellSize);

            const ESM::Land* correctedLand = nullptr;
            const ESM::Land::LandData* correctedLandData = nullptr;

            if (!fillLandData(landStore, correctedPos, cellSize, correctedLand, correctedLandData))
                return values;

            // We're passing in sLandTextureSize, NOT sLandSize like with getHeightAt
            const ESMTerrain::UniqueTextureId textureId = getTextureAt(correctedLandData->mTextures,
                correctedLand->getPlugin(), correctedLandData->sLandTextureSize, correctedPos, cellSize);

            // Need to check for 0, 0 so that we can safely subtract 1 later, as per documentation on UniqueTextureId
            if (textureId.first != 0)
            {
                const MWWorld::Store<ESM::LandTexture>& textureStore = store.get<ESM::LandTexture>();
                const std::string* textureString = textureStore.search(textureId.first - 1, textureId.second);
                if (!textureString)
                    return values;

                values.push_back(sol::make_object<std::string>(lua, *textureString));
                const std::vector<std::string>& contentList = MWBase::Environment::get().getWorld()->getContentFiles();
                if (textureId.second >= 0 && static_cast<size_t>(textureId.second) < contentList.size())
                    values.push_back(sol::make_object<std::string>(lua, contentList[textureId.second]));
            }

            return values;
        };

        return LuaUtil::makeReadOnly(landApi);
    }
}
