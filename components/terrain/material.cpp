#include "material.hpp"

#include <osg/Depth>
#include <osg/TexEnvCombine>
#include <osg/Texture2D>
#include <osg/TexMat>
#include <osg/Material>
#include <osg/TexEnvCombine>

namespace Terrain
{

    FixedFunctionTechnique::FixedFunctionTechnique(const std::vector<osg::ref_ptr<osg::Texture2D> >& layers,
                                                   const std::vector<osg::ref_ptr<osg::Texture2D> >& blendmaps)
    {
        bool firstLayer = true;
        int i=0;
        for (std::vector<osg::ref_ptr<osg::Texture2D> >::const_iterator it = layers.begin(); it != layers.end(); ++it)
        {
            osg::ref_ptr<osg::StateSet> stateset (new osg::StateSet);

            if (!firstLayer)
            {
                stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
                osg::ref_ptr<osg::Depth> depth (new osg::Depth);
                depth->setFunction(osg::Depth::EQUAL);
                stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);
            }

            int texunit = 0;
            if(!firstLayer)
            {
                osg::ref_ptr<osg::Texture2D> blendmap = blendmaps.at(i++);

                stateset->setTextureAttributeAndModes(texunit, blendmap.get());

                // This is to map corner vertices directly to the center of a blendmap texel.
                osg::Matrixf texMat;
                float scale = (16/(16.f+1.f));
                texMat.preMultTranslate(osg::Vec3f(0.5f, 0.5f, 0.f));
                texMat.preMultScale(osg::Vec3f(scale, scale, 1.f));
                texMat.preMultTranslate(osg::Vec3f(-0.5f, -0.5f, 0.f));

                stateset->setTextureAttributeAndModes(texunit, new osg::TexMat(texMat));

                osg::ref_ptr<osg::TexEnvCombine> texEnvCombine (new osg::TexEnvCombine);
                texEnvCombine->setCombine_RGB(osg::TexEnvCombine::REPLACE);
                texEnvCombine->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);

                stateset->setTextureAttributeAndModes(texunit, texEnvCombine, osg::StateAttribute::ON);

                ++texunit;
            }

            // Add the actual layer texture multiplied by the alpha map.
            osg::ref_ptr<osg::Texture2D> tex = *it;
            stateset->setTextureAttributeAndModes(texunit, tex.get());

            osg::ref_ptr<osg::TexMat> texMat (new osg::TexMat);
            float scale = 16.f;
            texMat->setMatrix(osg::Matrix::scale(osg::Vec3f(scale,scale,1.f)));
            stateset->setTextureAttributeAndModes(texunit, texMat, osg::StateAttribute::ON);

            firstLayer = false;

            addPass(stateset);
        }
    }

    Effect::Effect(const std::vector<osg::ref_ptr<osg::Texture2D> > &layers, const std::vector<osg::ref_ptr<osg::Texture2D> > &blendmaps)
        : mLayers(layers)
        , mBlendmaps(blendmaps)
    {
        osg::ref_ptr<osg::Material> material (new osg::Material);
        material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
        getOrCreateStateSet()->setAttributeAndModes(material, osg::StateAttribute::ON);

        selectTechnique(0);
    }

    bool Effect::define_techniques()
    {
        addTechnique(new FixedFunctionTechnique(mLayers, mBlendmaps));

        return true;
    }

}
