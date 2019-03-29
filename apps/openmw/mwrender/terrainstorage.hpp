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

        TerrainStorage(Resource::ResourceSystem* resourceSystem, const std::string& normalMapPattern = "", const std::string& normalHeightMapPatteern = "", bool autoUseNormalMaps = false, const std::string& specularMapPattern = "", bool autoUseSpecularMaps = false);
        ~TerrainStorage();

        virtual osg::ref_ptr<const ESMTerrain::LandObject> getLand (int cellX, int cellY) override;
        virtual const ESM::LandTexture* getLandTexture(int index, short plugin) override;

        virtual bool hasData(int cellX, int cellY) override;

        /// Get bounds of the whole terrain in cell units
        virtual void getBounds(float& minX, float& maxX, float& minY, float& maxY) override;

        LandManager* getLandManager() const;

    private:
       std::unique_ptr<LandManager> mLandManager;

       Resource::ResourceSystem* mResourceSystem;
    };

}


#endif
