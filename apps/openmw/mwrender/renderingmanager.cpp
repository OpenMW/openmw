#include "renderingmanager.hpp"

#include <cassert>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreViewport.h>
#include <OgreCamera.h>
#include <OgreTextureManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreControllerManager.h>
#include <OgreMeshManager.h>
#include <OgreRenderTexture.h>

#include <SDL_video.h>

#include <extern/shiny/Main/Factory.hpp>
#include <extern/shiny/Platforms/Ogre/OgrePlatform.hpp>

#include <openengine/bullet/physic.hpp>

#include <components/settings/settings.hpp>
#include <components/terrain/defaultworld.hpp>
#include <components/terrain/terraingrid.hpp>

#include "../mwworld/esmstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwbase/world.hpp" // these includes can be removed once the static-hack is gone
#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp" // FIXME
#include "../mwbase/windowmanager.hpp" // FIXME
#include "../mwbase/statemanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/ptr.hpp"

#include "shadows.hpp"
#include "localmap.hpp"
#include "water.hpp"
#include "npcanimation.hpp"
#include "globalmap.hpp"
#include "terrainstorage.hpp"
#include "effectmanager.hpp"

using namespace MWRender;
using namespace Ogre;

namespace MWRender {

RenderingManager::RenderingManager(OEngine::Render::OgreRenderer& _rend, const boost::filesystem::path& resDir,
                                   const boost::filesystem::path& cacheDir, OEngine::Physic::PhysicEngine* engine,
                                   MWWorld::Fallback* fallback)
    : mSunEnabled(0)
    , mFallback(fallback)
    , mTerrain(NULL)
    , mRendering(_rend)
    , mEffectManager(NULL)
    , mPlayerAnimation(NULL)
    , mAmbientMode(0)
    , mPhysicsEngine(engine)
    , mRenderWorld(true)
{
    mActors = new MWRender::Actors(mRendering, this);
    mObjects = new MWRender::Objects(mRendering);
    mEffectManager = new EffectManager(mRendering.getScene());
    // select best shader mode
    bool openGL = (Ogre::Root::getSingleton ().getRenderSystem ()->getName().find("OpenGL") != std::string::npos);
    bool glES = (Ogre::Root::getSingleton ().getRenderSystem ()->getName().find("OpenGL ES") != std::string::npos);

    // glsl is only supported in opengl mode and hlsl only in direct3d mode.
    std::string currentMode = Settings::Manager::getString("shader mode", "General");
    if (currentMode == ""
            || (openGL && currentMode == "hlsl")
            || (!openGL && currentMode == "glsl")
            || (glES && currentMode != "glsles"))
    {
        Settings::Manager::setString("shader mode", "General", openGL ? (glES ? "glsles" : "glsl") : "hlsl");
    }

    mRendering.adjustCamera(Settings::Manager::getFloat("field of view", "General"), 5);

    mRendering.getWindow()->addListener(this);
    mRendering.setWindowListener(this);

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
    else if (l == "glsles")
        lang = sh::Language_GLSLES;
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

    TextureManager::getSingleton().setDefaultNumMipmaps(Settings::Manager::getInt("num mipmaps", "General"));

    // Set default texture filtering options
    TextureFilterOptions tfo;
    std::string filter = Settings::Manager::getString("texture filtering", "General");

    if (filter == "anisotropic") tfo = TFO_ANISOTROPIC;
    else if (filter == "trilinear") tfo = TFO_TRILINEAR;
    else if (filter == "bilinear") tfo = TFO_BILINEAR;
    else /*if (filter == "none")*/ tfo = TFO_NONE;

    MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);
    MaterialManager::getSingleton().setDefaultAnisotropy( (filter == "anisotropic") ? Settings::Manager::getInt("anisotropy", "General") : 1 );

    Ogre::TextureManager::getSingleton().setMemoryBudget(126*1024*1024);
    Ogre::MeshManager::getSingleton().setMemoryBudget(64*1024*1024);

    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

    // disable unsupported effects
    if (!Settings::Manager::getBool("shaders", "Objects"))
        Settings::Manager::setBool("enabled", "Shadows", false);

    sh::Factory::getInstance ().setShadersEnabled (Settings::Manager::getBool("shaders", "Objects"));

