#ifndef GAME_RENDER_MWSCENE_H
#define GAME_RENDER_MWSCENE_H

#include <utility>
#include <openengine/ogre/renderer.hpp>

#include <vector>
#include <string>

namespace ESM
{
    struct Pathgrid;
}

namespace OEngine
{
    namespace Physic
    {
        class PhysicEngine;
    }
}

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
    class Ptr;
    class CellStore;
}

namespace MWRender
{
    class Debugging
    {
        OEngine::Physic::PhysicEngine* mEngine;
        Ogre::SceneManager *mSceneMgr;

        // Path grid stuff
        bool mPathgridEnabled;

        void togglePathgrid();

        typedef std::vector<MWWorld::CellStore *> CellList;
        CellList mActiveCells;

        Ogre::SceneNode *mRootNode;

        Ogre::SceneNode *mPathGridRoot;

        typedef std::map<std::pair<int,int>, Ogre::SceneNode *> ExteriorPathgridNodes;
        ExteriorPathgridNodes mExteriorPathgridNodes;
        Ogre::SceneNode *mInteriorPathgridNode;

        void enableCellPathgrid(MWWorld::CellStore *store);
        void disableCellPathgrid(MWWorld::CellStore *store);

        // utility
        void destroyCellPathgridNode(Ogre::SceneNode *node);
        void destroyAttachedObjects(Ogre::SceneNode *node);

        // materials
        bool mGridMatsCreated;
        void createGridMaterials();
        void destroyGridMaterials();

        // path grid meshes
        Ogre::ManualObject *createPathgridLines(const ESM::Pathgrid *pathgrid);
        Ogre::ManualObject *createPathgridPoints(const ESM::Pathgrid *pathgrid);
    public:
        Debugging(Ogre::SceneNode* root, OEngine::Physic::PhysicEngine *engine);
        ~Debugging();
        bool toggleRenderMode (int mode);

        void cellAdded(MWWorld::CellStore* store);
        void cellRemoved(MWWorld::CellStore* store);
    };


}

#endif
