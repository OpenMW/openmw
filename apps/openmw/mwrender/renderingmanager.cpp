#include "renderingmanager.hpp"

#include <cassert>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>
#include <OgreViewport.h>
#include <OgreCamera.h>
#include <OgreTextureManager.h>
#include <OgreCompositorManager.h>
#include <OgreCompositorChain.h>
#include <OgreCompositionTargetPass.h>
#include <OgreCompositionPass.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreControllerManager.h>

#include <extern/shiny/Main/Factory.hpp>
#include <extern/shiny/Platforms/Ogre/OgrePlatform.hpp>

#include <openengine/bullet/physic.hpp>

#include <components/esm/loadstat.hpp>
#include <components/settings/settings.hpp>
#include "../mwworld/esmstore.hpp"
#include "../mwworld/class.hpp"

#include "../mwbase/world.hpp" // these includes can be removed once the static-hack is gone
#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp" // FIXME
#include "../mwbase/windowmanager.hpp" // FIXME

#include "../mwworld/ptr.hpp"
#include "../mwworld/player.hpp"

#include "shadows.hpp"
#include "localmap.hpp"
#include "water.hpp"
#include "compositors.hpp"
#include "npcanimation.hpp"
#include "externalrendering.hpp"
#include "globalmap.hpp"
#include "videoplayer.hpp"

using namespace MWRender;
using namespace Ogre;

namespace MWRender {

RenderingManager::RenderingManager (OEngine::Render::OgreRenderer& _rend, const boost::filesystem::path& resDir,
                                    const boost::filesystem::path& cacheDir, OEngine::Physic::PhysicEngine* engine,MWWorld::Fallback* fallback)
    : mRendering(_rend)
    , mFallback(fallback)
    , mObjects(mRendering,mFallback)
    , mActors(mRendering, this)
    , mAmbientMode(0)
    , mSunEnabled(0)
    , mPhysicsEngine(engine)
{
    // select best shader mode
    bool openGL = (Ogre::Root::getSingleton ().getRenderSystem ()->getName().find("OpenGL") != std::string::npos);

    // glsl is only supported in opengl mode and hlsl only in direct3d mode.
    if (Settings::Manager::getString("shader mode", "General") == ""
            || (openGL && Settings::Manager::getString("shader mode", "General") == "hlsl")
            || (!openGL && Settings::Manager::getString("shader mode", "General") == "glsl"))
    {
        Settings::Manager::setString("shader mode", "General", openGL ? "glsl" : "hlsl");
    }

    mRendering.createScene("PlayerCam", Settings::Manager::getFloat("field of view", "General"), 5);
    mRendering.setWindowEventListener(this);

    mRendering.getWindow()->addListener(this);

    mCompositors = new Compositors(mRendering.getViewport());

    mWater = 0;

    // material system
    sh::OgrePlatform* platform = new sh::OgrePlatform("General", (resDir / "materials").string());
    if (!boost::filesystem::exists (cacheDir))
        boost::filesystem::create_directories (cacheDir);
    platform->setCacheFolder (cacheDir.string());
    mFactory = new sh::Factory(platform);

    sh::Language lang;
    std::string l = Settings::Manager::getString("shader mode", "General");
    if (l == "glsl")
        lang = sh::Language_GLSL;
    else if (l == "hlsl")
        lang = sh::Language_HLSL;
    else
        lang = sh::Language_CG;
    mFactory->setCurrentLanguage (lang);
    mFactory->setWriteSourceCache (true);
    mFactory->setReadSourceCache (true);
    mFactory->setReadMicrocodeCache (true);
    mFactory->setWriteMicrocodeCache (true);

    mFactory->loadAllFiles();

    // Set default mipmap level (NB some APIs ignore this)
    // Mipmap generation is currently disabled because it causes issues on Intel/AMD
    //TextureManager::getSingleton().setDefaultNumMipmaps(Settings::Manager::getInt("num mipmaps", "General"));
    TextureManager::getSingleton().setDefaultNumMipmaps(0);

    // Set default texture filtering options
    TextureFilterOptions tfo;
    std::string filter = Settings::Manager::getString("texture filtering", "General");
    if (filter == "anisotropic") tfo = TFO_ANISOTROPIC;
    else if (filter == "trilinear") tfo = TFO_TRILINEAR;
    else if (filter == "bilinear") tfo = TFO_BILINEAR;
    else /*if (filter == "none")*/ tfo = TFO_NONE;

    MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);
    MaterialManager::getSingleton().setDefaultAnisotropy( (filter == "anisotropic") ? Settings::Manager::getInt("anisotropy", "General") : 1 );

