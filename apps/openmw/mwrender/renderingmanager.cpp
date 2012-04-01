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

RenderingManager::RenderingManager (OEngine::Render::OgreRenderer& _rend, const boost::filesystem::path& resDir, OEngine::Physic::PhysicEngine* engine, MWWorld::Environment& environment)
:mRendering(_rend), mObjects(mRendering), mActors(mRendering, environment), mAmbientMode(0), mDebugging(engine)
{
    mRendering.createScene("PlayerCam", 55, 5);
    mTerrainManager = new TerrainManager(mRendering.getScene(),
                                         environment);

    //The fog type must be set before any terrain objects are created as if the
    //fog type is set to FOG_NONE then the initially created terrain won't have any fog
    configureFog(1, ColourValue(1,1,1));

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
    mActors.setMwRoot(mMwRoot);

    Ogre::SceneNode *playerNode = mMwRoot->createChildSceneNode ("player");
    playerNode->pitch(Degree(90));
    Ogre::SceneNode *cameraYawNode = playerNode->createChildSceneNode();
    Ogre::SceneNode *cameraPitchNode = cameraYawNode->createChildSceneNode();
    cameraPitchNode->attachObject(mRendering.getCamera());
    
    //mSkyManager = 0;
    mSkyManager = new SkyManager(mMwRoot, mRendering.getCamera(), &environment);

    mOcclusionQuery = new OcclusionQuery(&mRendering, mSkyManager->getSunNode());

    mWater = 0;

    mPlayer = new MWRender::Player (mRendering.getCamera(), playerNode);
    mSun = 0;

    mLocalMap = new MWRender::LocalMap(&mRendering, &environment);
}

RenderingManager::~RenderingManager ()
{
    //TODO: destroy mSun?
    delete mPlayer;
    delete mSkyManager;
    delete mTerrainManager;
    delete mLocalMap;
    delete mOcclusionQuery;
}

