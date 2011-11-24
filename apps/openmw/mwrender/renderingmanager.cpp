#include "renderingmanager.hpp"

#include <assert.h>

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "OgreCamera.h"
#include "OgreTextureManager.h"

#include "../mwworld/world.hpp" // these includes can be removed once the static-hack is gone
#include "../mwworld/ptr.hpp"
#include <components/esm/loadstat.hpp>


using namespace MWRender;
using namespace Ogre;

namespace MWRender {



RenderingManager::RenderingManager (OEngine::Render::OgreRenderer& _rend, const boost::filesystem::path& resDir, OEngine::Physic::PhysicEngine* engine)
:mRendering(_rend), mObjects(mRendering), mDebugging(engine), mActors(mRendering)
{
    mRendering.createScene("PlayerCam", 55, 5);
    mSkyManager = MWRender::SkyManager::create(mRendering.getWindow(), mRendering.getCamera(), resDir);

    // Set default mipmap level (NB some APIs ignore this)
    TextureManager::getSingleton().setDefaultNumMipmaps(5);

    // Load resources
    ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

    // Turn the entire scene (represented by the 'root' node) -90
    // degrees around the x axis. This makes Z go upwards, and Y go into
    // the screen (when x is to the right.) This is the orientation that
    // Morrowind uses, and it automagically makes everything work as it
    // should.
    SceneNode *rt = mRendering.getScene()->getRootSceneNode();
    mMwRoot = rt->createChildSceneNode();
    mMwRoot->pitch(Degree(-90));
    mObjects.setMwRoot(mMwRoot);

    //used to obtain ingame information of ogre objects (which are faced or selected)
    mRaySceneQuery = mRendering.getScene()->createRayQuery(Ray());

    Ogre::SceneNode *playerNode = mMwRoot->createChildSceneNode ("player");
    playerNode->pitch(Degree(90));
    Ogre::SceneNode *cameraYawNode = playerNode->createChildSceneNode();
    Ogre::SceneNode *cameraPitchNode = cameraYawNode->createChildSceneNode();
    cameraPitchNode->attachObject(mRendering.getCamera());

    mPlayer = new MWRender::Player (mRendering.getCamera(), playerNode);
}

RenderingManager::~RenderingManager ()
{
    delete mPlayer;
    delete mSkyManager;
}


MWRender::Objects& RenderingManager::getObjects(){
    return mObjects;
}
MWRender::Actors& RenderingManager::getActors(){
    return mActors;
}

MWRender::Player& RenderingManager::getPlayer(){
    return (*mPlayer);
}

void RenderingManager::removeCell (MWWorld::Ptr::CellStore *store){
    mObjects.removeCell(store);
}

void RenderingManager::cellAdded (MWWorld::Ptr::CellStore *store)
{
    mObjects.buildStaticGeometry (*store);
}

void RenderingManager::addObject (const MWWorld::Ptr& ptr){
    const MWWorld::Class& class_ =
            MWWorld::Class::get (ptr);
    class_.insertObjectRendering(ptr, *this);

}
void RenderingManager::removeObject (const MWWorld::Ptr& ptr)
{
    if (!mObjects.deleteObject (ptr))
    {
        /// \todo delete non-object MW-references
    }
}

void RenderingManager::moveObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& position)
{
    /// \todo move this to the rendering-subsystems
    mRendering.getScene()->getSceneNode (ptr.getRefData().getHandle())->
            setPosition (position);
}

void RenderingManager::scaleObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& scale){

}
void RenderingManager::rotateObject (const MWWorld::Ptr& ptr, const::Ogre::Quaternion& orientation){

}
void RenderingManager::moveObjectToCell (const MWWorld::Ptr& ptr, const Ogre::Vector3& position, MWWorld::Ptr::CellStore *store){

}

void RenderingManager::update (float duration){


}

void RenderingManager::skyEnable ()
{
    mSkyManager->enable();
}

void RenderingManager::skyDisable ()
{
    mSkyManager->disable();
}

void RenderingManager::skySetHour (double hour)
{
    mSkyManager->setHour(hour);
}


void RenderingManager::skySetDate (int day, int month)
{
    mSkyManager->setDate(day, month);
}

int RenderingManager::skyGetMasserPhase() const
{
    return mSkyManager->getMasserPhase();
}

int RenderingManager::skyGetSecundaPhase() const
{
    return mSkyManager->getSecundaPhase();
}

void RenderingManager::skySetMoonColour (bool red)
{
    mSkyManager->setMoonColour(red);
}
bool RenderingManager::toggleRenderMode(int mode){
    return mDebugging.toggleRenderMode(mode);
}

void RenderingManager::configureFog(ESMS::CellStore<MWWorld::RefData> &mCell)
{
  Ogre::ColourValue color;
  color.setAsABGR (mCell.cell->ambi.fog);

  float high = 4500 + 9000 * (1-mCell.cell->ambi.fogDensity);
  float low = 200;

  mRendering.getScene()->setFog (FOG_LINEAR, color, 0, low, high);
  mRendering.getCamera()->setFarClipDistance (high + 10);
  mRendering.getViewport()->setBackgroundColour (color);
}

void RenderingManager::setAmbientMode()
{
  switch (mAmbientMode)
  {
    case 0:

      mRendering.getScene()->setAmbientLight(mAmbientColor);
      break;

    case 1:

      mRendering.getScene()->setAmbientLight(0.7f*mAmbientColor + 0.3f*ColourValue(1,1,1));
      break;

    case 2:

      mRendering.getScene()->setAmbientLight(ColourValue(1,1,1));
      break;
  }
}

void RenderingManager::configureAmbient(ESMS::CellStore<MWWorld::RefData> &mCell)
{
  mAmbientColor.setAsABGR (mCell.cell->ambi.ambient);
  setAmbientMode();

  // Create a "sun" that shines light downwards. It doesn't look
  // completely right, but leave it for now.
  Ogre::Light *light = mRendering.getScene()->createLight();
  Ogre::ColourValue colour;
  colour.setAsABGR (mCell.cell->ambi.sunlight);
  light->setDiffuseColour (colour);
  light->setType(Ogre::Light::LT_DIRECTIONAL);
  light->setDirection(0,-1,0);
}
// Switch through lighting modes.

void RenderingManager::toggleLight()
{
  if (mAmbientMode==2)
    mAmbientMode = 0;
  else
    ++mAmbientMode;

  switch (mAmbientMode)
  {
    case 0: std::cout << "Setting lights to normal\n"; break;
    case 1: std::cout << "Turning the lights up\n"; break;
    case 2: std::cout << "Turning the lights to full\n"; break;
  }

  setAmbientMode();
}

void RenderingManager::playAnimationGroup (const MWWorld::Ptr& ptr, const std::string& groupName,
     int mode, int number)
{
std::cout<<"play animation " << groupName << ", " << mode << ", " << number << std::endl;
}

void RenderingManager::skipAnimation (const MWWorld::Ptr& ptr)
{
std::cout<<"skip animation"<<std::endl;
}

}