    //ResourceGroupManager::getSingleton ().declareResource ("GlobalMap.png", "Texture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

    // causes light flicker in opengl when moving..
    //mRendering.getScene()->setCameraRelativeRendering(true);

    // disable unsupported effects
    if (!Settings::Manager::getBool("shaders", "Objects"))
        Settings::Manager::setBool("enabled", "Shadows", false);

    sh::Factory::getInstance ().setShadersEnabled (Settings::Manager::getBool("shaders", "Objects"));

    sh::Factory::getInstance ().setGlobalSetting ("fog", "true");
    sh::Factory::getInstance ().setGlobalSetting ("num_lights", Settings::Manager::getString ("num lights", "Objects"));
    sh::Factory::getInstance ().setGlobalSetting ("terrain_num_lights", Settings::Manager::getString ("num lights", "Terrain"));
    sh::Factory::getInstance ().setGlobalSetting ("simple_water", Settings::Manager::getBool("shader", "Water") ? "false" : "true");
    sh::Factory::getInstance ().setGlobalSetting ("render_refraction", "false");

    sh::Factory::getInstance ().setSharedParameter ("waterEnabled", sh::makeProperty<sh::FloatValue> (new sh::FloatValue(0.0)));
    sh::Factory::getInstance ().setSharedParameter ("waterLevel", sh::makeProperty<sh::FloatValue>(new sh::FloatValue(0)));
    sh::Factory::getInstance ().setSharedParameter ("waterTimer", sh::makeProperty<sh::FloatValue>(new sh::FloatValue(0)));
    sh::Factory::getInstance ().setSharedParameter ("windDir_windSpeed", sh::makeProperty<sh::Vector3>(new sh::Vector3(0.5, -0.8, 0.2)));
    sh::Factory::getInstance ().setSharedParameter ("waterSunFade_sunHeight", sh::makeProperty<sh::Vector2>(new sh::Vector2(1, 0.6)));
    sh::Factory::getInstance ().setGlobalSetting ("refraction", Settings::Manager::getBool("refraction", "Water") ? "true" : "false");
    sh::Factory::getInstance ().setGlobalSetting ("viewproj_fix", "false");
    sh::Factory::getInstance ().setSharedParameter ("vpRow2Fix", sh::makeProperty<sh::Vector4> (new sh::Vector4(0,0,0,0)));

    applyCompositors();

    SceneNode *rt = mRendering.getScene()->getRootSceneNode();
    mRootNode = rt;

    mObjects.setRootNode(mRootNode);
    mActors.setRootNode(mRootNode);

    Ogre::SceneNode *playerNode = mRootNode->createChildSceneNode ("player");
    mPlayer = new MWRender::Player (mRendering.getCamera(), playerNode);

    mShadows = new Shadows(&mRendering);

    mTerrainManager = new TerrainManager(mRendering.getScene(), this);

    mSkyManager = new SkyManager(mRootNode, mRendering.getCamera());

    mOcclusionQuery = new OcclusionQuery(&mRendering, mSkyManager->getSunNode());

    mVideoPlayer = new VideoPlayer(mRendering.getScene ());
    mVideoPlayer->setResolution (Settings::Manager::getInt ("resolution x", "Video"), Settings::Manager::getInt ("resolution y", "Video"));

    mSun = 0;

    mDebugging = new Debugging(mRootNode, engine);
    mLocalMap = new MWRender::LocalMap(&mRendering, this);

    mWater = new MWRender::Water(mRendering.getCamera(), this);

    setMenuTransparency(Settings::Manager::getFloat("menu transparency", "GUI"));
}

