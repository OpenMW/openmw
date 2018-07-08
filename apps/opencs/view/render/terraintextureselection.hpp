#ifndef CSV_RENDER_TERRAINTEXTURESELECTION_H
#define CSV_RENDER_TERRAINTEXTURESELECTION_H

#include <osg/Geometry>

#include "terrainselection.hpp"

namespace CSVRender
{
    class TerrainTextureSelection : public TerrainSelection
    {
        public:

            TerrainTextureSelection(osg::Group* parentNode, const CSMWorld::CellCoordinates&, const ESM::Land&);

        protected:

            void addToSelection(osg::Vec3d worldPos) override;
            void toggleSelection(osg::Vec3d worldPos) override;
            void deselect() override;

            void update() override;
            int calculateLandHeight(int x, int y);

        private:

            osg::ref_ptr<osg::Geode> mGeode;
            std::vector<std::pair<int, int>> mSelection;
    };
}

#endif
