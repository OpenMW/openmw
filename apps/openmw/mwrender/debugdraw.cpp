#include "debugdraw.hpp"
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/shader/shadermanager.hpp>


#include <osg/Uniform>
#include <osg/Drawable>
#include <osg/Program>
#include <osg/Array>
#include <osg/Vec3>
#include <osg/GLExtensions>


static osg::Vec3 sphereCoordToCarthesian(float theta ,float phi ,float r ) 
{
    osg::Vec3 returnVec = osg::Vec3(0.0,0.0,0.0);
    float phiToHorizontal = osg::PI_2 - phi ;
    returnVec.x() = std::cos( theta);
    returnVec.y() = std::sin( theta);
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

    geom.setVertexAttribArray(0, vertices, osg::Array::BIND_PER_VERTEX);
    geom.setVertexAttribArray(1, normals, osg::Array::BIND_PER_VERTEX);

    osg::Vec2i indexPos[] = { osg::Vec2i(0,0),osg::Vec2i(1,0),osg::Vec2i(1,1),osg::Vec2i(0,1) };

    for (int i = 0; i < 4; i++)
    {
        osg::Vec3 vert1 = osg::Vec3(indexPos[i].x() - 0.5, indexPos[i].y()- 0.5, 0.5);
        int next = (i + 1) % 4;
        osg::Vec3 vert2 = osg::Vec3(indexPos[next].x()- 0.5, indexPos[next].y()- 0.5, 0.5);

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
    for (unsigned long i = 0; i < vertices->size(); i ++)
    {
        normals->push_back(osg::Vec3(1., 1., 1.));
    }

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
        //if (axis != 2)
        //    continue;
        int dir = i_face % 2 == 0 ? -1 : 1;
        float float_dir = dir;
        normale[axis] = float_dir;
        u[(axis + 1) % 3] = 1.0;
        v[(axis + 2) % 3] = 1.0;


        for (int i_point = 0; i_point < 4; i_point++)
        {
            float iu = i_point % 2 == 1 ? float_dir : -float_dir;//This is to get the right triangle orientation when the normal changes*
            float iv = i_point / 2 == 1 ? 1.0 : -1.0;
            osg::Vec3f point = (u * iu) + (v * iv);
            point = (point + normale);
            point = point * (dim * 0.5f);
            vertices->push_back(point);
            normals->push_back(normale);
        }
        int start_vertex(i_face * 4);
        int newFace1[] = { start_vertex,start_vertex + 1,start_vertex + 2 };
        for (int i = 0; i < 3; i++)
        {
            indices->push_back(newFace1[i]);
        }
        int newFace2[] = { start_vertex + 2,start_vertex + 1,start_vertex + 3 };
        for (int i = 0; i < 3; i++)
        {
            indices->push_back(newFace2[i]);
        }
    }
    geom.setVertexAttribArray(0, vertices, osg::Array::BIND_PER_VERTEX);
    geom.setVertexAttribArray(1, normals, osg::Array::BIND_PER_VERTEX);
    geom.addPrimitiveSet(indices);

}


