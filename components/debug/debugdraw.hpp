#ifndef OPENMW_COMPONENTS_DEBUG_DEBUGDRAW_H
#define OPENMW_COMPONENTS_DEBUG_DEBUGDRAW_H

#include <osg/Drawable>
#include <osg/Vec3>
#include <osg/ref_ptr>

#include <array>
#include <memory>
#include <vector>

namespace osg
{
    class Group;
    class Geometry;
    class Geometry;
    class RenderInfo;
}
namespace Shader
{
    class ShaderManager;
}

namespace Debug
{
    static const osg::Vec3f colorWhite = osg::Vec3(1., 1., 1.);
    static const osg::Vec3f colorRed = osg::Vec3(1., 0., 0.);
    static const osg::Vec3f colorBlue = osg::Vec3(0., 0., 1.);
    static const osg::Vec3f colorGreen = osg::Vec3(0., 1., 0.);
    static const osg::Vec3f colorMagenta = osg::Vec3(1., 0., 1.);
    static const osg::Vec3f colorYellow = osg::Vec3(1., 1., 0.);
    static const osg::Vec3f colorCyan = osg::Vec3(0., 1., 1.);
    static const osg::Vec3f colorBlack = osg::Vec3(0., 0., 0.);
    static const osg::Vec3f colorDarkGrey = osg::Vec3(0.25, 0.25, 0.25);

    class DebugDrawCallback;

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

        static DrawCall cube(osg::Vec3f pos, osg::Vec3 dims = osg::Vec3(50., 50., 50.), osg::Vec3 color = colorWhite)
        {
            return { pos, dims, color, DrawShape::Cube };
        }
        static DrawCall wireCube(
            osg::Vec3f pos, osg::Vec3 dims = osg::Vec3(50., 50., 50.), osg::Vec3 color = colorWhite)
        {
            return { pos, dims, color, DrawShape::WireCube };
        }
        static DrawCall cylinder(
            osg::Vec3f pos, osg::Vec3 dims = osg::Vec3(50., 50., 50.), osg::Vec3 color = colorWhite)
        {
            return { pos, dims, color, DrawShape::Cylinder };
        }
    };

    class DebugCustomDraw : public osg::Drawable
    {
    public:
        DebugCustomDraw();
        DebugCustomDraw(const DebugCustomDraw& copy, const osg::CopyOp& copyop);

        META_Node(Debug, DebugCustomDraw)

        mutable std::vector<DrawCall> mShapesToDraw;
        osg::ref_ptr<osg::Geometry> mLinesToDraw;

        osg::ref_ptr<osg::Geometry> mCubeGeometry;
        osg::ref_ptr<osg::Geometry> mCylinderGeometry;
        osg::ref_ptr<osg::Geometry> mWireCubeGeometry;

        virtual void drawImplementation(osg::RenderInfo&) const override;
    };

    struct DebugDrawer : public osg::Node
    {
        DebugDrawer() = default;
        DebugDrawer(const DebugDrawer& copy, const osg::CopyOp& copyop);
        DebugDrawer(Shader::ShaderManager& shaderManager);

        META_Node(Debug, DebugDrawer)

        void traverse(osg::NodeVisitor& nv) override;

        void drawCube(
            osg::Vec3f mPosition, osg::Vec3f mDims = osg::Vec3(50., 50., 50.), osg::Vec3f mColor = colorWhite);
        void drawCubeMinMax(osg::Vec3f min, osg::Vec3f max, osg::Vec3f mColor = colorWhite);
        void addDrawCall(const DrawCall& draw);
        void addLine(const osg::Vec3& start, const osg::Vec3& end, const osg::Vec3 color = colorWhite);

    private:
        unsigned int mCurrentFrame = 0;

        std::array<osg::ref_ptr<DebugCustomDraw>, 2> mCustomDebugDrawer;
    };
}
#endif // !