MWRender::SkyManager* RenderingManager::getSkyManager()
{
    return mSkyManager;
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

OEngine::Render::Fader* RenderingManager::getFader()
{
    return mRendering.getFader();
}

void RenderingManager::removeCell (MWWorld::Ptr::CellStore *store)
{
    mObjects.removeCell(store);
    mActors.removeCell(store);
    if (store->cell->isExterior())
      mTerrainManager->cellRemoved(store);
}

void RenderingManager::removeWater ()
{
    if(mWater){
        delete mWater;
        mWater = 0;
    }
}

void RenderingManager::toggleWater()
{
    if (mWater)
        mWater->toggle();
}

void RenderingManager::cellAdded (MWWorld::Ptr::CellStore *store)
{
    mObjects.buildStaticGeometry (*store);
    if (store->cell->isExterior())
      mTerrainManager->cellAdded(store);
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
     if (!mActors.deleteObject (ptr))
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

    mActors.update (duration);

    mOcclusionQuery->update(duration);

    mSkyManager->update(duration);

    mSkyManager->setGlare(mOcclusionQuery->getSunVisibility());

    mRendering.update(duration);

    mLocalMap->updatePlayer( mRendering.getCamera()->getRealPosition(), mRendering.getCamera()->getRealOrientation() );

    checkUnderwater();
}
void RenderingManager::waterAdded (MWWorld::Ptr::CellStore *store){
    if(store->cell->data.flags & store->cell->HasWater){
        if(mWater == 0)
            mWater = new MWRender::Water(mRendering.getCamera(), store->cell);
        else
            mWater->changeCell(store->cell);
        //else

    }
    else
        removeWater();
   
}

void RenderingManager::setWaterHeight(const float height)
{
    if (mWater)
        mWater->setHeight(height);
}

void RenderingManager::skyEnable ()
{
    if(mSkyManager)
    mSkyManager->enable();

    mOcclusionQuery->setSunNode(mSkyManager->getSunNode());
}

void RenderingManager::skyDisable ()
{
    if(mSkyManager)
        mSkyManager->disable();
}

void RenderingManager::skySetHour (double hour)
{
    if(mSkyManager)
        mSkyManager->setHour(hour);
}


void RenderingManager::skySetDate (int day, int month)
{
    if(mSkyManager)
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

void RenderingManager::skySetMoonColour (bool red){
    if(mSkyManager)
        mSkyManager->setMoonColour(red);
}

bool RenderingManager::toggleRenderMode(int mode)
{
    if (mode == MWWorld::World::Render_CollisionDebug)
        return mDebugging.toggleRenderMode(mode);
    else // if (mode == MWWorld::World::Render_Wireframe)
    {
        if (mRendering.getCamera()->getPolygonMode() == PM_SOLID)
        {
            mRendering.getCamera()->setPolygonMode(PM_WIREFRAME);
            return true;
        }
        else
        {
            mRendering.getCamera()->setPolygonMode(PM_SOLID);
            return false;
        }
    }
}

void RenderingManager::configureFog(ESMS::CellStore<MWWorld::RefData> &mCell)
{
  Ogre::ColourValue color;
  color.setAsABGR (mCell.cell->ambi.fog);

  configureFog(mCell.cell->ambi.fogDensity, color);
}

void RenderingManager::configureFog(const float density, const Ogre::ColourValue& colour)
{  
  /// \todo make the viewing distance and fog start/end configurable

  // right now we load 3x3 cells, so the maximum viewing distance we 
  // can allow (to prevent objects suddenly popping up) equals:
  // 8192            * 0.69
  //   ^ cell size    ^ minimum density value used (clear weather)
  float low = 5652.48 / density / 2.f;
  float high = 5652.48 / density;

  mRendering.getScene()->setFog (FOG_LINEAR, colour, 0, low, high);
  
  mRendering.getCamera()->setFarClipDistance ( high );
  mRendering.getViewport()->setBackgroundColour (colour);
}


void RenderingManager::setAmbientMode()
{
  switch (mAmbientMode)
  {
    case 0:

      setAmbientColour(mAmbientColor);
      break;

    case 1:

      setAmbientColour(0.7f*mAmbientColor + 0.3f*ColourValue(1,1,1));
      break;

    case 2:

      setAmbientColour(ColourValue(1,1,1));
      break;
  }
}

void RenderingManager::configureAmbient(ESMS::CellStore<MWWorld::RefData> &mCell)
{
  mAmbientColor.setAsABGR (mCell.cell->ambi.ambient);
  setAmbientMode();

  // Create a "sun" that shines light downwards. It doesn't look
  // completely right, but leave it for now.
  if(!mSun)
  {
      mSun = mRendering.getScene()->createLight();
  }
  Ogre::ColourValue colour;
  colour.setAsABGR (mCell.cell->ambi.sunlight);
  mSun->setDiffuseColour (colour);
  mSun->setType(Ogre::Light::LT_DIRECTIONAL);
  mSun->setDirection(0,-1,0);
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
void RenderingManager::checkUnderwater(){
    if(mWater){
         mWater->checkUnderwater( mRendering.getCamera()->getRealPosition().y );
    }
}

void RenderingManager::playAnimationGroup (const MWWorld::Ptr& ptr, const std::string& groupName,
     int mode, int number)
{
    mActors.playAnimationGroup(ptr, groupName, mode, number);
}

void RenderingManager::skipAnimation (const MWWorld::Ptr& ptr)
{
    mActors.skipAnimation(ptr);
}

void RenderingManager::setSunColour(const Ogre::ColourValue& colour)
{
    mSun->setDiffuseColour(colour);
    mTerrainManager->setDiffuse(colour);
}

void RenderingManager::setAmbientColour(const Ogre::ColourValue& colour)
{
    mRendering.getScene()->setAmbientLight(colour);
    mTerrainManager->setAmbient(colour);
}

void RenderingManager::sunEnable()
{
    if (mSun) mSun->setVisible(true);
}

void RenderingManager::sunDisable()
{
    if (mSun) mSun->setVisible(false);
}

void RenderingManager::setSunDirection(const Ogre::Vector3& direction)
{
    // direction * -1 (because 'direction' is camera to sun vector and not sun to camera), 
    // then convert from MW to ogre coordinates (swap y,z and make y negative)
    if (mSun) mSun->setDirection(Vector3(-direction.x, -direction.z, direction.y));
    
    mSkyManager->setSunDirection(direction);
}

void RenderingManager::setGlare(bool glare)
{
    mSkyManager->setGlare(glare);
}

void RenderingManager::requestMap(MWWorld::Ptr::CellStore* cell)
{
    if (!(cell->cell->data.flags & ESM::Cell::Interior))
        mLocalMap->requestMap(cell);
    else
        mLocalMap->requestMap(cell, mObjects.getDimensions(cell));
}

void RenderingManager::preCellChange(MWWorld::Ptr::CellStore* cell)
{
    mLocalMap->saveFogOfWar(cell);
}

} // namespace