static void generateCylinder(osg::Geometry& geom, float radius, float height, int subdiv)
{
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUShort> indices = new osg::DrawElementsUShort(osg::DrawElementsUShort::TRIANGLES, 0);
    int vertexCount = subdiv * 4 + 2; //2 discs + top and bottom + 2 center
    indices->reserve(vertexCount );
    int iVertex = 0;

    int beginTop = iVertex;
    auto topNormal = osg::Vec3(0.,0.,1.);
    //top disk
    for (int i = 0 ;i <  subdiv; i++)
    {
        float theta = (float(i )/ float(subdiv )) * osg::PI * 2.;
        osg::Vec3 pos= sphereCoordToCarthesian(theta, osg::PI_2, 1.);
        pos *= radius;
        pos.z() = height / 2.;
        vertices->push_back(pos);
        normals->push_back(topNormal);
        iVertex+=1;
    }
    auto centerTop = iVertex;
    //centerTop
    {
        vertices->push_back(osg::Vec3(0.,0.,height/2.));
        normals->push_back(topNormal);
        iVertex+=1;
    }
    auto centerBot = iVertex;
    //centerBot
    {
        vertices->push_back(osg::Vec3(0.,0.,-height/2));
        normals->push_back(-topNormal);
        iVertex+=1;
    }
    //bottom disk
    auto begin_bot = iVertex;
    for (int i = 0 ;i <  subdiv; i++)
    {
        float theta = float(i)/ float(subdiv) * osg::PI*2.;
        osg::Vec3 pos= sphereCoordToCarthesian(theta, osg::PI_2, 1.);
        pos *= radius;
        pos.z() = - height / 2.;
        vertices->push_back(pos);
        normals->push_back(-topNormal);
        iVertex+=1;
    }
        //sides
    int beginSide = iVertex;
    for (int i = 0 ;i <  subdiv; i++)
    {
        float theta = float(i )/ float(subdiv) * osg::PI*2.;
        osg::Vec3 normal = sphereCoordToCarthesian(theta, osg::PI_2, 1.);
        auto posTop = normal;
        posTop *= radius;
        auto posBot = posTop;
        posTop.z() =  height /2.;
        posBot.z() = -height /2.;
        vertices->push_back(posTop);
        normals->push_back(normal);
        iVertex+=1;
        vertices->push_back(posBot);
        normals->push_back(normal);
        iVertex+=1;
    }

        //create triangles sides
    for (int i = 0 ;i <  subdiv; i++)
    {
        auto next_vert = (i+1)%subdiv;
        auto v1 = (beginSide + 2 *i);
        auto v2 = (beginSide + 2 *i +1);
        auto v3 = (beginSide + 2 *next_vert);
        auto v4 = (beginSide + 2 *next_vert +1);
        indices->push_back(v1);
        indices->push_back(v2);
        indices->push_back(v4);

        indices->push_back(v4);
        indices->push_back(v3);
        indices->push_back(v1);

    }
    for (int i = 0 ;i < subdiv; i++)
    {
        auto next_vert = (i+1)%subdiv;
        auto top1 = (beginTop + i) ;
        auto top2 = (beginTop + next_vert) ;

        auto bot1 = (begin_bot + i) ;
        auto bot2 = (begin_bot + next_vert) ;

        indices->push_back(top2);
        indices->push_back(centerTop);
        indices->push_back(top1);

        indices->push_back(bot1);
        indices->push_back(centerBot);
        indices->push_back(bot2);
    }

    geom.setVertexAttribArray(0, vertices, osg::Array::BIND_PER_VERTEX);
    geom.setVertexAttribArray(1, normals, osg::Array::BIND_PER_VERTEX);
    geom.addPrimitiveSet(indices);
}


