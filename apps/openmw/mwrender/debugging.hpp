#ifndef _GAME_RENDER_MWSCENE_H
#define _GAME_RENDER_MWSCENE_H

#include <utility>
#include <openengine/ogre/renderer.hpp>
#include <openengine/bullet/physic.hpp>
#include "../mwworld/ptr.hpp"

#include <vector>
#include <string>

namespace Ogre
{
    class Camera;
    class Viewport;
    class SceneManager;
    class SceneNode;
    class RaySceneQuery;
    class Quaternion;
    class Vector3;
}

namespace MWWorld
{
    class World;
    class Environment;
}

namespace MWRender
{
    class Player;

    class Debugging
    {
        OEngine::Physic::PhysicEngine* mEngine;
        Ogre::SceneManager *mSceneMgr;
        MWWorld::Environment& mEnvironment;

        // Path grid stuff
        bool pathgridEnabled;

        void togglePathgrid();

        typedef std::vector<MWWorld::Ptr::CellStore *> CellList;

        CellList mActiveCells;

        Ogre::SceneNode *mMwRoot;
        Ogre::SceneNode *mPathGridRoot;
        Ogre::SceneNode *mInteriorPathgridNode;

        typedef std::map<std::pair<int,int>, Ogre::SceneNode *> ExteriorPathgridNodes;
        ExteriorPathgridNodes mExteriorPathgridNodes;

        void enableCellPathgrid(MWWorld::Ptr::CellStore *store);
        void disableCellPathgrid(MWWorld::Ptr::CellStore *store);

        // utility
        void destroyCellPathgridNode(Ogre::SceneNode *node);
        void destroyAttachedObjects(Ogre::SceneNode *node);

    public:
        Debugging(Ogre::SceneNode* mwRoot, MWWorld::Environment &env, OEngine::Physic::PhysicEngine *engine);
        ~Debugging();
        bool toggleRenderMode (int mode);

        void cellAdded(MWWorld::Ptr::CellStore* store);
        void cellRemoved(MWWorld::Ptr::CellStore* store);
    };


}

#endif
