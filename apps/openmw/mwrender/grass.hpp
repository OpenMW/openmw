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
    struct GrassItem
    {
        std::string mModel;
        ESM::Position mPos;
        float mScale;
        osg::ref_ptr<osg::Group> mNode;

        void updateVisibility(osg::Vec3f& playerPos);
        void attachToNode(osg::Group* cellnode, Resource::ResourceSystem* rs, osg::Uniform* windUniform, osg::Uniform* isGrassUniform);
    };

    struct Grass
    {
        std::vector<GrassItem> mItems;
        float mCurrentGrass = 0;

        bool loadGrassItem(const std::string& model, ESM::Position& pos, const float scale);
        void insertGrass(osg::Group* cellnode, Resource::ResourceSystem* rs);
        bool isEnabled(const std::string& model);
        static bool isGrassItem(const std::string& model);
        osg::ref_ptr<osg::Uniform> mWindSpeedUniform;
        osg::ref_ptr<osg::Uniform> mIsGrassUniform;

        Grass()
        {
            blank();
            mWindSpeedUniform = new osg::Uniform("windSpeed", 0.0f);
            mIsGrassUniform = new osg::Uniform("isGrass", true);
        }

        void blank()
        {
            mItems.clear();
            mCurrentGrass = 0;
        }

        void update();
    };
}

#endif
