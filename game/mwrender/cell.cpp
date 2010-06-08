#include "cell.hpp"

#include <OgreEntity.h>

#include "nifogre/ogre_nif_loader.hpp"

using namespace MWRender;
using namespace Ogre;
using namespace ESMS;

// Inserts one mesh into the scene
static void insertObj(SceneNode *base,          // Parent scene node
                      SceneManager *mgr,        // Scene manager
                      const std::string mesh,   // NIF file
                      const CellRef &ref)       // Reference information
{
  // Create and place scene node for this object
  SceneNode *node = base->createChildSceneNode();

  const float *f = ref.pos.pos;
  node->setPosition(f[0], f[1], f[2]);
  node->setScale(ref.scale, ref.scale, ref.scale);

  // Convert MW rotation to a quaternion:
  f = ref.pos.rot;

  // Rotate around X axis
  Quaternion xr(Radian(-f[0]), Vector3::UNIT_X);

  // Rotate around Y axis
  Quaternion yr(Radian(-f[1]), Vector3::UNIT_Y);

  // Rotate around Z axis
  Quaternion zr(Radian(-f[2]), Vector3::UNIT_Z);

  // Rotates first around z, then y, then x
  node->setOrientation(xr*yr*zr);

  // Finally, load the NIF mesh and attach it to the node
  NIFLoader::load(mesh);
  MovableObject *ent = mgr->createEntity(mesh);
  node->attachObject(ent);
}

void CellRender::show()
{
  // If already loaded, just make the cell visible.
  if(base)
    {
      base->setVisible(true);
      return;
    }

  base = scene.getRoot()->createChildSceneNode();

  // Loop through all statics in the cell
  CellRefList<Static>::List::const_iterator it;
  for(it = cell.statics.list.begin();
      it != cell.statics.list.end();
      it++)
    {
      assert(it->base != NULL);
      insertObj(base, scene.getMgr(),
                "meshes\\" + it->base->model,
                it->ref);
    }

  /*
  // Containers
  CellRefList<Container>::List::const_iterator it;

  for(it = cell.containers.list.begin();
      it != cell.containers.list.end();
      it++)
    {
      assert(it->base != NULL);

      cout << "refID = " << it->ref.refID << " x" << it->ref.scale << endl;
      cout << "flags = " << it->base->flags << endl;
    }
  */
}

void CellRender::hide()
{
  if(base)
    base->setVisible(false);
}

void CellRender::destroy()
{
  if(base)
    {
      base->removeAndDestroyAllChildren();
      scene.getMgr()->destroySceneNode(base);
    }

  base = NULL;
}

// Magic function from the internets. Might need this later.
/*
void Scene::DestroyAllAttachedMovableObjects( SceneNode* i_pSceneNode )
{
   if ( !i_pSceneNode )
   {
      ASSERT( false );
      return;
   }

   // Destroy all the attached objects
   SceneNode::ObjectIterator itObject = i_pSceneNode->getAttachedObjectIterator();

   while ( itObject.hasMoreElements() )
   {
      MovableObject* pObject = static_cast<MovableObject*>(itObject.getNext());
      i_pSceneNode->getCreator()->destroyMovableObject( pObject );
   }

   // Recurse to child SceneNodes
   SceneNode::ChildNodeIterator itChild = i_pSceneNode->getChildIterator();

   while ( itChild.hasMoreElements() )
   {
      SceneNode* pChildNode = static_cast<SceneNode*>(itChild.getNext());
      DestroyAllAttachedMovableObjects( pChildNode );
   }
}
*/
