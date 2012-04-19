#ifndef _GAME_RENDER_OBJECTS_H
#define _GAME_RENDER_OBJECTS_H

#include <openengine/ogre/renderer.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"

namespace MWRender{

/// information about light needed for rendering
struct LightInfo
{
    std::string name; // ogre handle
    Ogre::ColourValue colour;
    float radius;
};

class Objects{
    OEngine::Render::OgreRenderer &mRenderer;
    std::map<MWWorld::Ptr::CellStore *, Ogre::SceneNode *> mCellSceneNodes;
    std::map<MWWorld::Ptr::CellStore *, Ogre::StaticGeometry*> mStaticGeometry;
    std::map<MWWorld::Ptr::CellStore *, Ogre::StaticGeometry*> mStaticGeometrySmall;
    std::map<MWWorld::Ptr::CellStore *, Ogre::AxisAlignedBox> mBounds;
    std::vector<LightInfo> mLights;
    Ogre::SceneNode* mMwRoot;
    bool mIsStatic;
    static int uniqueID;

    static float lightLinearValue;
    static float lightLinearRadiusMult;

    static bool lightQuadratic;
    static float lightQuadraticValue;
    static float lightQuadraticRadiusMult;

    static bool lightOutQuadInLin;

    bool mInterior;

    void clearSceneNode (Ogre::SceneNode *node);
    ///< Remove all movable objects from \a node.

public:
    Objects(OEngine::Render::OgreRenderer& renderer): mRenderer (renderer), mInterior(true) {}
    ~Objects(){}
    void insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_);
    void insertMesh (const MWWorld::Ptr& ptr, const std::string& mesh);
    void insertLight (const MWWorld::Ptr& ptr, float r, float g, float b, float radius);

    void enableLights();
    void disableLights();

    void update (const float dt);
    ///< per-frame update

    void setInterior(const bool interior);
    ///< call this to switch from interior to exterior or vice versa

    Ogre::AxisAlignedBox getDimensions(MWWorld::Ptr::CellStore*);
    ///< get a bounding box that encloses all objects in the specified cell

    bool deleteObject (const MWWorld::Ptr& ptr);
    ///< \return found?

    void removeCell(MWWorld::Ptr::CellStore* store);
    void buildStaticGeometry(ESMS::CellStore<MWWorld::RefData> &cell);
    void setMwRoot(Ogre::SceneNode* root);
};
}
#endif
