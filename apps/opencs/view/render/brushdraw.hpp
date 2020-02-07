#ifndef CSV_RENDER_BRUSHDRAW_H
#define CSV_RENDER_BRUSHDRAW_H

#include <osg/Group>
#include <osg/Geometry>

#include <components/esm/loadland.hpp>

namespace CSVRender
{
    class BrushDraw
    {
        public:
            BrushDraw(osg::Group* parentNode);
            ~BrushDraw();

            void update(osg::Vec3d point, int brushSize);
            void hide();

        private:
            void buildGeometry(const float& radius, const osg::Vec3d& point, int amountOfPoints);
            float getIntersectionHeight (const osg::Vec3d& point);

            osg::Group* mParentNode;
            osg::ref_ptr<osg::Group> mBrushDrawNode;
            osg::ref_ptr<osg::Geometry> mGeometry;
            float mLandSizeFactor = ESM::Land::REAL_SIZE / ESM::Land::LAND_SIZE / 2;
    };
}

#endif