    sh::Factory::getInstance ().setGlobalSetting ("fog", "true");
    sh::Factory::getInstance ().setGlobalSetting ("num_lights", Settings::Manager::getString ("num lights", "Objects"));
    sh::Factory::getInstance ().setGlobalSetting ("simple_water", Settings::Manager::getBool("shader", "Water") ? "false" : "true");
    sh::Factory::getInstance ().setGlobalSetting ("render_refraction", "false");

    sh::Factory::getInstance ().setSharedParameter ("waterEnabled", sh::makeProperty<sh::FloatValue> (new sh::FloatValue(0.0)));
    sh::Factory::getInstance ().setSharedParameter ("waterLevel", sh::makeProperty<sh::FloatValue>(new sh::FloatValue(0)));
    sh::Factory::getInstance ().setSharedParameter ("waterTimer", sh::makeProperty<sh::FloatValue>(new sh::FloatValue(0)));
    sh::Factory::getInstance ().setSharedParameter ("windDir_windSpeed", sh::makeProperty<sh::Vector3>(new sh::Vector3(0.5f, -0.8f, 0.2f)));
    sh::Factory::getInstance ().setSharedParameter ("waterSunFade_sunHeight", sh::makeProperty<sh::Vector2>(new sh::Vector2(1, 0.6f)));
    sh::Factory::getInstance ().setGlobalSetting ("refraction", Settings::Manager::getBool("refraction", "Water") ? "true" : "false");
    sh::Factory::getInstance ().setGlobalSetting ("viewproj_fix", "false");
    sh::Factory::getInstance ().setSharedParameter ("vpRow2Fix", sh::makeProperty<sh::Vector4> (new sh::Vector4(0,0,0,0)));

    mRootNode = mRendering.getScene()->getRootSceneNode();
    mRootNode->createChildSceneNode("player");

    mObjects->setRootNode(mRootNode);
    mActors->setRootNode(mRootNode);

    mCamera = new MWRender::Camera(mRendering.getCamera());

    mShadows = new Shadows(&mRendering);

    mSkyManager = new SkyManager(mRootNode, mRendering.getCamera());

    mOcclusionQuery = new OcclusionQuery(&mRendering, mSkyManager->getSunNode());

    mSun = 0;

    mDebugging = new Debugging(mRootNode, engine);
    mLocalMap = new MWRender::LocalMap(&mRendering, this);

    mWater = new MWRender::Water(mRendering.getCamera(), this, mFallback);

    setMenuTransparency(Settings::Manager::getFloat("menu transparency", "GUI"));
}

RenderingManager::~RenderingManager ()
{
    mRendering.getWindow()->removeListener(this);

    delete mPlayerAnimation;
    delete mCamera;
    delete mSkyManager;
    delete mDebugging;
    delete mShadows;
    delete mTerrain;
    delete mLocalMap;
    delete mOcclusionQuery;
    delete mWater;
    delete mActors;
    delete mObjects;
    delete mEffectManager;
    delete mFactory;
}

MWRender::SkyManager* RenderingManager::getSkyManager()
{
    return mSkyManager;
}

MWRender::Objects& RenderingManager::getObjects(){
    return *mObjects;
}
MWRender::Actors& RenderingManager::getActors(){
    return *mActors;
}

MWRender::Camera* RenderingManager::getCamera() const
{
    return mCamera;
}

void RenderingManager::removeCell (MWWorld::CellStore *store)
{
    if (store->isExterior())
        mTerrain->unloadCell(store->getCell()->getGridX(), store->getCell()->getGridY());

    mLocalMap->saveFogOfWar(store);
    mObjects->removeCell(store);
    mActors->removeCell(store);
    mDebugging->cellRemoved(store);
}

void RenderingManager::removeWater ()
{
    mWater->setActive(false);
}

bool RenderingManager::toggleWater()
{
    return mWater->toggle();
}

bool RenderingManager::toggleWorld()
{
    mRenderWorld = !mRenderWorld;

    int visibilityMask = mRenderWorld ? ~int(0) : 0;
    mRendering.getViewport()->setVisibilityMask(visibilityMask);
    return mRenderWorld;
}

void RenderingManager::cellAdded (MWWorld::CellStore *store)
{
    if (store->isExterior())
        mTerrain->loadCell(store->getCell()->getGridX(), store->getCell()->getGridY());

    mObjects->buildStaticGeometry (*store);
    sh::Factory::getInstance().unloadUnreferencedMaterials();
    mDebugging->cellAdded(store);
}

