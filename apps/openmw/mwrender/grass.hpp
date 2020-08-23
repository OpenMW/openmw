#ifndef GAME_MWRENDER_GRASS_H
#define GAME_MWRENDER_GRASS_H

#include <string>

#include <osg/Group>

#include <components/esm/defs.hpp>

#include <components/sceneutil/statesetupdater.hpp>

namespace Resource
{
    class ResourceSystem;
}

namespace MWRender
{
    class GrassUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        GrassUpdater()
            : mWindSpeed(0.f)
            , mPlayerPos(osg::Vec3f())
        {
        }

        void setWindSpeed(float windSpeed)
        {
            mWindSpeed = windSpeed;
        }

        void setPlayerPos(osg::Vec3f playerPos)
        {
            mPlayerPos = playerPos;
        }

    protected:
        virtual void setDefaults(osg::StateSet *stateset);

        virtual void apply(osg::StateSet *stateset, osg::NodeVisitor *nv);

    private:
        float mWindSpeed;
        osg::Vec3f mPlayerPos;
    };

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
        osg::ref_ptr<GrassUpdater> mGrassUpdater;

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
