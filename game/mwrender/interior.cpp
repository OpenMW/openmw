#include "interior.hpp"

#include <OgreEntity.h>
#include <OgreLight.h>

#include "nifogre/ogre_nif_loader.hpp"
#include "mwscene.hpp"

using namespace MWRender;
using namespace Ogre;
using namespace ESMS;

bool InteriorCellRender::lightConst = false;
float InteriorCellRender::lightConstValue = 0.0;

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

void InteriorCellRender::insertBegin (const ESM::CellRef &ref)
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

std::string InteriorCellRender::insertEnd()
{
  assert (insert);

  std::string handle = insert->getName();
  
  insert = 0;
  
  return handle;
}     
           
// configure lighting according to cell

void InteriorCellRender::configureAmbient()
{
  ambientColor.setAsRGBA (cell.cell->ambi.ambient);
  setAmbientMode();
  
  if (cell.cell->data.flags & ESM::Cell::QuasiEx)
  {
    // Create a "sun" that shines light downwards. It doesn't look
    // completely right, but leave it for now.
    Ogre::Light *light = scene.getMgr()->createLight();
    Ogre::ColourValue colour;
    colour.setAsRGBA (cell.cell->ambi.sunlight);
    light->setDiffuseColour (colour);
    light->setType(Ogre::Light::LT_DIRECTIONAL);
    light->setDirection(0,-1,0);
    // TODO: update position on regular basis or attach to camera scene node
  }
}
                
// configure fog according to cell

void InteriorCellRender::configureFog()
{
  Ogre::ColourValue color;
  color.setAsRGBA (cell.cell->ambi.fog);
  
  float high = 10000;
  float low = 8000;
  
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

      scene.getMgr()->setAmbientLight(0.7*ambientColor + 0.3*ColourValue(1,1,1));
      break;
          
    case 2:
  
      scene.getMgr()->setAmbientLight(ColourValue(1,1,1));
      break;
  }
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

  configureAmbient();
  configureFog();

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
