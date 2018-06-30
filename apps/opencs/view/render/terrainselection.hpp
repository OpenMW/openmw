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

            TerrainSelection(const CSMWorld::CellCoordinates&, const ESM::Land&, osg::Group* parentNode);

            TerrainSelection(const TerrainSelection&) = delete;
            TerrainSelection& operator=(const TerrainSelection&) = delete;

            TerrainSelection(TerrainSelection&&) = delete;
            TerrainSelection& operator=(TerrainSelection&&) = delete;

            virtual ~TerrainSelection();

            void select(const WorldspaceHitResult&);
            void toggleSelect(const WorldspaceHitResult&);

            void activate();
            void deactivate();

        protected:

            using TerrainPos = std::pair<int, int>;
            using Container = std::vector<TerrainPos>;

            // these functions need better names
            virtual void addToSelection(osg::Vec3d worldPos) = 0;
            virtual void toggleSelection(osg::Vec3d worldPos) = 0;
            virtual void deselect() = 0;

            virtual void update() = 0;

            TerrainPos toTextureCoords(osg::Vec3d worldPos) const;
            TerrainPos toVertexCoords(osg::Vec3d worldPos) const;

            const ESM::Land::LandData* getLandData() const;
            const osg::ref_ptr<osg::PositionAttitudeTransform>& getBaseNode() const;

            static double toWorldCoords(int);

            static size_t landIndex(int x, int y);

        private:

            bool isInCell(const WorldspaceHitResult&) const;

            std::pair<double, double> toCellCoords(osg::Vec3d worldPos) const;

            CSMWorld::CellCoordinates mCoords;
            const ESM::Land& mEsmLand;
            osg::Group* mParentNode;
            osg::ref_ptr<osg::PositionAttitudeTransform> mBaseNode;
    };
}

#endif
