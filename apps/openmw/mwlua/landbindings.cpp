#include "landbindings.hpp"

#include <apps/openmw/mwlua/object.hpp>
#include <apps/openmw/mwworld/cellstore.hpp>
#include <apps/openmw/mwworld/worldmodel.hpp>

#include <components/esm/util.hpp>
#include <components/esmterrain/storage.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"

static const ESM::RefId worldspaceAt(const osg::Vec3f& pos, sol::object cellOrName)
{
    ESM::RefId worldspace;
    if (cellOrName.is<MWLua::GCell>())
        worldspace = cellOrName.as<MWLua::GCell>().mStore->getCell()->getWorldSpace();
    else if (cellOrName.is<std::string_view>() && !cellOrName.as<std::string_view>().empty())
        worldspace = MWBase::Environment::get()
                         .getWorldModel()
                         ->getCell(cellOrName.as<std::string_view>())
                         .getCell()
                         ->getWorldSpace();
    else
        worldspace = ESM::Cell::sDefaultWorldspaceId;

    return worldspace;
}

static bool fillLandData(const MWWorld::Store<ESM::Land>* landStore, const osg::Vec3f& pos, const float cellSize,
    const ESM::Land** land, const ESM::Land::LandData** landData)
{
    int cellX = static_cast<int>(std::floor(pos.x() / cellSize));
    int cellY = static_cast<int>(std::floor(pos.y() / cellSize));

    *land = landStore->search(cellX, cellY);

    if (*land != nullptr)
        *landData = (*land)->getLandData(ESM::Land::DATA_VTEX);

    // If we fail to preload land data, return, we need to be able to get *any* land to know how to correct
    // the position used to sample terrain
    if (*landData == nullptr)
        return false;

    return true;
}

namespace MWLua
{
    sol::table initCoreLandBindings(const Context& context)
    {
        auto lua = context.sol();
        sol::table landApi(lua, sol::create);

        landApi["getHeightAt"] = [](const osg::Vec3f& pos, sol::object cellOrName) {
            auto worldspace = worldspaceAt(pos, cellOrName);
            return MWBase::Environment::get().getWorld()->getTerrainHeightAt(pos, worldspace);
        };

        landApi["getTextureAt"] = [lua = lua](const osg::Vec3f& pos, sol::object cellOrName) {
            sol::variadic_results values;
            auto store = MWBase::Environment::get().getESMStore();
            auto landStore = store->get<ESM::Land>();

            const float cellSize = ESM::getCellSize(worldspaceAt(pos, cellOrName));
            // We need to read land twice. Once to get the amount of texture samples per cell edge, and the second time
            // to get the actual data
            // This is because the visual land textures are offset with regards to quads that are rendered for terrain.
            // To properly calculate that offset, we need to know how many texture samples exist per cell edge,
            // as it differs between tes3 and tes4. It's equal -
            // Once we know the value, we will calculate the offset and retrieve a sample again, this time
            // with the offset taken into account.
            const ESM::Land* land = nullptr;
            const ESM::Land::LandData* landData = nullptr;

            if (!fillLandData(&landStore, pos, cellSize, &land, &landData))
                return values;

            // Use landData to get amount of sampler per cell edge (sLandTextureSize)
            // and then get the corrected position that will map to the rendered texture
            const osg::Vec3f correctedPos
                = ESMTerrain::Storage::getTextureCorrectedWorldPos(pos, landData->sLandTextureSize, cellSize);

            const ESM::Land* correctedLand = nullptr;
            const ESM::Land::LandData* correctedLandData = nullptr;

            if (!fillLandData(&landStore, correctedPos, cellSize, &correctedLand, &correctedLandData))
                return values;

            // We're passing in sLandTextureSize, NOT sLandSize like with getHeightAt
            const ESMTerrain::UniqueTextureId textureId
                = ESMTerrain::Storage::getTextureAt(correctedLandData->mTextures, correctedLand->getPlugin(),
                    correctedLandData->sLandTextureSize, correctedPos, cellSize);

            // Need to check for 0, 0 so that we can safely subtract 1 later, as per documentation on UniqueTextureId
            if (textureId.first != 0)
            {
                auto store = MWBase::Environment::get().getESMStore();
                auto textureStore = store->get<ESM::LandTexture>();
                const std::string* textureString = textureStore.search(textureId.first - 1, textureId.second);
                if (textureString)
                    values.push_back(sol::make_object<std::string>(lua, *textureString));
                else
                    return values;

                const std::vector<std::string>& contentList = MWBase::Environment::get().getWorld()->getContentFiles();
                if (textureId.second > 0 && textureId.second < contentList.size())
                    values.push_back(sol::make_object<std::string>(lua, contentList[textureId.second]));
            }

            return values;
        };

        return LuaUtil::makeReadOnly(landApi);
    }
}
