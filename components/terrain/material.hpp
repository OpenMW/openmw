#ifndef COMPONENTS_TERRAIN_MATERIAL_H
#define COMPONENTS_TERRAIN_MATERIAL_H

#include <osgFX/Technique>
#include <osgFX/Effect>

#include "defs.hpp"

namespace osg
{
    class Texture2D;
}

namespace Terrain
{

    class FixedFunctionTechnique : public osgFX::Technique
    {
    public:
        FixedFunctionTechnique(
                const std::vector<osg::ref_ptr<osg::Texture2D> >& layers,
                const std::vector<osg::ref_ptr<osg::Texture2D> >& blendmaps);

    protected:
        virtual void define_passes() {}
    };

    class Effect : public osgFX::Effect
    {
    public:
        Effect(
                const std::vector<osg::ref_ptr<osg::Texture2D> >& layers,
                const std::vector<osg::ref_ptr<osg::Texture2D> >& blendmaps);

        virtual bool define_techniques();

        virtual const char *effectName() const
        {
            return nullptr;
        }
        virtual const char *effectDescription() const
        {
            return nullptr;
        }
        virtual const char *effectAuthor() const
        {
            return nullptr;
        }

    private:
        std::vector<osg::ref_ptr<osg::Texture2D> > mLayers;
        std::vector<osg::ref_ptr<osg::Texture2D> > mBlendmaps;
    };

}

#endif
