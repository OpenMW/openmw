#include "interior.hpp"


#include <OgreEntity.h>

#include "nifogre/ogre_nif_loader.hpp"

using namespace MWRender;
using namespace Ogre;
using namespace ESMS;

// start inserting a new reference.

void InteriorCellRender::insertBegin (const ESMS::CellRef &ref)
{
  assert (!insert);
    
  // Create and place scene node for this object
  insert = base->createChildSceneNode();

  const float *f = ref.pos.pos;
  insert->setPosition(f[0], f[1], f[2]);
  insert->setScale(ref.scale, ref.scale, ref.scale);

  // Convert MW rotation to a quaternion:
  f = ref.pos.rot;

  // Rotate around X axis
  Quaternion xr(Radian(-f[0]), Vector3::UNIT_X);

  // Rotate around Y axis
  Quaternion yr(Radian(-f[1]), Vector3::UNIT_Y);

  // Rotate around Z axis
  Quaternion zr(Radian(-f[2]), Vector3::UNIT_Z);

  // Rotates first around z, then y, then x
  insert->setOrientation(xr*yr*zr);
}

// insert a mesh related to the most recent insertBegin call.

void InteriorCellRender::insertMesh(const std::string &mesh)
{
  assert (insert);
    
  NIFLoader::load(mesh);
  MovableObject *ent = scene.getMgr()->createEntity(mesh);
  insert->attachObject(ent);
}

// finish inserting a new reference and return a handle to it.

std::string InteriorCellRender::insertEnd()
{
  assert (insert);

  std::string handle = insert->getName();
  
  insert = 0;
  
  return handle;
}     
           
void InteriorCellRender::show()
{
  // If already loaded, just make the cell visible.
  if(base)
    {
      base->setVisible(true);
      return;
    }

  base = scene.getRoot()->createChildSceneNode();

  insertCell(cell);
}

void InteriorCellRender::hide()
{
  if(base)
    base->setVisible(false);
}

void InteriorCellRender::destroy()
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
