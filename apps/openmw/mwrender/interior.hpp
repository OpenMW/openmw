#ifndef _GAME_RENDER_INTERIOR_H
#define _GAME_RENDER_INTERIOR_H

#include "cell.hpp"
#include "cellimp.hpp"
#include "components/esm_store/cell_store.hpp"

#include "OgreColourValue.h"

namespace Ogre
{
  class SceneNode;
}

namespace MWRender
{
  class MWScene;

  /**
     This class is responsible for inserting meshes and other
     rendering objects from the given cell into the given rendering
     scene.

     TODO FIXME: Doesn't do full cleanup yet.
   */

  class InteriorCellRender : public CellRender, private CellRenderImp
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

    ESMS::CellStore<MWWorld::RefData> &cell;
    const ESMS::ESMStore &store;
    MWScene &scene;

    /// The scene node that contains all objects belonging to this
    /// cell.
    Ogre::SceneNode *base;

    Ogre::SceneNode *insert;

    // 0 normal, 1 more bright, 2 max
    int ambientMode;

    Ogre::ColourValue ambientColor;

    /// start inserting a new reference.
    virtual void insertBegin (ESM::CellRef &ref);

    /// insert a mesh related to the most recent insertBegin call.
    virtual void insertMesh(const std::string &mesh);

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

    InteriorCellRender(ESMS::CellStore<MWWorld::RefData> &_cell, const ESMS::ESMStore& _store, MWScene &_scene)
    : cell(_cell), store(_store), scene(_scene), base(NULL), insert(NULL), ambientMode (0) {}

    virtual ~InteriorCellRender() { destroy(); }

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
  };
}

#endif
