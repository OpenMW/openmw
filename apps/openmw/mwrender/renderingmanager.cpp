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
:rend(_rend), objects(rend), mDebugging(engine)
{
    rend.createScene("PlayerCam", 55, 5);
    mSkyManager = MWRender::SkyManager::create(rend.getWindow(), rend.getCamera(), resDir);

    // Set default mipmap level (NB some APIs ignore this)
    TextureManager::getSingleton().setDefaultNumMipmaps(5);

    // Load resources
    ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

    // Turn the entire scene (represented by the 'root' node) -90
    // degrees around the x axis. This makes Z go upwards, and Y go into
    // the screen (when x is to the right.) This is the orientation that
    // Morrowind uses, and it automagically makes everything work as it
    // should.
    SceneNode *rt = rend.getScene()->getRootSceneNode();
    mwRoot = rt->createChildSceneNode();
    mwRoot->pitch(Degree(-90));
    objects.setMwRoot(mwRoot);

    //used to obtain ingame information of ogre objects (which are faced or selected)
    mRaySceneQuery = rend.getScene()->createRayQuery(Ray());

    Ogre::SceneNode *playerNode = mwRoot->createChildSceneNode ("player");
    playerNode->pitch(Degree(90));
    Ogre::SceneNode *cameraYawNode = playerNode->createChildSceneNode();
    Ogre::SceneNode *cameraPitchNode = cameraYawNode->createChildSceneNode();
    cameraPitchNode->attachObject(rend.getCamera());

    mPlayer = new MWRender::Player (rend.getCamera(), playerNode);

    mWater = 0;

	//std::cout << "Three";
}

RenderingManager::~RenderingManager ()
{
    delete mPlayer;
    delete mSkyManager;
}

MWRender::Npcs& RenderingManager::getNPCs(){
    return npcs;
}
MWRender::Objects& RenderingManager::getObjects(){
    return objects;
}
MWRender::Creatures& RenderingManager::getCreatures(){
    return creatures;
}
MWRender::Player& RenderingManager::getPlayer(){
    return (*mPlayer);
}

void RenderingManager::removeCell (MWWorld::Ptr::CellStore *store){
    objects.removeCell(store);
    
}
void RenderingManager::removeWater (){
    if(mWater){
        delete mWater;
        mWater = 0;
    }
}

void RenderingManager::cellAdded (MWWorld::Ptr::CellStore *store)
{
    objects.buildStaticGeometry (*store);
}

void RenderingManager::addObject (const MWWorld::Ptr& ptr){
    const MWWorld::Class& class_ =
            MWWorld::Class::get (ptr);
    class_.insertObjectRendering(ptr, *this);

}
void RenderingManager::removeObject (const MWWorld::Ptr& ptr)
{
    if (!objects.deleteObject (ptr))
    {
        /// \todo delete non-object MW-references
    }
}

void RenderingManager::moveObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& position)
{
    /// \todo move this to the rendering-subsystems
    rend.getScene()->getSceneNode (ptr.getRefData().getHandle())->
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
void RenderingManager::waterAdded (MWWorld::Ptr::CellStore *store){
    if(store->cell->data.flags & store->cell->HasWater){
        if(mWater == 0)
            mWater = new MWRender::Water(rend.getCamera(), store->cell);
        //else

    }
   
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

  rend.getScene()->setFog (FOG_LINEAR, color, 0, low, high);
  rend.getCamera()->setFarClipDistance (high + 10);
  rend.getViewport()->setBackgroundColour (color);
}

void RenderingManager::setAmbientMode()
{
  switch (mAmbientMode)
  {
    case 0:

      rend.getScene()->setAmbientLight(mAmbientColor);
      break;

    case 1:

      rend.getScene()->setAmbientLight(0.7f*mAmbientColor + 0.3f*ColourValue(1,1,1));
      break;

    case 2:

      rend.getScene()->setAmbientLight(ColourValue(1,1,1));
      break;
  }
}

void RenderingManager::configureAmbient(ESMS::CellStore<MWWorld::RefData> &mCell)
{
  mAmbientColor.setAsABGR (mCell.cell->ambi.ambient);
  setAmbientMode();

  // Create a "sun" that shines light downwards. It doesn't look
  // completely right, but leave it for now.
  Ogre::Light *light = rend.getScene()->createLight();
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
void RenderingManager::checkUnderwater(float y){
    if(mWater){
         mWater->checkUnderwater(y);
    }
}



}
