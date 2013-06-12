#ifndef _GAME_RENDER_OBJECTS_H
#define _GAME_RENDER_OBJECTS_H

#include <OgreColourValue.h>
#include <OgreAxisAlignedBox.h>

#include <openengine/ogre/renderer.hpp>
#include "../mwworld/fallback.hpp"

namespace MWWorld
{
    class Ptr;
    class CellStore;
}

namespace MWRender{

/// information about light needed for rendering
enum LightType
{
    // These are all mutually exclusive
    LT_Normal=0,
    LT_Flicker=1,
    LT_FlickerSlow=2,
    LT_Pulse=3,
    LT_PulseSlow=4
};

struct LightInfo
{
    // Constants
    std::string name; // ogre handle
    Ogre::ColourValue colour;
    float radius;
    bool interior; // Does this light belong to an interior or exterior cell
    LightType type;

    // Runtime variables
    float dir;   // direction time is running...
    float time;  // current time
    float phase; // current phase

    LightInfo() :
        dir(1.0f), time(0.0f), phase (0.0f),
        interior(true), type(LT_Normal), radius(1.0)
    {
    }
};

class Objects{
    OEngine::Render::OgreRenderer &mRenderer;
    std::map<MWWorld::CellStore *, Ogre::SceneNode *> mCellSceneNodes;
    std::map<MWWorld::CellStore *, Ogre::StaticGeometry*> mStaticGeometry;
    std::map<MWWorld::CellStore *, Ogre::StaticGeometry*> mStaticGeometrySmall;
    std::map<MWWorld::CellStore *, Ogre::AxisAlignedBox> mBounds;
    std::vector<LightInfo> mLights;
    Ogre::SceneNode* mRootNode;
    bool mIsStatic;
    static int uniqueID;
    MWWorld::Fallback* mFallback;
    float lightLinearValue();
    float lightLinearRadiusMult();

    bool lightQuadratic();
    float lightQuadraticValue();
    float lightQuadraticRadiusMult();

    bool lightOutQuadInLin();

    void clearSceneNode (Ogre::SceneNode *node);
    ///< Remove all movable objects from \a node.

public:
    Objects(OEngine::Render::OgreRenderer& renderer, MWWorld::Fallback* fallback): mRenderer (renderer), mIsStatic(false), mFallback(fallback) {}
    ~Objects(){}
    void insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_);
    void insertMesh (const MWWorld::Ptr& ptr, const std::string& mesh, bool light=false);
    void insertLight (const MWWorld::Ptr& ptr, Ogre::Entity *skelBase=0, Ogre::Vector3 fallbackCenter=Ogre::Vector3(0.0f));

    void enableLights();
    void disableLights();

    void update (const float dt);
    ///< per-frame update

    Ogre::AxisAlignedBox getDimensions(MWWorld::CellStore*);
    ///< get a bounding box that encloses all objects in the specified cell

    bool deleteObject (const MWWorld::Ptr& ptr);
    ///< \return found?

    void removeCell(MWWorld::CellStore* store);
    void buildStaticGeometry(MWWorld::CellStore &cell);
    void setRootNode(Ogre::SceneNode* root);

    void rebuildStaticGeometry();

    /// Updates containing cell for object rendering data
    void updateObjectCell(const MWWorld::Ptr &old, const MWWorld::Ptr &cur);
};
}
#endif
