#ifndef _GAME_RENDER_EXTERIOR_H
#define _GAME_RENDER_EXTERIOR_H

#include "cell.hpp"
#include "cellimp.hpp"

#include "OgreColourValue.h"
#include <OgreMath.h>
#include <Ogre.h>

namespace Ogre
{
  class SceneNode;
}

namespace MWWorld
{
    class Environment;
}

namespace MWRender
{
  class MWScene;

  /**
     This class is responsible for inserting meshes and other
     rendering objects from the given cell into the given rendering
     scene.
   */

  class ExteriorCellRender : public CellRender, private CellRenderImp
  {

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

    ESMS::CellStore<MWWorld::RefData> &mCell;
    MWWorld::Environment &mEnvironment;
    MWScene &mScene;

    /// The scene node that contains all objects belonging to this
    /// cell.
    Ogre::SceneNode *mBase;

    Ogre::SceneNode *mInsert;
    std::string mInsertMesh;
    Ogre::SceneNode *mNpcPart;

    //the static geometry
    Ogre::StaticGeometry *sg;
    bool isStatic;

    // 0 normal, 1 more bright, 2 max
    int mAmbientMode;

    Ogre::ColourValue mAmbientColor;

    /// start inserting a new reference.
    virtual void insertBegin (ESM::CellRef &ref, bool static_ = false);

    /// insert a mesh related to the most recent insertBegin call.
    virtual void insertMesh(const std::string &mesh, Ogre::Vector3 vec,  Ogre::Vector3 axis, Ogre::Radian angle, std::string sceneNodeName, std::string sceneParent[], int elements);
    virtual void insertMesh(const std::string &mesh, Ogre::Vector3 vec, Ogre::Vector3 axis, Ogre::Radian angle, std::string sceneNodeName, std::string sceneParent[], int elements, bool translateFirst);

    virtual void insertMesh(const std::string &mesh);

     virtual void rotateMesh(Ogre::Vector3 axis, Ogre::Radian angle,  std::string sceneNodeName[], int elements);
     virtual void scaleMesh(Ogre::Vector3 axis,  std::string sceneNodeName[], int elements);

    virtual void insertObjectPhysics();

    virtual void insertActorPhysics();

    /// insert a light related to the most recent insertBegin call.
    virtual void insertLight(float r, float g, float b, float radius);

    /// finish inserting a new reference and return a handle to it.
    virtual std::string insertEnd (bool Enable);

    /// configure lighting according to cell
    void configureAmbient();

    /// configure fog according to cell
    void configureFog();

    void setAmbientMode();


  public:

    ExteriorCellRender(ESMS::CellStore<MWWorld::RefData> &_cell, MWWorld::Environment& environment,
        MWScene &_scene);

    virtual ~ExteriorCellRender() { destroy(); }

    /// Make the cell visible. Load the cell if necessary.
    virtual void show();

    /// Remove the cell from rendering, but don't remove it from
    /// memory.
    virtual void hide();

    /// Destroy all rendering objects connected with this cell.
    virtual void destroy(); // comment by Zini: shouldn't this go into the destructor?

    /// Switch through lighting modes.
    void toggleLight();

    /// Make the reference with the given handle visible.
    virtual void enable (const std::string& handle);

    /// Make the reference with the given handle invisible.
    virtual void disable (const std::string& handle);

    /// Remove the reference with the given handle permanently from the scene.
    virtual void deleteObject (const std::string& handle);

    void destroyAllAttachedMovableObjects(Ogre::SceneNode* i_pSceneNode);

    static int uniqueID;
  };
}

#endif
