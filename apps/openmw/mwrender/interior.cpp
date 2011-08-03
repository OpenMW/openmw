#include "interior.hpp"

#include <OgreEntity.h>
#include <OgreLight.h>
#include <OgreSceneNode.h>
#include <OgreCamera.h>
#include <OgreSceneManager.h>
#include <OgreMath.h>

#include <components/nifogre/ogre_nif_loader.hpp>
#include "mwscene.hpp"
#include <Ogre.h>
#include <stdio.h>

#include <libs/mangle/vfs/servers/ogre_vfs.hpp>

using namespace MWRender;
using namespace Ogre;
using namespace ESMS;

bool InteriorCellRender::lightConst = false;
float InteriorCellRender::lightConstValue = 0.0f;

bool InteriorCellRender::lightLinear = true;
int InteriorCellRender::lightLinearMethod = 1;
float InteriorCellRender::lightLinearValue = 3;
float InteriorCellRender::lightLinearRadiusMult = 1;

bool InteriorCellRender::lightQuadratic = false;
int InteriorCellRender::lightQuadraticMethod = 2;
float InteriorCellRender::lightQuadraticValue = 16;
float InteriorCellRender::lightQuadraticRadiusMult = 1;

bool InteriorCellRender::lightOutQuadInLin = false;

// start inserting a new reference.

void InteriorCellRender::insertBegin (ESM::CellRef &ref, bool static_)
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

    mInsertMesh.clear();
}

// insert a mesh related to the most recent insertBegin call.
void InteriorCellRender::rotateMesh(Ogre::Vector3 axis, Ogre::Radian angle,  std::string sceneNodeName[], int elements)
{
    assert(insert);
    Ogre::SceneNode *parent = insert;
     //std::cout << "ELEMENTS:" << elements;
    for (int i = 0; i < elements; i++){
       if(sceneNodeName[i] != "" && parent->getChild(sceneNodeName[i]))
           parent = dynamic_cast<Ogre::SceneNode*> (parent->getChild(sceneNodeName[i]));
    }
       parent->rotate(axis, angle);
}
// insert a mesh related to the most recent insertBegin call.

