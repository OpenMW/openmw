#ifndef _GAME_RENDER_OBJECTS_H
#define _GAME_RENDER_OBJECTS_H

#include <openengine/ogre/renderer.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"

namespace MWRender{

class Objects{
    OEngine::Render::OgreRenderer &mRenderer;
    std::map<MWWorld::Ptr::CellStore *, Ogre::SceneNode *> mCellSceneNodes;
    std::map<MWWorld::Ptr::CellStore *, Ogre::StaticGeometry*> mStaticGeometry;
    Ogre::SceneNode* mMwRoot;
    bool mIsStatic;
    static int uniqueID;
    static bool lightConst;
    static float lightConstValue;

    static bool lightLinear;
    static int lightLinearMethod;
    static float lightLinearValue;
    static float lightLinearRadiusMult;

    static bool lightQuadratic;
    static int lightQuadraticMethod;
    static float lightQuadraticValue;
    static float lightQuadraticRadiusMult;

    static bool lightOutQuadInLin;

    void clearSceneNode (Ogre::SceneNode *node);
    ///< Remove all movable objects from \a node.

public:
    Objects(OEngine::Render::OgreRenderer& renderer): mRenderer (renderer){}
    ~Objects(){}
    void insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_);
    void insertMesh (const MWWorld::Ptr& ptr, const std::string& mesh);
    void insertLight (const MWWorld::Ptr& ptr, float r, float g, float b, float radius);

    bool deleteObject (const MWWorld::Ptr& ptr);
    ///< \return found?

    void removeCell(MWWorld::Ptr::CellStore* store);
    void buildStaticGeometry(ESMS::CellStore<MWWorld::RefData> &cell);
    void setMwRoot(Ogre::SceneNode* root);
};
}
#endif
