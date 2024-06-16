#ifndef MWRENDER_TERRAINSTORAGE_H
#define MWRENDER_TERRAINSTORAGE_H

#include <memory>

#include <components/esmterrain/storage.hpp>

#include <components/resource/resourcesystem.hpp>

namespace MWRender
{

    class LandManager;

    /// @brief Connects the ESM Store used in OpenMW with the ESMTerrain storage.
    class TerrainStorage : public ESMTerrain::Storage
    {
    public:
        TerrainStorage(Resource::ResourceSystem* resourceSystem, std::string_view normalMapPattern = {},
            std::string_view normalHeightMapPattern = {}, bool autoUseNormalMaps = false,
            std::string_view specularMapPattern = {}, bool autoUseSpecularMaps = false);
        ~TerrainStorage();

        osg::ref_ptr<const ESMTerrain::LandObject> getLand(ESM::ExteriorCellLocation cellLocation) override;
        const std::string* getLandTexture(std::uint16_t index, int plugin) override;

        bool hasData(ESM::ExteriorCellLocation cellLocation) override;

        /// Get bounds of the whole terrain in cell units
        void getBounds(float& minX, float& maxX, float& minY, float& maxY, ESM::RefId worldspace) override;

        LandManager* getLandManager() const;

    private:
        std::unique_ptr<LandManager> mLandManager;

        Resource::ResourceSystem* mResourceSystem;
    };

}

#endif
