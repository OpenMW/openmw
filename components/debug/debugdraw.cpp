#include "debugdraw.hpp"
#include <components/sceneutil/nodecallback.hpp>
#include <components/shader/shadermanager.hpp>

#include <osg/Array>
#include <osg/Drawable>
#include <osg/GLExtensions>
#include <osg/Geometry>
#include <osg/Program>
#include <osg/Uniform>
#include <osg/Vec3>

static osg::Vec3 sphereCoordToCartesian(float theta, float phi, float r)
{
    osg::Vec3 returnVec = osg::Vec3(0.0, 0.0, 0.0);
    float phiToHorizontal = osg::PI_2 - phi;
    returnVec.x() = std::cos(theta);
    returnVec.y() = std::sin(theta);
    returnVec.z() = std::sin(phiToHorizontal);

    returnVec.x() *= std::cos(phiToHorizontal);
    returnVec.y() *= std::cos(phiToHorizontal);
    returnVec.x() *= r;
    returnVec.z() *= r;

    returnVec.y() *= r;
    return returnVec;
}

static void generateWireCube(osg::Geometry& geom, float dim)
{
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;

    osg::Vec2i indexPos[] = { osg::Vec2i(0, 0), osg::Vec2i(1, 0), osg::Vec2i(1, 1), osg::Vec2i(0, 1) };

    for (int i = 0; i < 4; i++)
    {
        osg::Vec3 vert1 = osg::Vec3(indexPos[i].x() - 0.5, indexPos[i].y() - 0.5, 0.5);
        int next = (i + 1) % 4;
        osg::Vec3 vert2 = osg::Vec3(indexPos[next].x() - 0.5, indexPos[next].y() - 0.5, 0.5);

        vertices->push_back(vert1 * dim);
        vertices->push_back(vert2 * dim);
        vert1.z() *= -1;
        vert2.z() *= -1;
        vertices->push_back(vert1 * dim);
        vertices->push_back(vert2 * dim);

        auto vert3 = vert1;
        vert3.z() *= -1;
        vertices->push_back(vert1 * dim);
        vertices->push_back(vert3 * dim);
    }
    for (std::size_t i = 0; i < vertices->size(); i++)
    {
        normals->push_back(osg::Vec3(1., 1., 1.));
    }

    geom.setVertexArray(vertices);
    geom.setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
    geom.addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size()));
}

static void generateCube(osg::Geometry& geom, float dim)
{
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUShort> indices = new osg::DrawElementsUShort(osg::DrawElementsUShort::TRIANGLES, 0);

    for (int i_face = 0; i_face < 6; i_face++)
    {
        osg::Vec3f normale(0., 0., 0.);
        osg::Vec3f u(0., 0., 0.);
        osg::Vec3f v(0., 0., 0.);
        int axis = i_face / 2;
        int dir = i_face % 2 == 0 ? -1 : 1;
        float float_dir = dir;
        normale[axis] = float_dir;
        u[(axis + 1) % 3] = 1.0;
        v[(axis + 2) % 3] = 1.0;

        for (int i_point = 0; i_point < 4; i_point++)
        {
            float iu = i_point % 2 == 1
                ? float_dir
                : -float_dir; // This is to get the right triangle orientation when the normal changes*
            float iv = i_point / 2 == 1 ? 1.0 : -1.0;
            osg::Vec3f point = (u * iu) + (v * iv);
            point = (point + normale);
            point = point * (dim * 0.5f);
            vertices->push_back(point);
            normals->push_back(normale);
        }
        int start_vertex(i_face * 4);
        int newFace1[] = { start_vertex, start_vertex + 1, start_vertex + 2 };
        for (int i = 0; i < 3; i++)
        {
            indices->push_back(newFace1[i]);
        }
        int newFace2[] = { start_vertex + 2, start_vertex + 1, start_vertex + 3 };
        for (int i = 0; i < 3; i++)
        {
            indices->push_back(newFace2[i]);
        }
    }
    geom.setVertexArray(vertices);
    geom.setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
    geom.addPrimitiveSet(indices);
}

