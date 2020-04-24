#include "waterutil.hpp"

#include <osg/Depth>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/StateSet>

namespace SceneUtil
{
    osg::ref_ptr<osg::Geometry> createWaterGeometry(float size, int segments, float textureRepeats)
    {
        osg::ref_ptr<osg::Vec3Array> verts (new osg::Vec3Array);
        osg::ref_ptr<osg::Vec2Array> texcoords (new osg::Vec2Array);

        // some drivers don't like huge triangles, so we do some subdivisons
        // a paged solution would be even better
        const float step = size/segments;
        const float texCoordStep = textureRepeats / segments;
        for (int x=0; x<segments; ++x)
        {
            for (int y=0; y<segments; ++y)
            {
                float x1 = -size/2.f + x*step;
                float y1 = -size/2.f + y*step;
                float x2 = x1 + step;
                float y2 = y1 + step;

                verts->push_back(osg::Vec3f(x1, y2, 0.f));
                verts->push_back(osg::Vec3f(x1, y1, 0.f));
                verts->push_back(osg::Vec3f(x2, y1, 0.f));
                verts->push_back(osg::Vec3f(x2, y2, 0.f));

                float u1 = x*texCoordStep;
                float v1 = y*texCoordStep;
                float u2 = u1 + texCoordStep;
                float v2 = v1 + texCoordStep;

                texcoords->push_back(osg::Vec2f(u1, v2));
                texcoords->push_back(osg::Vec2f(u1, v1));
                texcoords->push_back(osg::Vec2f(u2, v1));
                texcoords->push_back(osg::Vec2f(u2, v2));
            }
        }

        osg::ref_ptr<osg::Geometry> waterGeom (new osg::Geometry);
        waterGeom->setVertexArray(verts);
        waterGeom->setTexCoordArray(0, texcoords);

        osg::ref_ptr<osg::Vec3Array> normal (new osg::Vec3Array);
        normal->push_back(osg::Vec3f(0,0,1));
        waterGeom->setNormalArray(normal, osg::Array::BIND_OVERALL);

        waterGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,verts->size()));
        return waterGeom;
    }

    osg::ref_ptr<osg::StateSet> createSimpleWaterStateSet(float alpha, int renderBin)
    {
        osg::ref_ptr<osg::StateSet> stateset (new osg::StateSet);

        osg::ref_ptr<osg::Material> material (new osg::Material);
        material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, alpha));
        material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, 1.f));
        material->setColorMode(osg::Material::OFF);
        stateset->setAttributeAndModes(material, osg::StateAttribute::ON);

        stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

        osg::ref_ptr<osg::Depth> depth (new osg::Depth);
        depth->setWriteMask(false);
        stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);

        stateset->setRenderBinDetails(renderBin, "RenderBin");

        // Let the shader know we're dealing with simple water here.
        stateset->addUniform(new osg::Uniform("simpleWater", true));

        return stateset;
    }
}