void InteriorCellRender::scaleMesh(Ogre::Vector3 axis,  std::string sceneNodeName[], int elements)
{
    assert(insert);
    Ogre::SceneNode *parent = insert;
     //std::cout << "ELEMENTS:" << elements;
    for (int i = 0; i < elements; i++){
       if(sceneNodeName[i] != "" && parent->getChild(sceneNodeName[i]))
           parent = dynamic_cast<Ogre::SceneNode*> (parent->getChild(sceneNodeName[i]));
    }
       parent->scale(axis);
}
void InteriorCellRender::insertMesh(const std::string &mesh, Ogre::Vector3 vec, Ogre::Vector3 axis, Ogre::Radian angle,  std::string sceneNodeName, std::string sceneParent[], int elements)
{
    insertMesh(mesh, vec, axis, angle, sceneNodeName,  sceneParent, elements, true);
}
void InteriorCellRender::insertMesh(const std::string &mesh, Ogre::Vector3 vec, Ogre::Vector3 axis, Ogre::Radian angle,  std::string sceneNodeName, std::string sceneParent[], int elements, bool translateFirst){

       assert (insert);
     //insert->
       Ogre::SceneNode *parent = insert;
       for (int i = 0; i < elements; i++){
       if(sceneParent[i] != "" && parent->getChild(sceneParent[i]))
           parent = dynamic_cast<Ogre::SceneNode*> (parent->getChild(sceneParent[i]));
        }

     npcPart = parent->createChildSceneNode(sceneNodeName);
    //npcPart->showBoundingBox(true);

  MeshPtr good2 = NifOgre::NIFLoader::load(mesh);

  MovableObject *ent = scene.getMgr()->createEntity(mesh);
  //ent->extr

    //  MovableObject *ent2 = scene.getMgr()->createEntity(bounds
        //      );
 //ent->
    //std::cout << mesh << bounds << "\n";

  if(translateFirst){
  npcPart->translate(vec);
  npcPart->rotate(axis, angle);
  }
  else{

  npcPart->rotate(axis, angle);
  npcPart->translate(vec);
  }

  npcPart->attachObject(ent);
   Ogre::MeshManager *m = MeshManager::getSingletonPtr();
  const std::string beast1 ="meshes\\b\\B_N_Khajiit_F_Skins.nif";
    const std::string beast2 ="meshes\\b\\B_N_Khajiit_M_Skins.nif";
    const std::string beast3 ="meshes\\b\\B_N_Argonian_F_Skins.nif";
    const std::string beast4 ="meshes\\b\\B_N_Argonian_M_Skins.nif";

    const std::string beasttail1 ="tail\\b\\B_N_Khajiit_F_Skins.nif";
    const std::string beasttail2 ="tail\\b\\B_N_Khajiit_M_Skins.nif";
    const std::string beasttail3 ="tail\\b\\B_N_Argonian_F_Skins.nif";
    const std::string beasttail4 ="tail\\b\\B_N_Argonian_M_Skins.nif";

    const std::string beastfoot1 ="foot\\b\\B_N_Khajiit_F_Skins.nif";
    const std::string beastfoot2 ="foot\\b\\B_N_Khajiit_M_Skins.nif";
    const std::string beastfoot3 ="foot\\b\\B_N_Argonian_F_Skins.nif";
    const std::string beastfoot4 ="foot\\b\\B_N_Argonian_M_Skins.nif";
    if(mesh.compare(beast1) == 0 && m->getByName(beasttail1).isNull())
        {
            //std::cout << "CLONINGKHAJIITF\n";
            good2->reload();
            MeshPtr tail = good2->clone(beasttail1);
            good2->reload();
            MeshPtr foot = good2->clone(beastfoot1);
            good2->reload();
        }
        else if(mesh.compare(beast2) == 0 && m->getByName(beasttail2).isNull())
        {
            //std::cout << "CLONINGKHAJIITM\n";
            good2->reload();
            MeshPtr tail = good2->clone(beasttail2);
            good2->reload();
            MeshPtr foot = good2->clone(beastfoot2);
            good2->reload();
        }
        else if(mesh.compare(beast3) == 0 && m->getByName(beasttail3).isNull())
        {
            //std::cout << "CLONINGARGONIANF\n";
            good2->reload();
            MeshPtr tail = good2->clone(beasttail3);
            good2->reload();
            MeshPtr foot = good2->clone(beastfoot3);
            good2->reload();
        }
        else if(mesh.compare(beast4) == 0 && m->getByName(beasttail4).isNull())
        {
            //std::cout << "CLONINGARGONIANM\n";
            good2->reload();
            MeshPtr tail = good2->clone(beasttail4);
            good2->reload();
            MeshPtr foot = good2->clone(beastfoot4);
            good2->reload();
        }
}

void InteriorCellRender::insertMesh(const std::string &mesh)
{
    assert (insert);

    NifOgre::NIFLoader::load(mesh);
    MovableObject *ent = scene.getMgr()->createEntity(mesh);
    insert->attachObject(ent);

    if (mInsertMesh.empty())
        mInsertMesh = mesh;
}

void InteriorCellRender::insertObjectPhysics()
{
    if (!mInsertMesh.empty())
        scene.addObject (insert->getName(), mInsertMesh, insert->getOrientation(),
            insert->getScale().x, insert->getPosition());
}

void InteriorCellRender::insertActorPhysics()
{
    scene.addActor (insert->getName(), mInsertMesh, insert->getPosition());
}

