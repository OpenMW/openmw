#ifndef _GAME_RENDER_ACTORS_H
#define _GAME_RENDER_ACTORS_H

#include "components/esm_store/cell_store.hpp"
#include <map>

#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"
#include <openengine/ogre/renderer.hpp>
namespace MWRender{
    class Actors{
        OEngine::Render::OgreRenderer &mRend;
        std::map<MWWorld::Ptr::CellStore *, Ogre::SceneNode *> mCellSceneNodes;
        Ogre::SceneNode* mMwRoot;

        

        public:
        Actors(OEngine::Render::OgreRenderer& _rend): mRend(_rend){}
        ~Actors(){}
        void setMwRoot(Ogre::SceneNode* root);
        void insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_);
        void insertMesh (const MWWorld::Ptr& ptr, const std::string& mesh);
         bool deleteObject (const MWWorld::Ptr& ptr);
        ///< \return found?

        void removeCell(MWWorld::Ptr::CellStore* store);
        
    };
}
#endif