#ifndef GAME_MWRENDER_GRASS_H
#define GAME_MWRENDER_GRASS_H

#include <string>

#include <osg/Group>

#include <components/esm/defs.hpp>

namespace Resource
{
    class ResourceSystem;
}

namespace MWRender
{
    struct GrassItem
    {
        std::string mModel;
        ESM::Position mPos;
        float mScale;
        osg::ref_ptr<osg::Group> mNode;

        void updateVisibility(osg::Vec3f& playerPos);
    };

    struct Grass
    {
        std::vector<GrassItem> mItems;
        float mCurrentGrass = 0;
        bool mUseAnimation = false;
        osg::ref_ptr<osg::Uniform> mWindSpeedUniform;

        Grass();
        void blank();
        bool loadGrassItem(const std::string& model, ESM::Position& pos, const float scale);
        void insertGrass(osg::Group* cellnode, Resource::ResourceSystem* rs);
        bool isEnabled(const std::string& model);
        static bool isGrassItem(const std::string& model);

        void attachToNode(MWRender::GrassItem& item, osg::Group* cellnode, Resource::ResourceSystem* rs);
        void update();
    };
}

#endif
