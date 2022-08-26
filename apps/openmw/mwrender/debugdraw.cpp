#include "debugdraw.h"
#include "debugdraw.hpp"
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/shader/shadermanager.hpp>


#include <osg/uniform>
#include <osg/drawable>
#include <osg/program>
#include <osg/Array>
#include <osg/Vec3>
#include <osg/GLExtensions>

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

        auto transLocation = pcp->getUniformLocation(uTrans->getNameID() );
        auto colLocation = pcp->getUniformLocation(uCol->getNameID() );

        for (int i = 0; i < mCubesToDraw.size(); i++)
        {
            osg::Vec3f translation = mCubesToDraw[i].mPosition;
            osg::Vec3f color = mCubesToDraw[i].mColor;

            if (uTrans)
                ext->glUniform3f(transLocation, translation.x(), translation.y(), translation.z());
            if (uCol)
            {
                ext->glUniform3f(colLocation, color.x(), color.y(), color.z());
            }
            this->mCubeGeometry->drawImplementation(renderInfo);
        }



    }
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


MWRenderDebug::DebugDrawer::DebugDrawer(MWRender::RenderingManager& renderingManager,osg::ref_ptr<osg::Group> parentNode)
{
    auto& shaderManager = renderingManager.getResourceSystem()->getSceneManager()->getShaderManager();
    auto vertexShader = shaderManager.getShader("debugDraw_vertex.glsl", Shader::ShaderManager::DefineMap(), osg::Shader::Type::VERTEX);
    auto fragmentShader = shaderManager.getShader("debugDraw_fragment.glsl", Shader::ShaderManager::DefineMap(), osg::Shader::Type::FRAGMENT);

    auto program = shaderManager.getProgram(vertexShader, fragmentShader);
    program->addBindAttribLocation("aPos", 0);
    program->addBindAttribLocation("aNormal", 1);
    mCubeGeometry = new osg::Geometry;
    mcustomCubesDrawer = new CubeCustomDraw(mCubeGeometry, mCubesToDrawRead, mDrawCallMutex);
    osg::StateSet* stateset = mcustomCubesDrawer->getOrCreateStateSet();

    stateset->addUniform(new osg::Uniform("passColor", osg::Vec3f(1., 1., 1.)));
    stateset->addUniform(new osg::Uniform("trans", osg::Vec3f(1., 1., 1.)));

    stateset->setAttributeAndModes(program, osg::StateAttribute::ON);
    stateset->setMode(GL_DEPTH_TEST, GL_TRUE);
    stateset->setMode(GL_CULL_FACE, GL_TRUE);
    mCubeGeometry->setCullingActive(false);
    mCubeGeometry->setSupportsDisplayList(false);
    mCubeGeometry->setUseVertexBufferObjects(true);

    generateCube(*mCubeGeometry,50.);

    mCubeGeometry->setStateSet(stateset->clone(osg::CopyOp::DEEP_COPY_ALL)->asStateSet());

    parentNode->addChild(mcustomCubesDrawer);
}

void MWRenderDebug::DebugDrawer::update()
{
    {
        std::lock_guard lock(mDrawCallMutex);
        mCubesToDrawRead.swap(mCubesToDrawWrite);
    }
    mCubesToDrawWrite.clear();


}

void MWRenderDebug::DebugDrawer::drawCube(osg::Vec3f mPosition, osg::Vec3f mDims, osg::Vec3f mColor)
{
    mCubesToDrawWrite.push_back({ mDims, mColor, mPosition });
}
