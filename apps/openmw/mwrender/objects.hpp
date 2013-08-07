#ifndef _GAME_RENDER_OBJECTS_H
#define _GAME_RENDER_OBJECTS_H

#include <OgreColourValue.h>
#include <OgreAxisAlignedBox.h>

#include <openengine/ogre/renderer.hpp>

namespace MWWorld
{
    class Ptr;
    class CellStore;
}

namespace MWRender{

class ObjectAnimation;

class Objects{
    typedef std::map<MWWorld::Ptr,ObjectAnimation*> PtrAnimationMap;

    OEngine::Render::OgreRenderer &mRenderer;

    std::map<MWWorld::CellStore*,Ogre::SceneNode*> mCellSceneNodes;
    std::map<MWWorld::CellStore*,Ogre::StaticGeometry*> mStaticGeometry;
    std::map<MWWorld::CellStore*,Ogre::StaticGeometry*> mStaticGeometrySmall;
    std::map<MWWorld::CellStore*,Ogre::AxisAlignedBox> mBounds;
    PtrAnimationMap mObjects;

    Ogre::SceneNode* mRootNode;
    bool mIsStatic;
    static int uniqueID;

public:
    Objects(OEngine::Render::OgreRenderer &renderer)
        : mRenderer(renderer)
        , mIsStatic(false)
        , mRootNode(NULL)
    {}
    ~Objects(){}
    void insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_);
    void insertMesh (const MWWorld::Ptr& ptr, const std::string& mesh);

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
