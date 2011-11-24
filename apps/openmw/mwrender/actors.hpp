#ifndef _GAME_RENDER_ACTORS_H
#define _GAME_RENDER_ACTORS_H

#include "components/esm_store/cell_store.hpp"

#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"
#include <openengine/ogre/renderer.hpp>
namespace MWRender{
    class Actors{
        OEngine::Render::OgreRenderer &mRend;
        std::map<MWWorld::Ptr::CellStore *, Ogre::SceneNode *> mCellSceneNodes;
        std::map<MWWorld::Ptr::CellStore *, Ogre::StaticGeometry*> mSG;
        Ogre::SceneNode* mMwRoot;
        bool isStatic;
        static int uniqueID;

        public:
              Actors(OEngine::Render::OgreRenderer& _rend): mRend(_rend){}
              ~Actors(){}
    };
}
#endif