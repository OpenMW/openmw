#ifndef CSV_RENDER_BRUSHDRAW_H
#define CSV_RENDER_BRUSHDRAW_H

#include <osg/Group>
#include <osg/Geometry>

#include <components/esm/loadland.hpp>
#include "../widget/brushshapes.hpp"

namespace CSVRender
{
    class BrushDraw
    {
        public:
            BrushDraw(osg::ref_ptr<osg::Group> parentNode, bool textureMode = false);
            ~BrushDraw();

            void update(osg::Vec3d point, int brushSize, CSVWidget::BrushShape toolShape);
            void hide();

        private:
            void buildPointGeometry(const osg::Vec3d& point);
            void buildSquareGeometry(const float& radius, const osg::Vec3d& point);
            void buildCircleGeometry(const float& radius, const osg::Vec3d& point);
            void buildCustomGeometry(const float& radius, const osg::Vec3d& point);
            float getIntersectionHeight (const osg::Vec3d& point);

            osg::ref_ptr<osg::Group> mParentNode;
            osg::ref_ptr<osg::Group> mBrushDrawNode;
            osg::ref_ptr<osg::Geometry> mGeometry;
            bool mTextureMode;
            float mLandSizeFactor;
    };
}

#endif
