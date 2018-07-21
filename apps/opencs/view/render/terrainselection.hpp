#ifndef CSV_RENDER_TERRAINSELECTION_H
#define CSV_RENDER_TERRAINSELECTION_H

#include <utility>
#include <vector>

#include <osg/Vec3d>
#include <osg/ref_ptr>
#include <osg/PositionAttitudeTransform>

#include <components/esm/loadland.hpp>
#include "../../model/world/cellcoordinates.hpp"

namespace osg
{
    class Group;
}

namespace CSVRender
{
    struct WorldspaceHitResult;

    // should TerrainSelection know about its dependencies?
    enum class TerrainSelectionType {
        Texture
    };

    // class with functionality common to
    // different terrain selection types
    class TerrainSelection
    {
        public:

            TerrainSelection(osg::Group* parentNode);

            TerrainSelection(const TerrainSelection&) = delete;
            TerrainSelection& operator=(const TerrainSelection&) = delete;

            TerrainSelection(TerrainSelection&&) = delete;
            TerrainSelection& operator=(TerrainSelection&&) = delete;

            virtual ~TerrainSelection();

            void selectTerrainTexture(const WorldspaceHitResult&);
            void onlyAddSelect(const WorldspaceHitResult&);
            void toggleSelect(const WorldspaceHitResult&);

            void activate();
            void deactivate();

        protected:

            // these functions need better names
            void addToSelection(osg::Vec3d worldPos);
            void toggleSelection(osg::Vec3d worldPos);
            void deselect();

            void update();

            std::pair<int, int> toTextureCoords(osg::Vec3d worldPos) const;
            std::pair<int, int> toVertexCoords(osg::Vec3d worldPos) const;

            const osg::ref_ptr<osg::PositionAttitudeTransform>& getBaseNode() const;

            static double toWorldCoords(int);

            static size_t landIndex(int x, int y);

            int calculateLandHeight(int x, int y);

        private:

            bool isInCell(const WorldspaceHitResult&) const;

            std::pair<double, double> toCellCoords(osg::Vec3d worldPos) const;

            CSMWorld::CellCoordinates mCoords;
            osg::Group* mParentNode;
            osg::ref_ptr<osg::PositionAttitudeTransform> mBaseNode;

            osg::ref_ptr<osg::Geode> mGeode;
            std::vector<std::pair<int, int>> mSelection; // Global cell_selection coordinate in either vertex or texture units
    };
}

#endif