RenderingManager::~RenderingManager ()
{
    mRendering.getWindow()->removeListener(this);
    mRendering.removeWindowEventListener(this);

    delete mPlayer;
    delete mSkyManager;
    delete mDebugging;
    delete mShadows;
    delete mTerrainManager;
    delete mLocalMap;
    delete mOcclusionQuery;
    delete mCompositors;
    delete mWater;
    delete mVideoPlayer;
    delete mFactory;
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

OEngine::Render::Fader* RenderingManager::getFader()
{
    return mRendering.getFader();
}

void RenderingManager::removeCell (MWWorld::Ptr::CellStore *store)
{
    mObjects.removeCell(store);
    mActors.removeCell(store);
    mDebugging->cellRemoved(store);
    if (store->mCell->isExterior())
      mTerrainManager->cellRemoved(store);
}

void RenderingManager::removeWater ()
{
    mWater->setActive(false);
}

void RenderingManager::toggleWater()
{
    mWater->toggle();
}

void RenderingManager::cellAdded (MWWorld::Ptr::CellStore *store)
{
    mObjects.buildStaticGeometry (*store);
    mDebugging->cellAdded(store);
    if (store->mCell->isExterior())
      mTerrainManager->cellAdded(store);
    waterAdded(store);
}

void RenderingManager::addObject (const MWWorld::Ptr& ptr){
    const MWWorld::Class& class_ =
            MWWorld::Class::get (ptr);
    class_.insertObjectRendering(ptr, *this);
}

void RenderingManager::removeObject (const MWWorld::Ptr& ptr)
{
    if (!mObjects.deleteObject (ptr))
        mActors.deleteObject (ptr);
}

void RenderingManager::moveObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& position)
{
    /// \todo move this to the rendering-subsystems
    ptr.getRefData().getBaseNode()->setPosition(position);
}

void RenderingManager::scaleObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& scale)
{
    ptr.getRefData().getBaseNode()->setScale(scale);
}

bool RenderingManager::rotateObject( const MWWorld::Ptr &ptr, Ogre::Vector3 &rot, bool adjust)
{
    bool isActive = ptr.getRefData().getBaseNode() != 0;
    bool isPlayer = isActive && ptr.getRefData().getHandle() == "player";
    bool force = true;
    
    if (isPlayer)
        force = mPlayer->rotate(rot, adjust);
    
    MWWorld::Class::get(ptr).adjustRotation(ptr, rot.x, rot.y, rot.z);

    if (!isPlayer && isActive)
    {
        Ogre::Quaternion xr(Ogre::Radian(-rot.x), Ogre::Vector3::UNIT_X);
        Ogre::Quaternion yr(Ogre::Radian(-rot.y), Ogre::Vector3::UNIT_Y);
        Ogre::Quaternion zr(Ogre::Radian(-rot.z), Ogre::Vector3::UNIT_Z);

        Ogre::Quaternion xref(Ogre::Radian(-ptr.getRefData().getPosition().rot[0]), Ogre::Vector3::UNIT_X);
        Ogre::Quaternion yref(Ogre::Radian(-ptr.getRefData().getPosition().rot[1]), Ogre::Vector3::UNIT_Y);
        Ogre::Quaternion zref(Ogre::Radian(-ptr.getRefData().getPosition().rot[2]), Ogre::Vector3::UNIT_Z);

        Ogre::Quaternion newo = adjust ? (xr * yr * zr) * (xref*yref*zref) : xr * yr * zr;

        Ogre::Matrix3 mat;
        newo.ToRotationMatrix(mat);
        Ogre::Radian ax,ay,az;
        mat.ToEulerAnglesXYZ(ax,ay,az);
        rot.x = -ax.valueRadians();
        rot.y = -ay.valueRadians();
        rot.z = -az.valueRadians();

        ptr.getRefData().getBaseNode()->setOrientation(newo);
    }
    else if(isPlayer)
    {
        rot.x = -mPlayer->getPitch();
        rot.z = mPlayer->getYaw();
    }
    return force;
}

void
RenderingManager::updateObjectCell(const MWWorld::Ptr &old, const MWWorld::Ptr &cur)
{
    Ogre::SceneNode *child =
        mRendering.getScene()->getSceneNode(old.getRefData().getHandle());

    Ogre::SceneNode *parent = child->getParentSceneNode();
    parent->removeChild(child);

    if (MWWorld::Class::get(old).isActor()) {
        mActors.updateObjectCell(old, cur);
    } else {
        mObjects.updateObjectCell(old, cur);
    }
}

