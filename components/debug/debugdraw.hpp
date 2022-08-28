#ifndef  OPENMW_COMPONENTS_DEBUG_DEBUGDRAW_H
#define OPENMW_COMPONENTS_DEBUG_DEBUGDRAW_H

#include <osg/Vec3f>
#include <osg/ref_ptr>
#include <vector>

namespace osg
{
    class Group;
    class Geometry;
}
namespace Shader
{
    class ShaderManager;
}

namespace MWRenderDebug
{
    static const osg::Vec3f colorWhite = osg::Vec3(1., 1., 1.);
    static const osg::Vec3f colorRed = osg::Vec3(1., 0., 0.);
    static const osg::Vec3f colorBlue = osg::Vec3(0., 0., 1.);
    static const osg::Vec3f colorGreen = osg::Vec3(0., 1., 0.);
    static const osg::Vec3f colorBlack = osg::Vec3(0., 0., 0.);
    static const osg::Vec3f colorDarkGrey= osg::Vec3(0.25, 0.25, 0.25);

    enum class DrawShape
    {
        Cube,
        Cylinder,
        WireCube,
    };

    struct DrawCall
    {
        osg::Vec3f mPosition;
        osg::Vec3f mDims;
        osg::Vec3f mColor;

        DrawShape mDrawShape;

        static DrawCall cube(osg::Vec3f pos, osg::Vec3 dims = osg::Vec3(50., 50., 50.), osg::Vec3 color = colorWhite) { return { pos, dims, color, DrawShape::Cube}; }
        static DrawCall wireCube(osg::Vec3f pos, osg::Vec3 dims = osg::Vec3(50., 50., 50.), osg::Vec3 color = colorWhite) { return { pos, dims, color, DrawShape::WireCube}; }
        static DrawCall cylinder(osg::Vec3f pos, osg::Vec3 dims = osg::Vec3(50., 50., 50.), osg::Vec3 color = colorWhite) { return { pos, dims, color, DrawShape::Cylinder}; }
    };

    class DebugCustomDraw : public osg::Drawable
    {
    public:
        DebugCustomDraw( std::vector<DrawCall>& cubesToDraw,osg::ref_ptr<osg::Geometry>& linesToDraw ,std::mutex& mutex) : mShapsToDraw(cubesToDraw),mLinesToDraw(linesToDraw), mDrawCallMutex(mutex) {}

        std::vector<DrawCall>& mShapsToDraw;
        osg::ref_ptr<osg::Geometry>& mLinesToDraw;

        std::mutex& mDrawCallMutex;

        osg::ref_ptr<osg::Geometry>  mCubeGeometry;
        osg::ref_ptr<osg::Geometry>  mCylinderGeometry;
        osg::ref_ptr<osg::Geometry>  mWireCubeGeometry;

        virtual osg::BoundingSphere computeBound() const
        {
            return osg::BoundingSphere();
        }

        virtual void drawImplementation(osg::RenderInfo&) const;
    };

    struct DebugLines;

    struct DebugDrawer
    {
        DebugDrawer(Shader::ShaderManager& shaderManager,osg::ref_ptr<osg::Group> parentNode);
        ~DebugDrawer();

        void update();
        void drawCube(osg::Vec3f mPosition, osg::Vec3f mDims = osg::Vec3(50.,50.,50.), osg::Vec3f mColor = colorWhite);
        void drawCubeMinMax(osg::Vec3f min, osg::Vec3f max, osg::Vec3f mColor = colorWhite);
        void addDrawCall(const DrawCall& draw);
        void addLine(const osg::Vec3& start, const osg::Vec3& end, const osg::Vec3 color = colorWhite);

        private:

        std::unique_ptr<DebugLines> mDebugLines;

        std::vector<DrawCall> mShapesToDrawRead;
        std::vector<DrawCall> mShapesToDrawWrite;
        std::mutex mDrawCallMutex;

        osg::ref_ptr<DebugCustomDraw> mCustomDebugDrawer;
        osg::ref_ptr<osg::Group> mDebugDrawSceneObjects;
    };
}
#endif // ! 
