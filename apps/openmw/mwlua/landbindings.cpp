#include "landbindings.hpp"

#include <span>
#include <string>

#include <sol/object.hpp>
#include <sol/table.hpp>
#include <sol/variadic_results.hpp>

#include <components/esm/refid.hpp>
#include <components/esm/util.hpp>
#include <components/esmterrain/storage.hpp>
#include <components/lua/luastate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/worldmodel.hpp"

#include "context.hpp"
#include "object.hpp"

namespace
{
    // Takes in a corrected world pos to match the visuals.
    ESMTerrain::UniqueTextureId getTextureAt(const std::span<const std::uint16_t> landData, const int plugin,
        const osg::Vec3f& correctedWorldPos, const float cellSize)
    {
        int cellX = static_cast<int>(std::floor(correctedWorldPos.x() / cellSize));
        int cellY = static_cast<int>(std::floor(correctedWorldPos.y() / cellSize));

        // Normalized position in the cell
        float nX = (correctedWorldPos.x() - (cellX * cellSize)) / cellSize;
        float nY = (correctedWorldPos.y() - (cellY * cellSize)) / cellSize;

        int startX = static_cast<int>(nX * ESM::Land::LAND_TEXTURE_SIZE);
        int startY = static_cast<int>(nY * ESM::Land::LAND_TEXTURE_SIZE);

        assert(startX < ESM::Land::LAND_TEXTURE_SIZE);
        assert(startY < ESM::Land::LAND_TEXTURE_SIZE);

        const std::uint16_t tex = landData[startY * ESM::Land::LAND_TEXTURE_SIZE + startX];
        if (tex == 0)
            return { 0, 0 }; // vtex 0 is always the base texture, regardless of plugin

        return { tex, plugin };
    }

    const ESM::RefId worldspaceAt(sol::object cellOrId)
    {
        const MWWorld::Cell* cell = nullptr;
        if (cellOrId.is<MWLua::GCell>())
            cell = cellOrId.as<MWLua::GCell>().mStore->getCell();
        else if (cellOrId.is<MWLua::LCell>())
            cell = cellOrId.as<MWLua::LCell>().mStore->getCell();
        else if (cellOrId.is<std::string_view>() && !cellOrId.as<std::string_view>().empty())
            cell = MWBase::Environment::get()
                       .getWorldModel()
                       ->getCell(ESM::RefId::deserializeText(cellOrId.as<std::string_view>()))
                       .getCell();
        if (cell == nullptr)
            throw std::runtime_error("Invalid cell");
        else if (!cell->isExterior())
            throw std::runtime_error("Cell cannot be interior");

        return cell->getWorldSpace();
    }
}

namespace MWLua
{
    sol::table initCoreLandBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table landApi(lua, sol::create);

        landApi["getHeightAt"] = [](const osg::Vec3f& pos, sol::object cellOrId) {
            ESM::RefId worldspace = worldspaceAt(cellOrId);
            return MWBase::Environment::get().getWorld()->getTerrainHeightAt(pos, worldspace);
        };

        landApi["getTextureAt"] = [lua = lua](const osg::Vec3f& pos, sol::object cellOrId) {
            sol::variadic_results values;
            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
            const MWWorld::Store<ESM::Land>& landStore = store.get<ESM::Land>();
            ESM::RefId worldspace = worldspaceAt(cellOrId);

            if (worldspace != ESM::Cell::sDefaultWorldspaceId)
                return values;

            const int cellSize = ESM::getCellSize(worldspace);
            const float offset = (cellSize / ESM::LandRecordData::sLandTextureSize) * 0.25f;
            const osg::Vec3f correctedPos = pos + osg::Vec3f{ -offset, +offset, 0.0f };

            const ESM::Land* land = nullptr;
            const ESM::Land::LandData* landData = nullptr;

            int cellX = static_cast<int>(std::floor(correctedPos.x() / cellSize));
            int cellY = static_cast<int>(std::floor(correctedPos.y() / cellSize));

            land = landStore.search(cellX, cellY);

            if (land != nullptr)
                landData = land->getLandData(ESM::Land::DATA_VTEX);

            // If we fail to preload land data, return, we need to be able to get *any* land to know how to correct
            // the position used to sample terrain
            if (landData == nullptr)
                return values;

            const ESMTerrain::UniqueTextureId textureId
                = getTextureAt(landData->mTextures, land->getPlugin(), correctedPos, static_cast<float>(cellSize));

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