void RenderingManager::update (float duration, bool paused)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();

    // player position
    MWWorld::RefData &data =
        MWBase::Environment::get()
            .getWorld()
            ->getPlayer()
            .getPlayer()
            .getRefData();
    float *_playerPos = data.getPosition().pos;
    Ogre::Vector3 playerPos(_playerPos[0], _playerPos[1], _playerPos[2]);

    Ogre::Vector3 orig, dest;
    mPlayer->setCameraDistance();
    if (!mPlayer->getPosition(orig, dest)) {
        orig.z += mPlayer->getHeight() * mRootNode->getScale().z;

        btVector3 btOrig(orig.x, orig.y, orig.z);
        btVector3 btDest(dest.x, dest.y, dest.z);
        std::pair<std::string, float> test =
            mPhysicsEngine->rayTest(btOrig, btDest);
        if (!test.first.empty()) {
            mPlayer->setCameraDistance(test.second * orig.distance(dest), false, false);
        }
    }

    mOcclusionQuery->update(duration);

    mVideoPlayer->update ();

    mRendering.update(duration);

    Ogre::ControllerManager::getSingleton().setTimeFactor(paused ? 0.f : 1.f);

    Ogre::Vector3 cam = mRendering.getCamera()->getRealPosition();

    applyFog(world->isUnderwater (world->getPlayer().getPlayer().getCell(), cam));

    if(paused)
    {
        return;
    }

    mPlayer->update(duration);

    mActors.update (duration);
    mObjects.update (duration);


    mSkyManager->update(duration);

    mSkyManager->setGlare(mOcclusionQuery->getSunVisibility());

    Ogre::SceneNode *node = data.getBaseNode();
    //Ogre::Quaternion orient =
        //node->convertLocalToWorldOrientation(node->_getDerivedOrientation());
    Ogre::Quaternion orient =
node->_getDerivedOrientation();

    mLocalMap->updatePlayer(playerPos, orient);

    mWater->updateUnderwater(
        world->isUnderwater(
            world->getPlayer().getPlayer().getCell(),
            cam)
    );

    mWater->update(duration, playerPos);
}

void RenderingManager::preRenderTargetUpdate(const RenderTargetEvent &evt)
{
    mOcclusionQuery->setActive(true);
}

void RenderingManager::postRenderTargetUpdate(const RenderTargetEvent &evt)
{
    // deactivate queries to make sure we aren't getting false results from several misc render targets
    // (will be reactivated at the bottom of this method)
    mOcclusionQuery->setActive(false);
}

void RenderingManager::waterAdded (MWWorld::Ptr::CellStore *store)
{
    const MWWorld::Store<ESM::Land> &lands =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>();

    if(store->mCell->mData.mFlags & ESM::Cell::HasWater
        || ((store->mCell->isExterior())
            && !lands.search(store->mCell->getGridX(),store->mCell->getGridY()) )) // always use water, if the cell does not have land.
    {
        mWater->changeCell(store->mCell);
        mWater->setActive(true);
    }
    else
        removeWater();
}

void RenderingManager::setWaterHeight(const float height)
{
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
    if (mode == MWBase::World::Render_CollisionDebug || mode == MWBase::World::Render_Pathgrid)
        return mDebugging->toggleRenderMode(mode);
    else if (mode == MWBase::World::Render_Wireframe)
    {
        if (mRendering.getCamera()->getPolygonMode() == PM_SOLID)
        {
            mCompositors->setEnabled(false);

            mRendering.getCamera()->setPolygonMode(PM_WIREFRAME);
            return true;
        }
        else
        {
            mCompositors->setEnabled(true);

            mRendering.getCamera()->setPolygonMode(PM_SOLID);
            return false;
        }
    }
    else if (mode == MWBase::World::Render_BoundingBoxes)
    {
        bool show = !mRendering.getScene()->getShowBoundingBoxes();
        mRendering.getScene()->showBoundingBoxes(show);
        return show;
    }
    else //if (mode == MWBase::World::Render_Compositors)
    {
        return mCompositors->toggle();
    }
}

void RenderingManager::configureFog(MWWorld::Ptr::CellStore &mCell)
{
    Ogre::ColourValue color;
    color.setAsABGR (mCell.mCell->mAmbi.mFog);

    configureFog(mCell.mCell->mAmbi.mFogDensity, color);
}

void RenderingManager::configureFog(const float density, const Ogre::ColourValue& colour)
{
    mFogColour = colour;
    float max = Settings::Manager::getFloat("max viewing distance", "Viewing distance");

    mFogStart = max / (density) * Settings::Manager::getFloat("fog start factor", "Viewing distance");
    mFogEnd = max / (density) * Settings::Manager::getFloat("fog end factor", "Viewing distance");

    mRendering.getCamera()->setFarClipDistance ( Settings::Manager::getFloat("max viewing distance", "Viewing distance") / density );
}

