#include "exterior.hpp"

#include <OgreEntity.h>
#include <OgreLight.h>
#include <OgreSceneNode.h>
#include <OgreCamera.h>
#include <OgreSceneManager.h>

#include <components/nifogre/ogre_nif_loader.hpp>
#include "mwscene.hpp"

using namespace MWRender;
using namespace Ogre;
using namespace ESMS;

bool ExteriorCellRender::lightConst = false;
float ExteriorCellRender::lightConstValue = 0.0f;

bool ExteriorCellRender::lightLinear = true;
int ExteriorCellRender::lightLinearMethod = 1;
float ExteriorCellRender::lightLinearValue = 3;
float ExteriorCellRender::lightLinearRadiusMult = 1;

bool ExteriorCellRender::lightQuadratic = false;
int ExteriorCellRender::lightQuadraticMethod = 2;
float ExteriorCellRender::lightQuadraticValue = 16;
float ExteriorCellRender::lightQuadraticRadiusMult = 1;

bool ExteriorCellRender::lightOutQuadInLin = false;

// start inserting a new reference.

void ExteriorCellRender::insertBegin (ESM::CellRef &ref)
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

void ExteriorCellRender::insertMesh(const std::string &mesh)
{
  assert (insert);

  NIFLoader::load(mesh);
  MovableObject *ent = scene.getMgr()->createEntity(mesh);
  insert->attachObject(ent);
}

// insert a light related to the most recent insertBegin call.
void ExteriorCellRender::insertLight(float r, float g, float b, float radius)
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

std::string ExteriorCellRender::insertEnd (bool enable)
{
  assert (insert);

  std::string handle = insert->getName();

  if (!enable)
    insert->setVisible (false);

  insert = 0;

  return handle;
}

// configure lighting according to cell

void ExteriorCellRender::configureAmbient()
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
void ExteriorCellRender::configureFog()
{
  Ogre::ColourValue color;
  color.setAsABGR (cell.cell->ambi.fog);

  float high = 4500 + 9000 * (1-cell.cell->ambi.fogDensity);
  float low = 200;

  scene.getMgr()->setFog (FOG_LINEAR, color, 0, low, high);
  scene.getCamera()->setFarClipDistance (high + 10);
  scene.getViewport()->setBackgroundColour (color);
}

void ExteriorCellRender::setAmbientMode()
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

void ExteriorCellRender::show()
{
  base = scene.getRoot()->createChildSceneNode();

  configureAmbient();
  configureFog();

  insertCell(cell, mEnvironment);
}

void ExteriorCellRender::hide()
{
  if(base)
    base->setVisible(false);
}

void ExteriorCellRender::destroy()
{
  if(base)
    {
      base->removeAndDestroyAllChildren();
      scene.getMgr()->destroySceneNode(base);
    }

  base = NULL;
}

// Switch through lighting modes.

void ExteriorCellRender::toggleLight()
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

void ExteriorCellRender::enable (const std::string& handle)
{
    if (!handle.empty())
        scene.getMgr()->getSceneNode (handle)->setVisible (true);
}

void ExteriorCellRender::disable (const std::string& handle)
{
    if (!handle.empty())
        scene.getMgr()->getSceneNode (handle)->setVisible (false);
}

void ExteriorCellRender::deleteObject (const std::string& handle)
{
    if (!handle.empty())
    {
        Ogre::SceneNode *node = scene.getMgr()->getSceneNode (handle);
        node->removeAndDestroyAllChildren();
        scene.getMgr()->destroySceneNode (node);
    }
}