static void generateCylinder(osg::Geometry& geom, float radius, float height, int subdiv)
{
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUShort> indices = new osg::DrawElementsUShort(osg::DrawElementsUShort::TRIANGLES, 0);
    int vertexCount = subdiv * 4 + 2; // 2 discs + top and bottom + 2 center
    indices->reserve(vertexCount);
    int iVertex = 0;

    int beginTop = iVertex;
    auto topNormal = osg::Vec3(0., 0., 1.);
    // top disk
    for (int i = 0; i < subdiv; i++)
    {
        float theta = (float(i) / float(subdiv)) * osg::PI * 2.;
        osg::Vec3 pos = sphereCoordToCartesian(theta, osg::PI_2f, 1.);
        pos *= radius;
        pos.z() = height / 2.;
        vertices->push_back(pos);
        normals->push_back(topNormal);
        iVertex += 1;
    }
    auto centerTop = iVertex;
    // centerTop
    {
        vertices->push_back(osg::Vec3(0., 0., height / 2.));
        normals->push_back(topNormal);
        iVertex += 1;
    }
    auto centerBot = iVertex;
    // centerBot
    {
        vertices->push_back(osg::Vec3(0., 0., -height / 2));
        normals->push_back(-topNormal);
        iVertex += 1;
    }
    // bottom disk
    auto begin_bot = iVertex;
    for (int i = 0; i < subdiv; i++)
    {
        float theta = float(i) / float(subdiv) * osg::PI * 2.;
        osg::Vec3 pos = sphereCoordToCartesian(theta, osg::PI_2f, 1.);
        pos *= radius;
        pos.z() = -height / 2.;
        vertices->push_back(pos);
        normals->push_back(-topNormal);
        iVertex += 1;
    }
    // sides
    int beginSide = iVertex;
    for (int i = 0; i < subdiv; i++)
    {
        float theta = float(i) / float(subdiv) * osg::PI * 2.;
        osg::Vec3 normal = sphereCoordToCartesian(theta, osg::PI_2f, 1.);
        auto posTop = normal;
        posTop *= radius;
        auto posBot = posTop;
        posTop.z() = height / 2.;
        posBot.z() = -height / 2.;
        vertices->push_back(posTop);
        normals->push_back(normal);
        iVertex += 1;
        vertices->push_back(posBot);
        normals->push_back(normal);
        iVertex += 1;
    }

    // create triangles sides
    for (int i = 0; i < subdiv; i++)
    {
        auto next_vert = (i + 1) % subdiv;
        auto v1 = (beginSide + 2 * i);
        auto v2 = (beginSide + 2 * i + 1);
        auto v3 = (beginSide + 2 * next_vert);
        auto v4 = (beginSide + 2 * next_vert + 1);
        indices->push_back(v1);
        indices->push_back(v2);
        indices->push_back(v4);

        indices->push_back(v4);
        indices->push_back(v3);
        indices->push_back(v1);
    }
    for (int i = 0; i < subdiv; i++)
    {
        auto next_vert = (i + 1) % subdiv;
        auto top1 = (beginTop + i);
        auto top2 = (beginTop + next_vert);

        auto bot1 = (begin_bot + i);
        auto bot2 = (begin_bot + next_vert);

        indices->push_back(top2);
        indices->push_back(centerTop);
        indices->push_back(top1);

        indices->push_back(bot1);
        indices->push_back(centerBot);
        indices->push_back(bot2);
    }

    geom.setVertexArray(vertices);
    geom.setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
    geom.addPrimitiveSet(indices);
}

static int getIndexBufferReadFromFrame(const unsigned int& nFrame)
{
    return nFrame % 2;
}

static int getIndexBufferWriteFromFrame(const unsigned int& nFrame)
{
    return (nFrame + 1) % 2;
}