void RenderingManager::applyFog (bool underwater)
{
    if (!underwater)
    {
        mRendering.getScene()->setFog (FOG_LINEAR, mFogColour, 0, mFogStart, mFogEnd);
        mRendering.getViewport()->setBackgroundColour (mFogColour);
        mWater->setViewportBackground (mFogColour);
    }
    else
    {
        mRendering.getScene()->setFog (FOG_LINEAR, Ogre::ColourValue(0.18039, 0.23137, 0.25490), 0, 0, 1000);
        mRendering.getViewport()->setBackgroundColour (Ogre::ColourValue(0.18039, 0.23137, 0.25490));
        mWater->setViewportBackground (Ogre::ColourValue(0.18039, 0.23137, 0.25490));
    }
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

void RenderingManager::configureAmbient(MWWorld::Ptr::CellStore &mCell)
{
    mAmbientColor.setAsABGR (mCell.mCell->mAmbi.mAmbient);
    setAmbientMode();

    // Create a "sun" that shines light downwards. It doesn't look
    // completely right, but leave it for now.
    if(!mSun)
    {
        mSun = mRendering.getScene()->createLight();
    }
    Ogre::ColourValue colour;
    colour.setAsABGR (mCell.mCell->mAmbi.mSunlight);
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

void RenderingManager::setSunColour(const Ogre::ColourValue& colour)
{
    if (!mSunEnabled) return;
    mSun->setDiffuseColour(colour);
    mSun->setSpecularColour(colour);
    mTerrainManager->setDiffuse(colour);
}

void RenderingManager::setAmbientColour(const Ogre::ColourValue& colour)
{
    mRendering.getScene()->setAmbientLight(colour);
    mTerrainManager->setAmbient(colour);
}

void RenderingManager::sunEnable(bool real)
{
    if (real && mSun) mSun->setVisible(true);
    else
    {
        // Don't disable the light, as the shaders assume the first light to be directional.
        mSunEnabled = true;
    }
}

void RenderingManager::sunDisable(bool real)
{
    if (real && mSun) mSun->setVisible(false);
    else
    {
        // Don't disable the light, as the shaders assume the first light to be directional.
        mSunEnabled = false;
        if (mSun)
        {
            mSun->setDiffuseColour(ColourValue(0,0,0));
            mSun->setSpecularColour(ColourValue(0,0,0));
        }
    }
}

void RenderingManager::setSunDirection(const Ogre::Vector3& direction)
{
    // direction * -1 (because 'direction' is camera to sun vector and not sun to camera),
    if (mSun) mSun->setDirection(Vector3(-direction.x, -direction.y, -direction.z));

    mSkyManager->setSunDirection(direction);
}

void RenderingManager::setGlare(bool glare)
{
    mSkyManager->setGlare(glare);
}

void RenderingManager::requestMap(MWWorld::Ptr::CellStore* cell)
{
    if (cell->mCell->isExterior())
        mLocalMap->requestMap(cell);
    else
        mLocalMap->requestMap(cell, mObjects.getDimensions(cell));
}

void RenderingManager::preCellChange(MWWorld::Ptr::CellStore* cell)
{
    mLocalMap->saveFogOfWar(cell);
}

void RenderingManager::disableLights(bool sun)
{
    mObjects.disableLights();
    sunDisable(sun);
}

void RenderingManager::enableLights(bool sun)
{
    mObjects.enableLights();
    sunEnable(sun);
}

Shadows* RenderingManager::getShadows()
{
    return mShadows;
}

void RenderingManager::switchToInterior()
{
    // causes light flicker in opengl when moving..
    //mRendering.getScene()->setCameraRelativeRendering(false);
}

void RenderingManager::switchToExterior()
{
    // causes light flicker in opengl when moving..
    //mRendering.getScene()->setCameraRelativeRendering(true);
}

Ogre::Vector4 RenderingManager::boundingBoxToScreen(Ogre::AxisAlignedBox bounds)
{
    Ogre::Matrix4 mat = mRendering.getCamera()->getViewMatrix();

    const Ogre::Vector3* corners = bounds.getAllCorners();

    float min_x = 1.0f, max_x = 0.0f, min_y = 1.0f, max_y = 0.0f;

    // expand the screen-space bounding-box so that it completely encloses
    // the object's AABB
    for (int i=0; i<8; i++)
    {
        Ogre::Vector3 corner = corners[i];

        // multiply the AABB corner vertex by the view matrix to
        // get a camera-space vertex
        corner = mat * corner;

        // make 2D relative/normalized coords from the view-space vertex
        // by dividing out the Z (depth) factor -- this is an approximation
        float x = corner.x / corner.z + 0.5;
        float y = corner.y / corner.z + 0.5;

        if (x < min_x)
        min_x = x;

        if (x > max_x)
        max_x = x;

        if (y < min_y)
        min_y = y;

        if (y > max_y)
        max_y = y;
    }

    return Vector4(min_x, min_y, max_x, max_y);
}

Compositors* RenderingManager::getCompositors()
{
    return mCompositors;
}

void RenderingManager::processChangedSettings(const Settings::CategorySettingVector& settings)
{
    bool changeRes = false;
    bool rebuild = false; // rebuild static geometry (necessary after any material changes)
    for (Settings::CategorySettingVector::const_iterator it=settings.begin();
            it != settings.end(); ++it)
    {
        if (it->second == "menu transparency" && it->first == "GUI")
        {
            setMenuTransparency(Settings::Manager::getFloat("menu transparency", "GUI"));
        }
        else if (it->second == "max viewing distance" && it->first == "Viewing distance")
        {
            if (!MWBase::Environment::get().getWorld()->isCellExterior() && !MWBase::Environment::get().getWorld()->isCellQuasiExterior())
                configureFog(*MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getCell());
        }
        else if (it->first == "Video" && (
                it->second == "resolution x"
                || it->second == "resolution y"
                || it->second == "fullscreen"))
            changeRes = true;
        else if (it->second == "field of view" && it->first == "General")
            mRendering.setFov(Settings::Manager::getFloat("field of view", "General"));
        else if ((it->second == "texture filtering" && it->first == "General")
            || (it->second == "anisotropy" && it->first == "General"))
        {
            TextureFilterOptions tfo;
            std::string filter = Settings::Manager::getString("texture filtering", "General");
            if (filter == "anisotropic") tfo = TFO_ANISOTROPIC;
            else if (filter == "trilinear") tfo = TFO_TRILINEAR;
            else if (filter == "bilinear") tfo = TFO_BILINEAR;
            else /*if (filter == "none")*/ tfo = TFO_NONE;

            MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);
            MaterialManager::getSingleton().setDefaultAnisotropy( (filter == "anisotropic") ? Settings::Manager::getInt("anisotropy", "General") : 1 );
        }
        else if (it->second == "shader" && it->first == "Water")
        {
            applyCompositors();
            sh::Factory::getInstance ().setGlobalSetting ("simple_water", Settings::Manager::getBool("shader", "Water") ? "false" : "true");
            rebuild = true;
            mRendering.getViewport ()->setClearEveryFrame (true);
        }
        else if (it->second == "refraction" && it->first == "Water")
        {
            sh::Factory::getInstance ().setGlobalSetting ("refraction", Settings::Manager::getBool("refraction", "Water") ? "true" : "false");
            rebuild = true;
        }
        else if (it->second == "shaders" && it->first == "Objects")
        {
            sh::Factory::getInstance ().setShadersEnabled (Settings::Manager::getBool("shaders", "Objects"));
            rebuild = true;
        }
        else if (it->second == "shader mode" && it->first == "General")
        {
            sh::Language lang;
            std::string l = Settings::Manager::getString("shader mode", "General");
            if (l == "glsl")
                lang = sh::Language_GLSL;
            else if (l == "hlsl")
                lang = sh::Language_HLSL;
            else
                lang = sh::Language_CG;
            sh::Factory::getInstance ().setCurrentLanguage (lang);
            rebuild = true;
        }
        else if (it->first == "Shadows")
        {
            mShadows->recreate ();

            rebuild = true;
        }
    }

    if (changeRes)
    {
        unsigned int x = Settings::Manager::getInt("resolution x", "Video");
        unsigned int y = Settings::Manager::getInt("resolution y", "Video");

        if (x != mRendering.getWindow()->getWidth() || y != mRendering.getWindow()->getHeight())
        {
            mRendering.getWindow()->resize(x, y);
        }
        mRendering.getWindow()->setFullscreen(Settings::Manager::getBool("fullscreen", "Video"), x, y);
    }

    mWater->processChangedSettings(settings);

    if (rebuild)
        mObjects.rebuildStaticGeometry();
}

void RenderingManager::setMenuTransparency(float val)
{
    Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().getByName("transparent.png");
    std::vector<Ogre::uint32> buffer;
    buffer.resize(1);
    buffer[0] = (int(255*val) << 24);
    memcpy(tex->getBuffer()->lock(Ogre::HardwareBuffer::HBL_DISCARD), &buffer[0], 1*4);
    tex->getBuffer()->unlock();
}

void RenderingManager::windowResized(Ogre::RenderWindow* rw)
{
    Settings::Manager::setInt("resolution x", "Video", rw->getWidth());
    Settings::Manager::setInt("resolution y", "Video", rw->getHeight());


    mRendering.adjustViewport();
    mCompositors->recreate();

    mVideoPlayer->setResolution (rw->getWidth(), rw->getHeight());

    const Settings::CategorySettingVector& changed = Settings::Manager::apply();
    MWBase::Environment::get().getInputManager()->processChangedSettings(changed);
    MWBase::Environment::get().getWindowManager()->processChangedSettings(changed);
}

void RenderingManager::windowClosed(Ogre::RenderWindow* rw)
{
    Ogre::Root::getSingleton ().queueEndRendering ();
}

void RenderingManager::applyCompositors()
{
}

void RenderingManager::getTriangleBatchCount(unsigned int &triangles, unsigned int &batches)
{
    if (mCompositors->anyCompositorEnabled())
    {
        mCompositors->countTrianglesBatches(triangles, batches);
    }
    else
    {
        triangles = mRendering.getWindow()->getTriangleCount();
        batches = mRendering.getWindow()->getBatchCount();
    }
}

void RenderingManager::attachCameraTo(const MWWorld::Ptr &ptr)
{
    mPlayer->attachTo(ptr);
}

void RenderingManager::renderPlayer(const MWWorld::Ptr &ptr)
{
    MWRender::NpcAnimation *anim =
        new MWRender::NpcAnimation(
            ptr, ptr.getRefData ().getBaseNode (),
            MWWorld::Class::get(ptr).getInventoryStore(ptr), RV_Actors
        );
    mPlayer->setAnimation(anim);
    mWater->removeEmitter (ptr);
    mWater->addEmitter (ptr);
    // apply race height
    MWBase::Environment::get().getWorld()->scaleObject(ptr, 1.f);
}

void RenderingManager::getPlayerData(Ogre::Vector3 &eyepos, float &pitch, float &yaw)
{
    eyepos = mPlayer->getPosition();
    eyepos.z += mPlayer->getHeight();
    mPlayer->getSightAngles(pitch, yaw);
}

void RenderingManager::getInteriorMapPosition (Ogre::Vector2 position, float& nX, float& nY, int &x, int& y)
{
    return mLocalMap->getInteriorMapPosition (position, nX, nY, x, y);
}

bool RenderingManager::isPositionExplored (float nX, float nY, int x, int y, bool interior)
{
    return mLocalMap->isPositionExplored(nX, nY, x, y, interior);
}

void RenderingManager::setupExternalRendering (MWRender::ExternalRendering& rendering)
{
    rendering.setup (mRendering.getScene());
}

Animation* RenderingManager::getAnimation(const MWWorld::Ptr &ptr)
{
    Animation *anim = mActors.getAnimation(ptr);
    if(!anim && ptr.getRefData().getHandle() == "player")
        anim = mPlayer->getAnimation();
    return anim;
}


void RenderingManager::playVideo(const std::string& name, bool allowSkipping)
{
    mVideoPlayer->playVideo ("video/" + name, allowSkipping);
}

void RenderingManager::stopVideo()
{
    mVideoPlayer->stopVideo ();
}

void RenderingManager::addWaterRippleEmitter (const MWWorld::Ptr& ptr, float scale, float force)
{
    mWater->addEmitter (ptr, scale, force);
}

void RenderingManager::removeWaterRippleEmitter (const MWWorld::Ptr& ptr)
{
    mWater->removeEmitter (ptr);
}

void RenderingManager::updateWaterRippleEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr)
{
    mWater->updateEmitterPtr(old, ptr);
}

void RenderingManager::frameStarted(float dt)
{
    mWater->frameStarted(dt);
}

} // namespace
