#ifndef _GAME_RENDER_ACTORS_H
#define _GAME_RENDER_ACTORS_H

#include "npcanimation.hpp"
#include "creatureanimation.hpp"

namespace MWWorld
{
    class Ptr;
    class CellStore;
}

namespace MWRender{
    class Actors{
        OEngine::Render::OgreRenderer &mRend;
        std::map<MWWorld::CellStore *, Ogre::SceneNode *> mCellSceneNodes;
        Ogre::SceneNode* mMwRoot;
		std::map<MWWorld::Ptr, Animation*> mAllActors;



        public:
        Actors(OEngine::Render::OgreRenderer& _rend): mRend(_rend) {}
        ~Actors();
        void setMwRoot(Ogre::SceneNode* root);
        void insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_);
        void insertCreature (const MWWorld::Ptr& ptr);
        void insertNPC(const MWWorld::Ptr& ptr, MWWorld::InventoryStore& inv);
         bool deleteObject (const MWWorld::Ptr& ptr);
        ///< \return found?

        void removeCell(MWWorld::CellStore* store);

        void playAnimationGroup (const MWWorld::Ptr& ptr, const std::string& groupName, int mode,
        int number = 1);
        ///< Run animation for a MW-reference. Calls to this function for references that are currently not
        /// in the rendered scene should be ignored.
        ///
        /// \param mode: 0 normal, 1 immediate start, 2 immediate loop
        /// \param number How offen the animation should be run

        void skipAnimation (const MWWorld::Ptr& ptr);
        ///< Skip the animation for the given MW-reference for one frame. Calls to this function for
        /// references that are currently not in the rendered scene should be ignored.

        void update (float duration);

    };
}
#endif
