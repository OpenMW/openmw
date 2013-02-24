#ifndef _GAME_RENDER_ACTORS_H
#define _GAME_RENDER_ACTORS_H

#include <openengine/ogre/renderer.hpp>

namespace MWWorld
{
    class Ptr;
    class CellStore;
    class InventoryStore;
}

namespace MWRender
{
    class Animation;

    class Actors
    {
        typedef std::map<MWWorld::CellStore*,Ogre::SceneNode*> CellSceneNodeMap;
        typedef std::map<MWWorld::Ptr,Animation*> PtrAnimationMap;

        OEngine::Render::OgreRenderer &mRend;
        Ogre::SceneNode* mMwRoot;

        CellSceneNodeMap mCellSceneNodes;
        PtrAnimationMap mAllActors;

    public:
        Actors(OEngine::Render::OgreRenderer& _rend): mRend(_rend) {}
        ~Actors();

        void setMwRoot(Ogre::SceneNode* root);
        void insertBegin (const MWWorld::Ptr& ptr);
        void insertNPC(const MWWorld::Ptr& ptr, MWWorld::InventoryStore& inv);
        void insertCreature (const MWWorld::Ptr& ptr);
        void insertActivator (const MWWorld::Ptr& ptr);
         bool deleteObject (const MWWorld::Ptr& ptr);
        ///< \return found?

        void removeCell(MWWorld::CellStore* store);

        void update (float duration);

        /// Updates containing cell for object rendering data
        void updateObjectCell(const MWWorld::Ptr &old, const MWWorld::Ptr &cur);

        Animation* getAnimation(const MWWorld::Ptr &ptr);
    };
}
#endif
