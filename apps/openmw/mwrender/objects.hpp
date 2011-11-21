#ifndef _GAME_RENDER_OBJECTS_H
#define _GAME_RENDER_OBJECTS_H

#include "components/esm_store/cell_store.hpp"

#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"
#include <openengine/ogre/renderer.hpp>

namespace MWRender{

class Objects{
    OEngine::Render::OgreRenderer &mRend;
    std::map<MWWorld::Ptr::CellStore *, Ogre::SceneNode *> mCellSceneNodes;
    std::map<MWWorld::Ptr::CellStore *, Ogre::StaticGeometry*> mSG;
    Ogre::SceneNode* mwRoot;
    bool isStatic;
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
public:
    Objects(OEngine::Render::OgreRenderer& _rend): mRend(_rend){}
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