namespace MWRenderDebug
{
    void CubeCustomDraw::drawImplementation(osg::RenderInfo& renderInfo) const
    {
        auto state = renderInfo.getState();
        osg::GLExtensions* ext = osg::GLExtensions::Get( state->getContextID(), true );

        const osg::StateSet* stateSet = this->getStateSet();

        auto program = static_cast<const osg::Program*>( stateSet->getAttribute(osg::StateAttribute::PROGRAM));
        const osg::Program::PerContextProgram* pcp = program->getPCP(*state);
        if (!pcp)
        {
            return;
        }
        std::lock_guard lock(mDrawCallMutex);

        osg::Uniform* uTrans = const_cast<osg::Uniform*>( stateSet->getUniform("trans"));
        osg::Uniform* uCol = const_cast<osg::Uniform*>( stateSet->getUniform("passColor"));
        osg::Uniform* uScale = const_cast<osg::Uniform*>( stateSet->getUniform("scale"));
        osg::Uniform* uUseNormalAsColor = const_cast<osg::Uniform*>( stateSet->getUniform("useNormalAsColor"));



        auto transLocation = pcp->getUniformLocation(uTrans->getNameID() );
        auto colLocation = pcp->getUniformLocation(uCol->getNameID() );
        auto scaleLocation = pcp->getUniformLocation(uScale->getNameID() );
        auto normalAsColorLocation = pcp->getUniformLocation(uUseNormalAsColor->getNameID() );


        ext->glUniform3f(transLocation, 0., 0., 0.);
        ext->glUniform3f(colLocation, 1., 1., 1.);
        ext->glUniform3f(scaleLocation,  1., 1., 1.);
        ext->glUniform1i(normalAsColorLocation,  1);

        mlinesToDraw->drawImplementation(renderInfo);

        ext->glUniform1i(normalAsColorLocation,  0);

        for (const auto& shapeToDraw : mShapsToDraw)
        {
            osg::Vec3f translation = shapeToDraw.mPosition;
            osg::Vec3f color = shapeToDraw.mColor;
            osg::Vec3f scale = shapeToDraw.mDims;


            if (uTrans)
                ext->glUniform3f(transLocation, translation.x(), translation.y(), translation.z());
            if (uCol)
            {
                ext->glUniform3f(colLocation, color.x(), color.y(), color.z());
            }
            if (uScale)
            {
                ext->glUniform3f(scaleLocation, scale.x(), scale.y(), scale.z());
            }
            switch (shapeToDraw.mDrawShape)
            {
            case DrawShape::Cube:
                this->mCubeGeometry->drawImplementation(renderInfo);
                break;
            case DrawShape::Cylinder:
                this->mCylinderGeometry->drawImplementation(renderInfo);
                break;
            case DrawShape::WireCube:
                this->mWireCubeGeometry->drawImplementation(renderInfo);
                break;
            }
        }

    }

    struct DebugLines
    {

        static void makeLineInstance( osg::Geometry& lines)
        {
            auto vertices  = new osg::Vec3Array;
            auto color     = new osg::Vec3Array;

            for (int i = 0; i < 2; i++)
            {
                vertices->push_back(osg::Vec3());
                vertices->push_back(osg::Vec3(0., 0., 0.));
                color->push_back(osg::Vec3(1., 1., 1.));
                color->push_back(osg::Vec3(1., 1., 1.));
            }

            lines.setUseVertexArrayObject(true);
            lines.setUseDisplayList(false);
            lines.setCullingActive(false);

            lines.setVertexAttribArray(0, vertices, osg::Array::BIND_PER_VERTEX);
            lines.setVertexAttribArray(1, color, osg::Array::BIND_PER_VERTEX);

            lines.addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size()));

            //lines.setStateSet(stateSet->clone(osg::CopyOp::DEEP_COPY_ALL)->asStateSet());
        }

        DebugLines()
        {

            mLinesWrite = new osg::Geometry();
            mLinesRead  = new osg::Geometry();

            makeLineInstance(*mLinesRead);
            makeLineInstance(*mLinesWrite);

        }

        void update(std::mutex& mutex)
        {
            mLinesWrite->removePrimitiveSet(0, 1);
            mLinesWrite->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0,static_cast<osg::Vec3Array*>(mLinesWrite->getVertexAttribArray(0))->size()));

            {
                auto lock = std::scoped_lock(mutex);
                mLinesWrite.swap(mLinesRead);
            }


            static_cast<osg::Vec3Array*>(mLinesWrite->getVertexAttribArray(0))->resize(2, osg::Vec3());
            static_cast<osg::Vec3Array*>(mLinesWrite->getVertexAttribArray(1))->resize(2, osg::Vec3());
        }

        osg::ref_ptr<osg::Geometry>  mLinesWrite;
        osg::ref_ptr<osg::Geometry>  mLinesRead;
    };
}

