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
    class WorldspaceWidget;

    enum class TerrainSelectionType
    {
        Texture,
        Shape
    };

    /// \brief Class handling the terrain selection data and rendering
    class TerrainSelection
    {
        public:

            TerrainSelection(osg::Group* parentNode, WorldspaceWidget *worldspaceWidget, TerrainSelectionType type);
            ~TerrainSelection();

            void onlySelect(const std::vector<std::pair<int, int>> &localPositions);
            void addSelect(const std::pair<int, int> &localPos);
            void toggleSelect(const std::vector<std::pair<int, int>> &localPositions, bool toggleInProgress);

            void activate();
            void deactivate();

            std::vector<std::pair<int, int>> getTerrainSelection() const;

            void update();

        protected:

            void drawShapeSelection(const osg::ref_ptr<osg::Vec3Array> vertices);
            void drawTextureSelection(const osg::ref_ptr<osg::Vec3Array> vertices);

            int calculateLandHeight(int x, int y);

        private:

            bool noCell(const std::string& cellId);

            bool noLand(const std::string& cellId);

            bool noLandLoaded(const std::string& cellId);

            bool isLandLoaded(const std::string& cellId);
            
            osg::Group* mParentNode;
            WorldspaceWidget *mWorldspaceWidget;
            osg::ref_ptr<osg::PositionAttitudeTransform> mBaseNode;
            osg::ref_ptr<osg::Geometry> mGeometry;
            osg::ref_ptr<osg::Group> mSelectionNode;
            std::vector<std::pair<int, int>> mSelection; // Global terrain selection coordinate in either vertex or texture units
            std::vector<std::pair<int, int>> mTemporarySelection; // Used during toggle to compare the most recent drag operation
            bool mDraggedOperationFlag; //true during drag operation, false when click-operation
            TerrainSelectionType mSelectionType;
    };
}

#endif