void RenderingManager::addObject (const MWWorld::Ptr& ptr, const std::string& model){
    const MWWorld::Class& class_ =
            ptr.getClass();
    class_.insertObjectRendering(ptr, model, *this);
}

void RenderingManager::removeObject (const MWWorld::Ptr& ptr)
{
    if (!mObjects->deleteObject (ptr))
        mActors->deleteObject (ptr);
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

void RenderingManager::rotateObject(const MWWorld::Ptr &ptr)
{
    Ogre::Vector3 rot(ptr.getRefData().getPosition().rot);

    if(ptr.getRefData().getHandle() == mCamera->getHandle() &&
       !mCamera->isVanityOrPreviewModeEnabled())
        mCamera->rotateCamera(-rot, false);

    Ogre::Quaternion newo = Ogre::Quaternion(Ogre::Radian(rot.z), Ogre::Vector3::NEGATIVE_UNIT_Z);
    if(!ptr.getClass().isActor())
        newo = Ogre::Quaternion(Ogre::Radian(rot.x), Ogre::Vector3::NEGATIVE_UNIT_X) *
               Ogre::Quaternion(Ogre::Radian(rot.y), Ogre::Vector3::NEGATIVE_UNIT_Y) * newo;
    ptr.getRefData().getBaseNode()->setOrientation(newo);
}

void
RenderingManager::updateObjectCell(const MWWorld::Ptr &old, const MWWorld::Ptr &cur)
{
    if (!old.getRefData().getBaseNode())
        return;
    Ogre::SceneNode *child =
        mRendering.getScene()->getSceneNode(old.getRefData().getHandle());

    Ogre::SceneNode *parent = child->getParentSceneNode();
    parent->removeChild(child);

    if (old.getClass().isActor()) {
        mActors->updateObjectCell(old, cur);
    } else {
        mObjects->updateObjectCell(old, cur);
    }
}

void RenderingManager::updatePlayerPtr(const MWWorld::Ptr &ptr)
{
    if(mPlayerAnimation)
        mPlayerAnimation->updatePtr(ptr);
    if(mCamera->getHandle() == ptr.getRefData().getHandle())
        attachCameraTo(ptr);
}

void RenderingManager::rebuildPtr(const MWWorld::Ptr &ptr)
{
    NpcAnimation *anim = NULL;
    if(ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
        anim = mPlayerAnimation;
    else if(ptr.getClass().isActor())
        anim = dynamic_cast<NpcAnimation*>(mActors->getAnimation(ptr));
    if(anim)
    {
        anim->rebuild();
        if(mCamera->getHandle() == ptr.getRefData().getHandle())
        {
            attachCameraTo(ptr);
            mCamera->setAnimation(anim);
        }
    }
}

void RenderingManager::update (float duration, bool paused)
{
    if (MWBase::Environment::get().getStateManager()->getState()==
        MWBase::StateManager::State_NoGame)
        return;

    MWBase::World *world = MWBase::Environment::get().getWorld();

    MWWorld::Ptr player = world->getPlayerPtr();

    int blind = static_cast<int>(player.getClass().getCreatureStats(player).getMagicEffects().get(ESM::MagicEffect::Blind).getMagnitude());
    MWBase::Environment::get().getWindowManager()->setBlindness(std::max(0, std::min(100, blind)));
    setAmbientMode();

    if (player.getClass().getNpcStats(player).isWerewolf())
        MWBase::Environment::get().getWindowManager()->setWerewolfOverlay(mCamera->isFirstPerson());

    // player position
    MWWorld::RefData &data = player.getRefData();
    Ogre::Vector3 playerPos(data.getPosition().pos);

    mCamera->setCameraDistance();
    if(!mCamera->isFirstPerson())
    {
        Ogre::Vector3 orig, dest;
        mCamera->getPosition(orig, dest);

        btVector3 btOrig(orig.x, orig.y, orig.z);
        btVector3 btDest(dest.x, dest.y, dest.z);
        std::pair<bool,float> test = mPhysicsEngine->sphereCast(mRendering.getCamera()->getNearClipDistance()*2.5f, btOrig, btDest);
        if(test.first)
            mCamera->setCameraDistance(test.second * orig.distance(dest), false, false);
    }

    // Sink the camera while sneaking
    bool isSneaking = player.getClass().getCreatureStats(player).getStance(MWMechanics::CreatureStats::Stance_Sneak);
    bool isInAir = !world->isOnGround(player);
    bool isSwimming = world->isSwimming(player);

    static const float i1stPersonSneakDelta = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
            .find("i1stPersonSneakDelta")->getFloat();
    if(!paused && isSneaking && !(isSwimming || isInAir))
        mCamera->setSneakOffset(i1stPersonSneakDelta);

    mOcclusionQuery->update(duration);

    mRendering.update(duration);

    Ogre::ControllerManager::getSingleton().setTimeFactor(paused ? 0.f : 1.f);

    Ogre::Vector3 cam = mRendering.getCamera()->getRealPosition();

    applyFog(world->isUnderwater(player.getCell(), cam));

    mCamera->update(duration, paused);

    Ogre::SceneNode *node = data.getBaseNode();
    Ogre::Quaternion orient = node->_getDerivedOrientation();
    mLocalMap->updatePlayer(playerPos, orient);

    if(paused)
        return;

    mEffectManager->update(duration, mRendering.getCamera());

    mActors->update (mRendering.getCamera());
    mPlayerAnimation->preRender(mRendering.getCamera());
    mObjects->update (duration, mRendering.getCamera());

    mSkyManager->update(duration);

    mSkyManager->setGlare(mOcclusionQuery->getSunVisibility());

    mWater->changeCell(player.getCell()->getCell());

    mWater->updateUnderwater(world->isUnderwater(player.getCell(), cam));

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

void RenderingManager::setWaterEnabled(bool enable)
{
    mWater->setActive(enable);
}

void RenderingManager::setWaterHeight(float height)
{
    mWater->setHeight(height);
}

void RenderingManager::skyEnable ()
{
    mSkyManager->enable();
    mOcclusionQuery->setSunNode(mSkyManager->getSunNode());
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

void RenderingManager::skySetMoonColour (bool red){
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
            mRendering.getCamera()->setPolygonMode(PM_WIREFRAME);
            return true;
        }
        else
        {
            mRendering.getCamera()->setPolygonMode(PM_SOLID);
            return false;
        }
    }
    else //if (mode == MWBase::World::Render_BoundingBoxes)
    {
        bool show = !mRendering.getScene()->getShowBoundingBoxes();
        mRendering.getScene()->showBoundingBoxes(show);
        return show;
    }
}

void RenderingManager::configureFog(const MWWorld::CellStore &mCell)
{
    Ogre::ColourValue color;
    color.setAsABGR (mCell.getCell()->mAmbi.mFog);

    configureFog (mCell.getCell()->mAmbi.mFogDensity, color);
}

void RenderingManager::configureFog(const float density, const Ogre::ColourValue& colour)
{
    mFogColour = colour;
    float max = Settings::Manager::getFloat("viewing distance", "Viewing distance");

    if (density == 0)
    {
        mFogStart = 0;
        mFogEnd = std::numeric_limits<float>::max();
        mRendering.getCamera()->setFarClipDistance (max);
    }
    else
    {
        mFogStart = max / (density) * Settings::Manager::getFloat("fog start factor", "Viewing distance");
        mFogEnd = max / (density) * Settings::Manager::getFloat("fog end factor", "Viewing distance");
        mRendering.getCamera()->setFarClipDistance (max / density);
    }

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
        Ogre::ColourValue clv(0.090195f, 0.115685f, 0.12745f);
        mRendering.getScene()->setFog (FOG_LINEAR, Ogre::ColourValue(clv), 0, 0, 1000);
        mRendering.getViewport()->setBackgroundColour (Ogre::ColourValue(clv));
        mWater->setViewportBackground (Ogre::ColourValue(clv));
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

void RenderingManager::configureAmbient(MWWorld::CellStore &mCell)
{
    if (mCell.getCell()->mData.mFlags & ESM::Cell::Interior)
        mAmbientColor.setAsABGR (mCell.getCell()->mAmbi.mAmbient);
    setAmbientMode();

    // Create a "sun" that shines light downwards. It doesn't look
    // completely right, but leave it for now.
    if(!mSun)
    {
        mSun = mRendering.getScene()->createLight();
        mSun->setType(Ogre::Light::LT_DIRECTIONAL);
    }
    if (mCell.getCell()->mData.mFlags & ESM::Cell::Interior)
    {
        Ogre::ColourValue colour;
        colour.setAsABGR (mCell.getCell()->mAmbi.mSunlight);
        mSun->setDiffuseColour (colour);
        mSun->setDirection(1,-1,-1);
        sunEnable(false);
    }
}

void RenderingManager::setSunColour(const Ogre::ColourValue& colour)
{
    if (!mSunEnabled) return;
    mSun->setDiffuseColour(colour);
    mSun->setSpecularColour(colour);
}

void RenderingManager::setAmbientColour(const Ogre::ColourValue& colour)
{
    mAmbientColor = colour;

    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
    int nightEye = static_cast<int>(player.getClass().getCreatureStats(player).getMagicEffects().get(ESM::MagicEffect::NightEye).getMagnitude());
    Ogre::ColourValue final = colour;
    final += Ogre::ColourValue(0.7f,0.7f,0.7f,0) * std::min(1.f, (nightEye/100.f));

    mRendering.getScene()->setAmbientLight(final);
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

void RenderingManager::setSunDirection(const Ogre::Vector3& direction, bool is_night)
{
    // direction * -1 (because 'direction' is camera to sun vector and not sun to camera),
    if (mSun) mSun->setDirection(Vector3(-direction.x, -direction.y, -direction.z));

    mSkyManager->setSunDirection(direction, is_night);
}

void RenderingManager::setGlare(bool glare)
{
    mSkyManager->setGlare(glare);
}

void RenderingManager::updateTerrain()
{
    if (mTerrain)
    {
        // Avoid updating with dims.getCenter for each cell. Player position should be good enough
        mTerrain->update(mRendering.getCamera()->getRealPosition());
        mTerrain->syncLoad();
        // need to update again so the chunks that were just loaded can be made visible
        mTerrain->update(mRendering.getCamera()->getRealPosition());
    }
}

void RenderingManager::requestMap(MWWorld::CellStore* cell)
{
    if (cell->getCell()->isExterior())
    {
        assert(mTerrain);

        Ogre::AxisAlignedBox dims = mObjects->getDimensions(cell);
        Ogre::Vector2 center (cell->getCell()->getGridX() + 0.5f, cell->getCell()->getGridY() + 0.5f);
        dims.merge(mTerrain->getWorldBoundingBox(center));

        mLocalMap->requestMap(cell, dims.getMinimum().z, dims.getMaximum().z);
    }
    else
        mLocalMap->requestMap(cell, mObjects->getDimensions(cell));
}

void RenderingManager::writeFog(MWWorld::CellStore* cell)
{
    mLocalMap->saveFogOfWar(cell);
}

void RenderingManager::disableLights(bool sun)
{
    mActors->disableLights();
    sunDisable(sun);
}

void RenderingManager::enableLights(bool sun)
{
    mActors->enableLights();
    sunEnable(sun);
}

void RenderingManager::notifyWorldSpaceChanged()
{
    mEffectManager->clear();
    mWater->clearRipples();
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
        float x = corner.x / corner.z + 0.5f;
        float y = corner.y / corner.z + 0.5f;

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
        else if (it->second == "viewing distance" && it->first == "Viewing distance")
        {
            if (!MWBase::Environment::get().getWorld()->isCellExterior() && !MWBase::Environment::get().getWorld()->isCellQuasiExterior()
                && MWBase::Environment::get().getWorld()->getPlayerPtr().mCell)
                configureFog(*MWBase::Environment::get().getWorld()->getPlayerPtr().getCell());
        }
        else if (it->first == "Video" && (
                it->second == "resolution x"
                || it->second == "resolution y"
                || it->second == "fullscreen"))
            changeRes = true;
        else if (it->first == "Video" && it->second == "window border")
            changeRes = true;
        else if (it->second == "field of view" && it->first == "General")
            mRendering.setFov(Settings::Manager::getFloat("field of view", "General"));
        else if (it->second == "gamma" && it->first == "General")
        {
            mRendering.setWindowGammaContrast(Settings::Manager::getFloat("gamma", "General"), Settings::Manager::getFloat("contrast", "General"));
        }
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
        bool fullscreen = Settings::Manager::getBool("fullscreen", "Video");
        bool windowBorder = Settings::Manager::getBool("window border", "Video");

        SDL_Window* window = mRendering.getSDLWindow();

        SDL_SetWindowFullscreen(window, 0);

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED)
            SDL_RestoreWindow(window);

        if (fullscreen)
        {
            SDL_DisplayMode mode;
            SDL_GetWindowDisplayMode(window, &mode);
            mode.w = x;
            mode.h = y;
            SDL_SetWindowDisplayMode(window, &mode);
            SDL_SetWindowFullscreen(window, fullscreen);
        }
        else
        {
            SDL_SetWindowSize(window, x, y);
            SDL_SetWindowBordered(window, windowBorder ? SDL_TRUE : SDL_FALSE);
        }
    }

    mWater->processChangedSettings(settings);

    if (rebuild)
    {
        mObjects->rebuildStaticGeometry();
        if (mTerrain)
            mTerrain->applyMaterials(Settings::Manager::getBool("enabled", "Shadows"),
                                     Settings::Manager::getBool("split", "Shadows"));
    }
}

void RenderingManager::setMenuTransparency(float val)
{
    Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().getByName("transparent.png"); std::vector<Ogre::uint32> buffer;
    buffer.resize(1);
    buffer[0] = (int(255*val) << 24) | (255 << 16) | (255 << 8) | 255;
    memcpy(tex->getBuffer()->lock(Ogre::HardwareBuffer::HBL_DISCARD), &buffer[0], 1*4);
    tex->getBuffer()->unlock();
}

void RenderingManager::windowResized(int x, int y)
{
    Settings::Manager::setInt("resolution x", "Video", x);
    Settings::Manager::setInt("resolution y", "Video", y);
    mRendering.adjustViewport();

    MWBase::Environment::get().getWindowManager()->windowResized(x,y);
}

void RenderingManager::getTriangleBatchCount(unsigned int &triangles, unsigned int &batches)
{
    batches = mRendering.getWindow()->getBatchCount();
    triangles = mRendering.getWindow()->getTriangleCount();
}

void RenderingManager::setupPlayer(const MWWorld::Ptr &ptr)
{
    ptr.getRefData().setBaseNode(mRendering.getScene()->getSceneNode("player"));
    attachCameraTo(ptr);
}

void RenderingManager::attachCameraTo(const MWWorld::Ptr &ptr)
{
    Ogre::SceneNode* cameraNode = mCamera->attachTo(ptr);
    mSkyManager->attachToNode(cameraNode);
}

void RenderingManager::renderPlayer(const MWWorld::Ptr &ptr)
{
    if(!mPlayerAnimation)
    {
        mPlayerAnimation = new NpcAnimation(ptr, ptr.getRefData().getBaseNode(), RV_Actors);
    }
    else
    {
        // Reconstruct the NpcAnimation in-place
        mPlayerAnimation->~NpcAnimation();
        new(mPlayerAnimation) NpcAnimation(ptr, ptr.getRefData().getBaseNode(), RV_Actors);
    }

    mCamera->setAnimation(mPlayerAnimation);
    mWater->removeEmitter(ptr);
    mWater->addEmitter(ptr);
    // apply race height
    MWBase::Environment::get().getWorld()->scaleObject(ptr, 1.f);
}

bool RenderingManager::vanityRotateCamera(const float *rot)
{
    if(!mCamera->isVanityOrPreviewModeEnabled())
        return false;

    Ogre::Vector3 vRot(rot);
    mCamera->rotateCamera(vRot, true);
    return true;
}

void RenderingManager::setCameraDistance(float dist, bool adjust, bool override)
{
    if(!mCamera->isVanityOrPreviewModeEnabled() && !mCamera->isFirstPerson())
    {
        if(mCamera->isNearest() && dist > 0.f)
            mCamera->toggleViewMode();
        else
            mCamera->setCameraDistance(-dist / 120.f * 10, adjust, override);
    }
    else if(mCamera->isFirstPerson() && dist < 0.f)
    {
        mCamera->toggleViewMode();
        mCamera->setCameraDistance(0.f, false, override);
    }
}

void RenderingManager::worldToInteriorMapPosition (Ogre::Vector2 position, float& nX, float& nY, int &x, int& y)
{
    return mLocalMap->worldToInteriorMapPosition (position, nX, nY, x, y);
}

Ogre::Vector2 RenderingManager::interiorMapToWorldPosition(float nX, float nY, int x, int y)
{
    return mLocalMap->interiorMapToWorldPosition(nX, nY, x, y);
}

bool RenderingManager::isPositionExplored (float nX, float nY, int x, int y, bool interior)
{
    return mLocalMap->isPositionExplored(nX, nY, x, y, interior);
}

Animation* RenderingManager::getAnimation(const MWWorld::Ptr &ptr)
{
    Animation *anim = mActors->getAnimation(ptr);

    if(!anim && ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
        anim = mPlayerAnimation;

    if (!anim)
        anim = mObjects->getAnimation(ptr);

    return anim;
}

void RenderingManager::screenshot(Image &image, int w, int h)
{
    // Create a temporary render target. We do not use the RenderWindow since we want a specific size.
    // Also, the GUI should not be visible (and it is only rendered on the RenderWindow's primary viewport)
    const std::string tempName = "@temp";
    Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().createManual(tempName,
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, w, h, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);

    float oldAspect = mRendering.getCamera()->getAspectRatio();

    mRendering.getCamera()->setAspectRatio(w / static_cast<float>(h));

    Ogre::RenderTarget* rt = texture->getBuffer()->getRenderTarget();
    Ogre::Viewport* vp = rt->addViewport(mRendering.getCamera());
    vp->setBackgroundColour(mRendering.getViewport()->getBackgroundColour());
    vp->setOverlaysEnabled(false);
    vp->setVisibilityMask(mRendering.getViewport()->getVisibilityMask());
    rt->update();

    Ogre::PixelFormat pf = rt->suggestPixelFormat();

    image.loadDynamicImage(
        OGRE_ALLOC_T(Ogre::uchar, w * h * Ogre::PixelUtil::getNumElemBytes(pf), Ogre::MEMCATEGORY_GENERAL),
        w, h, 1, pf, true // autoDelete=true, frees memory we allocate
    );
    rt->copyContentsToMemory(image.getPixelBox()); // getPixelBox returns a box sharing the same memory as the image

    Ogre::TextureManager::getSingleton().remove(tempName);
    mRendering.getCamera()->setAspectRatio(oldAspect);
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

void RenderingManager::frameStarted(float dt, bool paused)
{
    if (mTerrain)
        mTerrain->update(mRendering.getCamera()->getRealPosition());

    if (!paused)
        mWater->frameStarted(dt);
}

void RenderingManager::resetCamera()
{
    mCamera->reset();
}

float RenderingManager::getTerrainHeightAt(Ogre::Vector3 worldPos)
{
    if (!mTerrain || !mTerrain->getVisible())
        return -std::numeric_limits<float>::max();
    return mTerrain->getHeightAt(worldPos);
}

void RenderingManager::enableTerrain(bool enable)
{
    if (enable)
    {
        if (!mTerrain)
        {
            if (Settings::Manager::getBool("distant land", "Terrain"))
                mTerrain = new Terrain::DefaultWorld(mRendering.getScene(), new MWRender::TerrainStorage(true), RV_Terrain,
                                                Settings::Manager::getBool("shader", "Terrain"), Terrain::Align_XY, 1, 64);
            else
                mTerrain = new Terrain::TerrainGrid(mRendering.getScene(), new MWRender::TerrainStorage(false), RV_Terrain,
                                                Settings::Manager::getBool("shader", "Terrain"), Terrain::Align_XY);
            mTerrain->applyMaterials(Settings::Manager::getBool("enabled", "Shadows"),
                                     Settings::Manager::getBool("split", "Shadows"));
            mTerrain->update(mRendering.getCamera()->getRealPosition());
        }
        mTerrain->setVisible(true);
    }
    else if (mTerrain)
            mTerrain->setVisible(false);
}

float RenderingManager::getCameraDistance() const
{
    return mCamera->getCameraDistance();
}

void RenderingManager::spawnEffect(const std::string &model, const std::string &texture, const Vector3 &worldPosition, float scale)
{
    mEffectManager->addEffect(model, texture, worldPosition, scale);
}

void RenderingManager::clear()
{
    mLocalMap->clear();
    notifyWorldSpaceChanged();
}

} // namespace
