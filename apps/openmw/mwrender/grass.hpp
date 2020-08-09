#ifndef GAME_MWRENDER_GRASS_H
#define GAME_MWRENDER_GRASS_H

#include <string>

#include <osg/BlendFunc>
#include <osg/Group>
#include <osg/Material>

#include <components/resource/scenemanager.hpp>

#include "animation.hpp"

namespace MWRender
{
    class GrassUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        GrassUpdater(const float alpha)
            : mAlpha(alpha)
        {
        }

        void setAlpha(const float alpha)
        {
            mAlpha = alpha;
        }

    protected:
        virtual void setDefaults(osg::StateSet* stateset)
        {
            osg::BlendFunc* blendfunc (new osg::BlendFunc);
            stateset->setAttributeAndModes(blendfunc, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

            stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            stateset->setRenderBinMode(osg::StateSet::OVERRIDE_RENDERBIN_DETAILS);

            osg::Material* material = new osg::Material;
            material->setColorMode(osg::Material::OFF);
            material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,mAlpha));
            //material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
            stateset->setAttributeAndModes(material, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
            stateset->addUniform(new osg::Uniform("colorMode", 0), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        }

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/)
        {
            osg::Material* material = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
            material->setAlpha(osg::Material::FRONT_AND_BACK, mAlpha);
        }

    private:
        float mAlpha;
    };

    struct GrassItem
    {
        std::string mModel;
        ESM::Position mPos;
        float mScale;
        osg::ref_ptr<osg::Group> mNode;

        osg::ref_ptr<osg::Uniform> mStormDirectionUniform;

        void updateVisibility(osg::Vec3f& playerPos);
        void attachToNode(osg::Group* cellnode, Resource::ResourceSystem* rs);

    private:
        void setAlpha(float alpha);

        float mVisibility = 1.f;
        osg::ref_ptr<MWRender::GrassUpdater> mAlphaUpdater;
    };

    struct Grass
    {
        std::vector<GrassItem> mItems;
        float mCurrentGrass = 0;

        bool loadGrassItem(const std::string& model, ESM::Position& pos, const float scale);
        void updateGrassVisibility();
        void insertGrass(osg::Group* cellnode, Resource::ResourceSystem* rs);
        bool isEnabled(const std::string& model);
        static bool isGrassItem(const std::string& model);

        void blank()
        {
            mItems.clear();
            mCurrentGrass = 0;
        }
    };
}

#endif