MWRenderDebug::DebugDrawer::DebugDrawer(MWRender::RenderingManager& renderingManager,osg::ref_ptr<osg::Group> parentNode)
{
    auto& shaderManager = renderingManager.getResourceSystem()->getSceneManager()->getShaderManager();
    auto vertexShader = shaderManager.getShader("debugDraw_vertex.glsl", Shader::ShaderManager::DefineMap(), osg::Shader::Type::VERTEX);
    auto fragmentShader = shaderManager.getShader("debugDraw_fragment.glsl", Shader::ShaderManager::DefineMap(), osg::Shader::Type::FRAGMENT);

    auto program = shaderManager.getProgram(vertexShader, fragmentShader);
    mDebugLines = std::make_unique<DebugLines>();
    mcustomCubesDrawer = new CubeCustomDraw(mShapesToDrawRead,mDebugLines->mLinesRead, mDrawCallMutex);

    mDebugDrawSceneObjects = new osg::Group;
    mDebugDrawSceneObjects->setCullingActive(false);
    osg::StateSet* stateset = mDebugDrawSceneObjects->getOrCreateStateSet();
    stateset->addUniform(new osg::Uniform("passColor", osg::Vec3f(1., 1., 1.)));
    stateset->addUniform(new osg::Uniform("trans", osg::Vec3f(0., 0., 0.)));
    stateset->addUniform(new osg::Uniform("scale", osg::Vec3f(1., 1., 1.)));
    stateset->addUniform(new osg::Uniform("useNormalAsColor", 0));

    stateset->setAttributeAndModes(program, osg::StateAttribute::ON);
    stateset->setMode(GL_DEPTH_TEST, GL_TRUE);
    stateset->setMode(GL_CULL_FACE, GL_TRUE);

    auto cubeGeometry = new osg::Geometry;
    cubeGeometry->setSupportsDisplayList(false);
    cubeGeometry->setUseVertexBufferObjects(true);
    generateCube(*cubeGeometry,1.);
    mcustomCubesDrawer->mCubeGeometry = cubeGeometry;

    auto cylinderGeom = new osg::Geometry;
    cylinderGeom->setSupportsDisplayList(false);
    cylinderGeom->setUseVertexBufferObjects(true);
    generateCylinder(*cylinderGeom, .5, 1., 20);
    mcustomCubesDrawer->mCylinderGeometry = cylinderGeom;

    auto wireCube = new osg::Geometry;
    wireCube->setSupportsDisplayList(false);
    wireCube->setUseVertexBufferObjects(true);
    generateWireCube(*wireCube, 1.);
    mcustomCubesDrawer->mWireCubeGeometry = wireCube;
    mcustomCubesDrawer->setStateSet( stateset);

    mDebugDrawSceneObjects->addChild(mcustomCubesDrawer);

    parentNode->addChild(mDebugDrawSceneObjects);
}

MWRenderDebug::DebugDrawer::~DebugDrawer()
{

}

void MWRenderDebug::DebugDrawer::update()
{
    {
        std::lock_guard lock(mDrawCallMutex);
        mShapesToDrawRead.swap(mShapesToDrawWrite);
    }
    mShapesToDrawWrite.clear();
    mDebugLines->update(mDrawCallMutex);
}

void MWRenderDebug::DebugDrawer::drawCube(osg::Vec3f mPosition, osg::Vec3f mDims, osg::Vec3f mColor)
{
    mShapesToDrawWrite.push_back({mPosition, mDims, mColor, DrawShape::Cube});
}

void MWRenderDebug::DebugDrawer::drawCubeMinMax(osg::Vec3f min, osg::Vec3f max, osg::Vec3f color)
{
    osg::Vec3 dims = max - min;
    osg::Vec3 pos = min + dims * 0.5f;
    drawCube(pos, dims, color);
}

void MWRenderDebug::DebugDrawer::addDrawCall(const DrawCall& draw)
{
    mShapesToDrawWrite.push_back(draw);
}

void MWRenderDebug::DebugDrawer::addLine(const osg::Vec3& start, const osg::Vec3& end, const osg::Vec3 color)
{
    auto     vertices = static_cast<osg::Vec3Array*>(mDebugLines->mLinesWrite->getVertexAttribArray(0));
    auto     colors = static_cast<osg::Vec3Array*>(mDebugLines->mLinesWrite->getVertexAttribArray(1));

    vertices->push_back(start);
    vertices->push_back(end);
    vertices->dirty();

    colors->push_back(color);
    colors->push_back(color);
    colors->dirty();

}
