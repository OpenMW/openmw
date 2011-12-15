#ifndef _GAME_RENDER_ACTORS_H
#define _GAME_RENDER_ACTORS_H

#include "components/esm_store/cell_store.hpp"
#include <map>
#include <list>



#include <openengine/ogre/renderer.hpp>
#include "components/nifogre/ogre_nif_loader.hpp"

#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/environment.hpp"
#include "npcanimation.hpp"
#include "creatureanimation.hpp"

namespace MWRender{
    class Actors{
        OEngine::Render::OgreRenderer &mRend;
        std::map<MWWorld::Ptr::CellStore *, Ogre::SceneNode *> mCellSceneNodes;
        Ogre::SceneNode* mMwRoot;
        MWWorld::Environment& mEnvironment;
        std::list<Animation*> mAllActors;

        

        public:
        Actors(OEngine::Render::OgreRenderer& _rend, MWWorld::Environment& _env): mRend(_rend), mEnvironment(_env){}
        ~Actors(){}
        void setMwRoot(Ogre::SceneNode* root);
        void insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_);
        void insertCreature (const MWWorld::Ptr& ptr);
        void insertNPC(const MWWorld::Ptr& ptr);
         bool deleteObject (const MWWorld::Ptr& ptr);
        ///< \return found?

        void removeCell(MWWorld::Ptr::CellStore* store);
        
    };
}
#endif