namespace Debug
{
    static void makeLineInstance(osg::Geometry& lines)
    {
        auto vertices = new osg::Vec3Array;
        auto color = new osg::Vec3Array;
        lines.setDataVariance(osg::Object::STATIC);
        lines.setUseVertexBufferObjects(true);
        lines.setUseDisplayList(false);
        lines.setCullingActive(false);

        lines.setVertexArray(vertices);
        lines.setNormalArray(color, osg::Array::BIND_PER_VERTEX);

        lines.addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size()));
    }

    DebugCustomDraw::DebugCustomDraw()
    {
        mLinesToDraw = new osg::Geometry();
        makeLineInstance(*mLinesToDraw);
    }

    DebugCustomDraw::DebugCustomDraw(const DebugCustomDraw& copy, const osg::CopyOp& copyop)
        : Drawable(copy, copyop)
        , mShapesToDraw(copy.mShapesToDraw)
        , mLinesToDraw(copy.mLinesToDraw)
        , mCubeGeometry(copy.mCubeGeometry)
        , mCylinderGeometry(copy.mCylinderGeometry)
        , mWireCubeGeometry(copy.mWireCubeGeometry)
    {
    }

    void DebugCustomDraw::drawImplementation(osg::RenderInfo& renderInfo) const
    {
        auto state = renderInfo.getState();
        osg::GLExtensions* ext = osg::GLExtensions::Get(state->getContextID(), true);

        const osg::StateSet* stateSet = getStateSet();

        const osg::Program::PerContextProgram& pcp = *state->getLastAppliedProgramObject();
        auto transLocation = pcp.getUniformLocation(stateSet->getUniform("trans")->getNameID());
        auto colLocation = pcp.getUniformLocation(stateSet->getUniform("color")->getNameID());
        auto scaleLocation = pcp.getUniformLocation(stateSet->getUniform("scale")->getNameID());
        auto normalAsColorLocation = pcp.getUniformLocation(stateSet->getUniform("useNormalAsColor")->getNameID());

        auto drawPrimitive = [&](const osg::Drawable* primitive, const osg::Vec3f& pos, const osg::Vec3f& color,
                                 const osg::Vec3f& scale, const bool normalAsColor) {
            ext->glUniform3f(transLocation, pos.x(), pos.y(), pos.z());
            ext->glUniform3f(colLocation, color.x(), color.y(), color.z());
            ext->glUniform3f(scaleLocation, scale.x(), scale.y(), scale.z());
            ext->glUniform1i(normalAsColorLocation, normalAsColor);
            primitive->drawImplementation(renderInfo);
        };

        drawPrimitive(mLinesToDraw, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }, { 1.f, 1.f, 1.f }, true);

        for (const auto& shapeToDraw : mShapesToDraw)
        {
            const osg::Geometry* geometry = nullptr;
            switch (shapeToDraw.mDrawShape)
            {
                case DrawShape::Cube:
                    geometry = mCubeGeometry;
                    break;
                case DrawShape::Cylinder:
                    geometry = mCylinderGeometry;
                    break;
                case DrawShape::WireCube:
                    geometry = mWireCubeGeometry;
                    break;
            }
            drawPrimitive(geometry, shapeToDraw.mPosition, shapeToDraw.mColor, shapeToDraw.mDims, false);
        }
        mShapesToDraw.clear();
        static_cast<osg::Vec3Array*>(mLinesToDraw->getVertexArray())->clear();
        static_cast<osg::Vec3Array*>(mLinesToDraw->getNormalArray())->clear();
        static_cast<osg::DrawArrays*>(mLinesToDraw->getPrimitiveSet(0))->setCount(0);
        pcp.resetAppliedUniforms();
    }
}

Debug::DebugDrawer::DebugDrawer(const DebugDrawer& copy, const osg::CopyOp& copyop)
    : Node(copy, copyop)
    , mCurrentFrame(copy.mCurrentFrame)
    , mCustomDebugDrawer(copy.mCustomDebugDrawer)
{
}

