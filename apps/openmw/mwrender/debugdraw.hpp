#pragma once
#include <osg/Vec3f>
#include <osg/ref_ptr>
#include <vector>
#include "renderingmanager.hpp"

namespace osg
{
    class Group;
    class Geometry;
}

namespace MWRenderDebug
{
    static const osg::Vec3f colorWhite = osg::Vec3(1., 1., 1.);
    static const osg::Vec3f colorRed = osg::Vec3(1., 0., 0.);
    static const osg::Vec3f colorBlue = osg::Vec3(0., 0., 1.);
    static const osg::Vec3f colorGreen = osg::Vec3(0., 1., 0.);
    static const osg::Vec3f colorBlack = osg::Vec3(0., 0., 0.);
    static const osg::Vec3f colorDarkGrey= osg::Vec3(0.25, 0.25, 0.25);


    struct CubeDraw
    {
        osg::Vec3f mDims;
        osg::Vec3f mColor;

        osg::Vec3f mPosition;
    };

    class CubeCustomDraw : public osg::Drawable
    {
    public:
        CubeCustomDraw(osg::ref_ptr<osg::Geometry>&  CubeGeometry, std::vector<CubeDraw>& cubesToDraw,std::mutex& mutex) : mCubeGeometry( CubeGeometry), mCubesToDraw(cubesToDraw),mDrawCallMutex(mutex) {}

        std::vector<CubeDraw>& mCubesToDraw;

        std::mutex& mDrawCallMutex;

        osg::ref_ptr<osg::Geometry>  mCubeGeometry;
        virtual osg::BoundingSphere computeBound() const
        {
            return osg::BoundingSphere();
        }

        virtual void drawImplementation(osg::RenderInfo&) const;

    };

    struct DebugDrawer
    {
        DebugDrawer(MWRender::RenderingManager& manager,osg::ref_ptr<osg::Group> parentNode);

        void update();



        std::vector<CubeDraw> mCubesToDrawRead;
        std::vector<CubeDraw> mCubesToDrawWrite;
        std::mutex mDrawCallMutex;

        osg::ref_ptr<CubeCustomDraw> mcustomCubesDrawer;
        osg::ref_ptr<osg::Geometry>  mCubeGeometry;
        void drawCube(osg::Vec3f mPosition, osg::Vec3f mDims = osg::Vec3(50.,50.,50.), osg::Vec3f mColor = osg::Vec3(1.,1.,1.));
    };
}
