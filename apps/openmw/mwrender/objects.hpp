#ifndef GAME_RENDER_OBJECTS_H
#define GAME_RENDER_OBJECTS_H

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

    static int uniqueID;

    void insertBegin(const MWWorld::Ptr& ptr);



public:
    Objects(OEngine::Render::OgreRenderer &renderer)
        : mRenderer(renderer)
        , mRootNode(NULL)
    {}
    ~Objects(){}
    void insertModel(const MWWorld::Ptr& ptr, const std::string &model, bool batch=false);

    ObjectAnimation* getAnimation(const MWWorld::Ptr &ptr);

    void update (float dt, Ogre::Camera* camera);
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