Debug::DebugDrawer::DebugDrawer(Shader::ShaderManager& shaderManager)
{
    auto program = shaderManager.getProgram("debug");

    setCullingActive(false);
    osg::StateSet* stateset = getOrCreateStateSet();
    stateset->addUniform(new osg::Uniform("color", osg::Vec3f(1., 1., 1.)));
    stateset->addUniform(new osg::Uniform("trans", osg::Vec3f(0., 0., 0.)));
    stateset->addUniform(new osg::Uniform("scale", osg::Vec3f(1., 1., 1.)));
    stateset->addUniform(new osg::Uniform("useNormalAsColor", false));
    stateset->addUniform(new osg::Uniform("useAdvancedShader", true));

    stateset->setAttributeAndModes(program, osg::StateAttribute::ON);
    stateset->setMode(GL_DEPTH_TEST, GL_TRUE);
    stateset->setMode(GL_CULL_FACE, GL_TRUE);

    auto cubeGeometry = new osg::Geometry;
    cubeGeometry->setSupportsDisplayList(false);
    cubeGeometry->setUseVertexBufferObjects(true);
    generateCube(*cubeGeometry, 1.);

    auto cylinderGeom = new osg::Geometry;
    cylinderGeom->setSupportsDisplayList(false);
    cylinderGeom->setUseVertexBufferObjects(true);
    generateCylinder(*cylinderGeom, .5, 1., 20);

    auto wireCube = new osg::Geometry;
    wireCube->setSupportsDisplayList(false);
    wireCube->setUseVertexBufferObjects(true);
    generateWireCube(*wireCube, 1.);

    for (std::size_t i = 0; i < mCustomDebugDrawer.size(); i++)
    {
        mCustomDebugDrawer[i] = new DebugCustomDraw();
        mCustomDebugDrawer[i]->setStateSet(stateset);
        mCustomDebugDrawer[i]->mWireCubeGeometry = wireCube;
        mCustomDebugDrawer[i]->mCubeGeometry = cubeGeometry;
        mCustomDebugDrawer[i]->mCylinderGeometry = cylinderGeom;
    }
}

void Debug::DebugDrawer::traverse(osg::NodeVisitor& nv)
{
    mCurrentFrame = nv.getTraversalNumber();
    mCustomDebugDrawer[getIndexBufferReadFromFrame(mCurrentFrame)]->accept(nv);
}

void Debug::DebugDrawer::drawCube(osg::Vec3f mPosition, osg::Vec3f mDims, osg::Vec3f mColor)
{
    mCustomDebugDrawer[getIndexBufferWriteFromFrame(mCurrentFrame)]->mShapesToDraw.push_back(
        { mPosition, mDims, mColor, DrawShape::Cube });
}

void Debug::DebugDrawer::drawCubeMinMax(osg::Vec3f min, osg::Vec3f max, osg::Vec3f color)
{
    osg::Vec3 dims = max - min;
    osg::Vec3 pos = min + dims * 0.5f;
    drawCube(pos, dims, color);
}

void Debug::DebugDrawer::addDrawCall(const DrawCall& draw)
{
    mCustomDebugDrawer[getIndexBufferWriteFromFrame(mCurrentFrame)]->mShapesToDraw.push_back(draw);
}

void Debug::DebugDrawer::addLine(const osg::Vec3& start, const osg::Vec3& end, const osg::Vec3 color)
{
    const int indexWrite = getIndexBufferWriteFromFrame(mCurrentFrame);
    const auto& lines = mCustomDebugDrawer[indexWrite]->mLinesToDraw;
    auto vertices = static_cast<osg::Vec3Array*>(lines->getVertexArray());
    auto colors = static_cast<osg::Vec3Array*>(lines->getNormalArray());
    auto primitive = static_cast<osg::DrawArrays*>(lines->getPrimitiveSet(0));

    vertices->push_back(start);
    vertices->push_back(end);
    vertices->dirty();

    colors->push_back(color);
    colors->push_back(color);
    colors->dirty();

    primitive->setCount(vertices->size());
}