// insert a light related to the most recent insertBegin call.
void InteriorCellRender::insertLight(float r, float g, float b, float radius)
{
  assert (insert);

  Ogre::Light *light = scene.getMgr()->createLight();
  light->setDiffuseColour (r, g, b);

  float cval=0.0f, lval=0.0f, qval=0.0f;

  if(lightConst)
    cval = lightConstValue;
  if(!lightOutQuadInLin)
  {
    if(lightLinear)
      radius *= lightLinearRadiusMult;
    if(lightQuadratic)
      radius *= lightQuadraticRadiusMult;

    if(lightLinear)
      lval = lightLinearValue / pow(radius, lightLinearMethod);
    if(lightQuadratic)
      qval = lightQuadraticValue / pow(radius, lightQuadraticMethod);
  }
  else
  {
    // FIXME:
    // Do quadratic or linear, depending if we're in an exterior or interior
    // cell, respectively. Ignore lightLinear and lightQuadratic.
  }

  light->setAttenuation(10*radius, cval, lval, qval);

  insert->attachObject(light);
}

// finish inserting a new reference and return a handle to it.

std::string InteriorCellRender::insertEnd (bool enable)
{
  assert (insert);

  std::string handle = insert->getName();

  if (!enable)
    insert->setVisible (false);

  insert = 0;

  return handle;
}

// configure lighting according to cell

void InteriorCellRender::configureAmbient()
{
  ambientColor.setAsABGR (cell.cell->ambi.ambient);
  setAmbientMode();

  // Create a "sun" that shines light downwards. It doesn't look
  // completely right, but leave it for now.
  Ogre::Light *light = scene.getMgr()->createLight();
  Ogre::ColourValue colour;
  colour.setAsABGR (cell.cell->ambi.sunlight);
  light->setDiffuseColour (colour);
  light->setType(Ogre::Light::LT_DIRECTIONAL);
  light->setDirection(0,-1,0);
}

// configure fog according to cell
void InteriorCellRender::configureFog()
{
  Ogre::ColourValue color;
  color.setAsABGR (cell.cell->ambi.fog);

  float high = 4500 + 9000 * (1-cell.cell->ambi.fogDensity);
  float low = 200;

  scene.getMgr()->setFog (FOG_LINEAR, color, 0, low, high);
  scene.getCamera()->setFarClipDistance (high + 10);
  scene.getViewport()->setBackgroundColour (color);
}

void InteriorCellRender::setAmbientMode()
{
  switch (ambientMode)
  {
    case 0:

      scene.getMgr()->setAmbientLight(ambientColor);
      break;

    case 1:

      scene.getMgr()->setAmbientLight(0.7f*ambientColor + 0.3f*ColourValue(1,1,1));
      break;

    case 2:

      scene.getMgr()->setAmbientLight(ColourValue(1,1,1));
      break;
  }
}

void InteriorCellRender::show()
{
  base = scene.getRoot()->createChildSceneNode();

  configureAmbient();
  configureFog();

  insertCell(cell, mEnvironment);
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

// Switch through lighting modes.

void InteriorCellRender::toggleLight()
{
  if (ambientMode==2)
    ambientMode = 0;
  else
    ++ambientMode;

  switch (ambientMode)
  {
    case 0: std::cout << "Setting lights to normal\n"; break;
    case 1: std::cout << "Turning the lights up\n"; break;
    case 2: std::cout << "Turning the lights to full\n"; break;
  }

  setAmbientMode();
}

void InteriorCellRender::enable (const std::string& handle)
{
    if (!handle.empty())
        scene.getMgr()->getSceneNode (handle)->setVisible (true);
}

void InteriorCellRender::disable (const std::string& handle)
{
    if (!handle.empty())
        scene.getMgr()->getSceneNode (handle)->setVisible (false);
}

void InteriorCellRender::deleteObject (const std::string& handle)
{
    if (!handle.empty())
    {
        Ogre::SceneNode *node = scene.getMgr()->getSceneNode (handle);
        node->removeAndDestroyAllChildren();
        scene.getMgr()->destroySceneNode (node);
    }
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
