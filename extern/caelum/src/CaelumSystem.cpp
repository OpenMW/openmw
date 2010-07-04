/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2006-2008 Caelum team. See Contributors.txt for details.

Caelum is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Caelum is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Caelum. If not, see <http://www.gnu.org/licenses/>.
*/

#include "CaelumPrecompiled.h"
#include "CaelumSystem.h"
#include "CaelumExceptions.h"
#include "InternalUtilities.h"
#include "Astronomy.h"
#include "CaelumPlugin.h"
#include "FlatCloudLayer.h"

using namespace Ogre;

namespace Caelum
{
    const String CaelumSystem::DEFAULT_SKY_GRADIENTS_IMAGE = "EarthClearSky2.png";
    const String CaelumSystem::DEFAULT_SUN_COLOURS_IMAGE = "SunGradient.png";

    CaelumSystem::CaelumSystem
    (
        Ogre::Root *root, 
        Ogre::SceneManager *sceneMgr,
        CaelumComponent componentsToCreate/* = CAELUM_COMPONENTS_DEFAULT*/
    ):
        mOgreRoot (root),
        mSceneMgr (sceneMgr),
        mCleanup (false)
    {
        LogManager::getSingleton().logMessage ("Caelum: Initialising Caelum system...");
        //LogManager::getSingleton().logMessage ("Caelum: CaelumSystem* at d" +
        //        StringConverter::toString (reinterpret_cast<uint>(this)));

        Ogre::String uniqueId = Ogre::StringConverter::toString ((size_t)this);
        if (!CaelumPlugin::getSingletonPtr ()) {
            LogManager::getSingleton().logMessage ("Caelum: Plugin not installed; installing now.");
            new CaelumPlugin ();
            CaelumPlugin::getSingletonPtr ()->install ();
            CaelumPlugin::getSingletonPtr ()->initialise ();
        }

        mCaelumCameraNode.reset(mSceneMgr->getRootSceneNode ()->createChildSceneNode ("Caelum/CameraNode/" + uniqueId));
        mCaelumGroundNode.reset(mSceneMgr->getRootSceneNode ()->createChildSceneNode ("Caelum/GroundNode/" + uniqueId));
        mUniversalClock.reset(new UniversalClock ());

        // If the "Caelum" resource group does not exist; create it.
        // This resource group is never released; which may be bad.
        // What does ogre do for it's own runtime resources?
        Ogre::StringVector groups = ResourceGroupManager::getSingleton ().getResourceGroups ();
        if (std::find (groups.begin(), groups.end(), Caelum::RESOURCE_GROUP_NAME) == groups.end()) {
            LogManager::getSingleton ().logMessage (
                    "Caelum: Creating required internal resource group \'" + RESOURCE_GROUP_NAME + "\'");
            ResourceGroupManager::getSingleton ().createResourceGroup (Caelum::RESOURCE_GROUP_NAME);
        }

        // Autoconfigure. Calls clear first to set defaults.
        autoConfigure (componentsToCreate);
    }

    void CaelumSystem::destroySubcomponents (bool destroyEverything)
    {
        // Destroy sub-components
        setSkyDome (0);
        setSun (0);
        setImageStarfield (0);
        setPointStarfield (0);
        setCloudSystem (0);
        setPrecipitationController (0);
        setDepthComposer (0);
        setGroundFog (0);
        setMoon (0);   
        mSkyGradientsImage.reset ();
        mSunColoursImage.reset ();

        // These things can't be rebuilt.
        if (destroyEverything) {
            LogManager::getSingleton ().logMessage("Caelum: Delete UniversalClock");
            mUniversalClock.reset ();
            mCaelumCameraNode.reset ();
            mCaelumGroundNode.reset ();
        }
    }

    CaelumSystem::~CaelumSystem () {
        destroySubcomponents (true);
        LogManager::getSingleton ().logMessage ("Caelum: CaelumSystem destroyed.");
    }

    void CaelumSystem::clear()
    {
        // Destroy all subcomponents first.
        destroySubcomponents (false);

        // Some "magical" behaviour.
        mAutoMoveCameraNode = true;
        mAutoNotifyCameraChanged = true;
        mAutoAttachViewportsToComponents = true;
        mAutoViewportBackground = true;

        // Default lookups.
        setSkyGradientsImage(DEFAULT_SKY_GRADIENTS_IMAGE);
        setSunColoursImage(DEFAULT_SUN_COLOURS_IMAGE);

        // Fog defaults.
        setManageSceneFog (true);
        mGlobalFogDensityMultiplier = 1;
        mGlobalFogColourMultiplier = Ogre::ColourValue(1.0, 1.0, 1.0, 1.0);
        mSceneFogDensityMultiplier = 1;
        mSceneFogColourMultiplier = Ogre::ColourValue(0.7, 0.7, 0.7, 0.7);
        mGroundFogDensityMultiplier = 1;
        mGroundFogColourMultiplier = Ogre::ColourValue(1.0, 1.0, 1.0, 1.0);

        // Ambient lighting.
        setManageAmbientLight (true);
        setMinimumAmbientLight (Ogre::ColourValue (0.1, 0.1, 0.3));
        mEnsureSingleLightSource = false;
        mEnsureSingleShadowSource = false;

        // Observer time & position. J2000 is midday.
        mObserverLatitude = Ogre::Degree(45);
        mObserverLongitude = Ogre::Degree(0);
        mUniversalClock->setJulianDay (Astronomy::J2000);
    }

    void CaelumSystem::autoConfigure
    (
        CaelumComponent componentsToCreate/* = CAELUM_COMPONENTS_DEFAULT*/
    )
    {
        // Clear everything; revert to default.
        clear();

        if (componentsToCreate == 0) {
            // Nothing to do. Don't print junk if not creating anything.
            return;
        }
        LogManager::getSingleton ().logMessage ("Caelum: Creating caelum sub-components.");

        // Init skydome
        if (componentsToCreate & CAELUM_COMPONENT_SKY_DOME) {
            try {
                this->setSkyDome (new SkyDome (mSceneMgr, getCaelumCameraNode ()));
            } catch (Caelum::UnsupportedException& ex) {
                LogManager::getSingleton ().logMessage (
                        "Caelum: Failed to initialize skydome: " + ex.getFullDescription());
            }
        }
        
        // Init sun
        if (componentsToCreate & CAELUM_COMPONENT_SUN) {
            try {
                this->setSun (new SpriteSun (mSceneMgr, getCaelumCameraNode ()));
                this->getSun ()->setAmbientMultiplier (Ogre::ColourValue (0.5, 0.5, 0.5));
                this->getSun ()->setDiffuseMultiplier (Ogre::ColourValue (3, 3, 2.7));
                this->getSun ()->setSpecularMultiplier (Ogre::ColourValue (5, 5, 5));

                this->getSun ()->setAutoDisable (true);
                this->getSun ()->setAutoDisableThreshold (0.05);
            } catch (Caelum::UnsupportedException& ex) {
                LogManager::getSingleton ().logMessage (
                        "Caelum: Failed to initialize sun: " + ex.getFullDescription());
            }
        }

        // Init moon
        if (componentsToCreate & CAELUM_COMPONENT_MOON) {
            try {
                this->setMoon (new Moon (mSceneMgr, getCaelumCameraNode ()));
                this->getMoon ()->setAutoDisable (true);
                this->getMoon ()->setAutoDisableThreshold (0.05);
            } catch (Caelum::UnsupportedException& ex) {
                LogManager::getSingleton ().logMessage (
                        "Caelum: Failed to initialize moon: " + ex.getFullDescription());
            }
        }
        if (componentsToCreate & CAELUM_COMPONENT_IMAGE_STARFIELD) {
            try {
                this->setImageStarfield (new ImageStarfield (mSceneMgr, getCaelumCameraNode ()));
            } catch (Caelum::UnsupportedException& ex) {
                LogManager::getSingleton ().logMessage (
                        "Caelum: Failed to initialize the old image starfield: " + ex.getFullDescription());
            }
        }
        if (componentsToCreate & CAELUM_COMPONENT_POINT_STARFIELD) {
            try {
                this->setPointStarfield (new PointStarfield (mSceneMgr, getCaelumCameraNode ()));
            } catch (Caelum::UnsupportedException& ex) {
                LogManager::getSingleton ().logMessage (
                        "Caelum: Failed to initialize starfield: " + ex.getFullDescription());
            }
        }
        if (componentsToCreate & CAELUM_COMPONENT_GROUND_FOG) {
            try {
               this->setGroundFog (new GroundFog (mSceneMgr, getCaelumCameraNode ()));
            } catch (Caelum::UnsupportedException& ex) {
                LogManager::getSingleton ().logMessage (
                        "Caelum: Failed to initialize ground fog: " + ex.getFullDescription());
            }
        }
        if (componentsToCreate & CAELUM_COMPONENT_CLOUDS) {
            try {
			    this->setCloudSystem (new CloudSystem (mSceneMgr, getCaelumGroundNode ()));
                getCloudSystem ()->createLayerAtHeight (3000);		
                getCloudSystem ()->getLayer (0)->setCloudCover (0.3);
            } catch (Caelum::UnsupportedException& ex) {
                LogManager::getSingleton ().logMessage (
                        "Caelum: Failed to initialize clouds: " + ex.getFullDescription());
            }
        }
        if (componentsToCreate & CAELUM_COMPONENT_PRECIPITATION) {
            try {
                this->setPrecipitationController (new PrecipitationController (mSceneMgr));
            } catch (Caelum::UnsupportedException& ex) {
                LogManager::getSingleton ().logMessage (
                        "Caelum: Failed to initialize precipitation: " + ex.getFullDescription());
            }
        }
        if (componentsToCreate & CAELUM_COMPONENT_SCREEN_SPACE_FOG) {
            try {
                this->setDepthComposer (new DepthComposer (mSceneMgr));
            } catch (Caelum::UnsupportedException& ex) {
                LogManager::getSingleton ().logMessage (
                        "Caelum: Failed to initialize precipitation: " + ex.getFullDescription());
            }
        }

        LogManager::getSingleton ().logMessage ("Caelum: DONE initializing");
    }

    void CaelumSystem::shutdown (const bool cleanup) {
        LogManager::getSingleton ().logMessage ("Caelum: Shutting down Caelum system...");

        destroySubcomponents (true);

        if (cleanup) {
            mOgreRoot->removeFrameListener (this);
            delete this;
        } else {
            // We'll delete later. Make sure we're registered as a frame listener, or we'd leak.
            mOgreRoot->addFrameListener(this);
            mCleanup = true;
        }
    }

    void CaelumSystem::attachViewportImpl (Ogre::Viewport* vp)
    {
        LogManager::getSingleton().getDefaultLog ()->logMessage (
                "CaelumSystem: Attached to"
                " viewport " + StringConverter::toString ((long)vp) +
                " render target " + vp->getTarget ()->getName ());
        if (getAutoAttachViewportsToComponents ()) {
            if (getPrecipitationController ()) {
                getPrecipitationController ()->createViewportInstance (vp);
            }
            if (getDepthComposer ()) {
                getDepthComposer ()->createViewportInstance (vp);
            }
        }
    }

    void CaelumSystem::detachViewportImpl (Ogre::Viewport* vp)
    {
        LogManager::getSingleton().getDefaultLog ()->logMessage (
                "CaelumSystem: Detached from "
                " viewport " + StringConverter::toString ((long)vp) +
                " render target " + vp->getTarget ()->getName ());
        if (getAutoAttachViewportsToComponents ()) {
            if (getPrecipitationController ()) {
                getPrecipitationController ()->destroyViewportInstance (vp);
            }
            if (getDepthComposer ()) {
                getDepthComposer ()->destroyViewportInstance (vp);
            }
        }
    }
    
    void CaelumSystem::attachViewport (Ogre::Viewport* vp)
    {
        bool found = !mAttachedViewports.insert (vp).second;
        if (!found) {
            attachViewportImpl (vp);
        }
    }

    void CaelumSystem::detachViewport (Ogre::Viewport* vp)
    {
        std::set<Viewport*>::size_type erase_result = mAttachedViewports.erase(vp);
        assert(erase_result == 0 || erase_result == 1);
        bool found = erase_result == 1;
        if (found) {
            detachViewportImpl (vp);
        }
    }

    void CaelumSystem::detachAllViewports ()
    {
        std::set<Viewport*>::const_iterator it = mAttachedViewports.begin(), end = mAttachedViewports.end();
        for (; it != end; ++it) {
            detachViewportImpl (*it);
        }
        mAttachedViewports.clear();
    }

    bool CaelumSystem::isViewportAttached (Ogre::Viewport* vp) const {
        return mAttachedViewports.find (vp) != mAttachedViewports.end();
    }

    void CaelumSystem::setSkyDome (SkyDome *obj) {
        mSkyDome.reset (obj);
    }

    void CaelumSystem::setSun (BaseSkyLight* obj) {
        mSun.reset (obj);
    }

    void CaelumSystem::setMoon (Moon* obj) {
        mMoon.reset (obj);
    }

    void CaelumSystem::setImageStarfield (ImageStarfield* obj) {
        mImageStarfield.reset (obj);
    }

    void CaelumSystem::setPointStarfield (PointStarfield* obj) {
        mPointStarfield.reset (obj);
    }

    void CaelumSystem::setGroundFog (GroundFog* obj) {
        mGroundFog.reset (obj);
    }

    void CaelumSystem::setCloudSystem (CloudSystem* obj) {
        mCloudSystem.reset (obj);
    }

    void CaelumSystem::setPrecipitationController (PrecipitationController* newptr) {
        PrecipitationController* oldptr = getPrecipitationController ();
        if (oldptr == newptr) {
            return;
        }
        // Detach old
        if (getAutoAttachViewportsToComponents() && oldptr) {
            std::for_each (mAttachedViewports.begin(), mAttachedViewports.end(),
                    std::bind1st (std::mem_fun (&PrecipitationController::destroyViewportInstance), oldptr));
        }
        // Attach new.
        if (getAutoAttachViewportsToComponents() && newptr) {
            std::for_each (mAttachedViewports.begin(), mAttachedViewports.end(),
                    std::bind1st (std::mem_fun (&PrecipitationController::createViewportInstance), newptr));
        }
        mPrecipitationController.reset(newptr);
    }

    void CaelumSystem::setDepthComposer (DepthComposer* ptr) {
        mDepthComposer.reset(ptr);
        if (getAutoAttachViewportsToComponents() && getDepthComposer ()) {
            std::for_each (
                    mAttachedViewports.begin(), mAttachedViewports.end(),
                    std::bind1st (
                            std::mem_fun (&DepthComposer::createViewportInstance),
                            getDepthComposer ()));
        }
    }

    void CaelumSystem::preViewportUpdate (const Ogre::RenderTargetViewportEvent &e) {
        Ogre::Viewport *viewport = e.source;
        Ogre::Camera *camera = viewport->getCamera ();

        if (getAutoViewportBackground ()) {
            viewport->setBackgroundColour (Ogre::ColourValue::Black);
        }
        if (getAutoNotifyCameraChanged ()) {
            this->notifyCameraChanged (camera);
        }
    }

    void CaelumSystem::notifyCameraChanged(Ogre::Camera* cam)
    {
        // Move camera node.
        if (getAutoMoveCameraNode ()) {
            mCaelumCameraNode->setPosition (cam->getDerivedPosition());
            mCaelumCameraNode->_update (true, true);
        }

        if (getSkyDome ()) {
            getSkyDome ()->notifyCameraChanged (cam);
        }

        if (getSun ()) {
            getSun ()->notifyCameraChanged (cam);
        }

        if (getMoon ()) {
            getMoon ()->notifyCameraChanged (cam);
        }

        if (getImageStarfield ()) {
            getImageStarfield ()->notifyCameraChanged (cam);
        }

        if (getPointStarfield ()) {
            getPointStarfield ()->notifyCameraChanged (cam);
        }
        
        if (getGroundFog ()) {
            getGroundFog ()->notifyCameraChanged (cam);
        }
    }
                    
    bool CaelumSystem::frameStarted (const Ogre::FrameEvent &e) {
        if (mCleanup) {
            // Delayed destruction.
            mOgreRoot->removeFrameListener (this);
            delete this;
            return true;
        }

        updateSubcomponents(e.timeSinceLastFrame);

        return true;
    }

    void CaelumSystem::updateSubcomponents (Real timeSinceLastFrame)
    {
        /*
        LogManager::getSingleton().getDefaultLog()->logMessage(
                "CaelumSystem::updateSubcomponents: " +
                StringConverter::toString (timeSinceLastFrame, 10));
        */

        mUniversalClock->update (timeSinceLastFrame);

        // Timing variables
        LongReal julDay = mUniversalClock->getJulianDay ();
        LongReal relDayTime = fmod(julDay, 1);
        Real secondDiff = timeSinceLastFrame * mUniversalClock->getTimeScale ();

        // Get astronomical parameters.
        Ogre::Vector3 sunDir = getSunDirection(julDay);    
        Ogre::Vector3 moonDir = getMoonDirection(julDay);  
        Real moonPhase = getMoonPhase(julDay);    

        // Get parameters from sky colour model.
        Real fogDensity = getFogDensity (relDayTime, sunDir);           
        Ogre::ColourValue fogColour = getFogColour (relDayTime, sunDir);                  
        Ogre::ColourValue sunLightColour = getSunLightColour (relDayTime, sunDir);
        Ogre::ColourValue sunSphereColour = getSunSphereColour (relDayTime, sunDir);
        Ogre::ColourValue moonLightColour = getMoonLightColour (moonDir);
        Ogre::ColourValue moonBodyColour = getMoonBodyColour (moonDir); 

        fogDensity *= mGlobalFogDensityMultiplier;
        fogColour = fogColour * mGlobalFogColourMultiplier;

        // Update image starfield
        if (getImageStarfield ()) {
            getImageStarfield ()->update (relDayTime);
            getImageStarfield ()->setInclination (-getObserverLatitude ());
        }

        // Update point starfield
        if (getPointStarfield ()) {
            getPointStarfield ()->setObserverLatitude (getObserverLatitude ());
            getPointStarfield ()->setObserverLongitude (getObserverLongitude ());
            getPointStarfield ()->_update (relDayTime);
        }

        // Update skydome.
        if (getSkyDome ()) {
            getSkyDome ()->setSunDirection (sunDir);
            getSkyDome ()->setHazeColour (fogColour * mSceneFogColourMultiplier);
        }

        // Update scene fog.
        if (getManageSceneFog ()) {
            mSceneMgr->setFog (Ogre::FOG_EXP2,
                    fogColour * mSceneFogColourMultiplier,
                    fogDensity * mSceneFogDensityMultiplier);
        }

        // Update ground fog.
        if (getGroundFog ()) {
            getGroundFog ()->setColour (fogColour * mGroundFogColourMultiplier);
            getGroundFog ()->setDensity (fogDensity * mGroundFogDensityMultiplier);
        }

        // Update sun
        if (getSun ()) {
            mSun->update (sunDir, sunLightColour, sunSphereColour);
        }

        // Update moon.
        if (getMoon ()) {
            mMoon->update (
                    moonDir,
                    moonLightColour,
                    moonBodyColour);
            mMoon->setPhase (moonPhase);
        }

        // Update clouds
        if (getCloudSystem ()) {
            getCloudSystem ()->update (
                    secondDiff, sunDir, sunLightColour, fogColour, sunSphereColour);
        }

        // Update precipitation
        if (getPrecipitationController ()) {
            getPrecipitationController ()->update (secondDiff, fogColour);
        }

        // Update screen space fog
        if (getDepthComposer ()) {
            getDepthComposer ()->update ();
            getDepthComposer ()->setSunDirection (sunDir);
            getDepthComposer ()->setHazeColour (fogColour);
            getDepthComposer ()->setGroundFogColour (fogColour * mGroundFogColourMultiplier);
            getDepthComposer ()->setGroundFogDensity (fogDensity * mGroundFogDensityMultiplier);
        }

        // Update ambient lighting.
        if (getManageAmbientLight ()) {
            Ogre::ColourValue ambient = Ogre::ColourValue::Black;
            if (getMoon ()) {
                ambient += getMoon ()->getLightColour () * getMoon ()->getAmbientMultiplier ();
            }
            if (getSun ()) {
                ambient += getSun ()->getLightColour () * getSun ()->getAmbientMultiplier ();
            }
            ambient.r = std::max(ambient.r, mMinimumAmbientLight.r);
            ambient.g = std::max(ambient.g, mMinimumAmbientLight.g);
            ambient.b = std::max(ambient.b, mMinimumAmbientLight.b);
            ambient.a = std::max(ambient.a, mMinimumAmbientLight.a);
            // Debug ambient factos (ick).
            /*
            LogManager::getSingleton().logMessage (
                        "Sun is " + StringConverter::toString(sunLightColour) + "\n"
                        "Moon is " + StringConverter::toString(moonLightColour) + "\n"
                        "Ambient is " + StringConverter::toString(ambient) + "\n"
                        );
             */
            mSceneMgr->setAmbientLight (ambient);
        }

        if (getSun() && getMoon ()) {
            Ogre::Real moonBrightness = moonLightColour.r + moonLightColour.g + moonLightColour.b + moonLightColour.a;
            Ogre::Real sunBrightness = sunLightColour.r + sunLightColour.g + sunLightColour.b + sunLightColour.a;
            bool sunBrighterThanMoon = (sunBrightness > moonBrightness);

            if (getEnsureSingleLightSource ()) {
                getMoon ()->setForceDisable (sunBrighterThanMoon);
                getSun ()->setForceDisable (!sunBrighterThanMoon);
            }
            if (getEnsureSingleShadowSource ()) {
                getMoon ()->getMainLight ()->setCastShadows (!sunBrighterThanMoon);
                getSun ()->getMainLight ()->setCastShadows (sunBrighterThanMoon);
            }
        }
    }

    void CaelumSystem::setManageSceneFog (bool value) {
        mManageSceneFog = value;
        // Prevent having some stale values around.
        if (!value) {
            mSceneMgr->setFog (Ogre::FOG_NONE);
        }
    }

    bool CaelumSystem::getManageSceneFog () const {
        return mManageSceneFog;
    }

    void CaelumSystem::setSceneFogDensityMultiplier (Real value) {
        mSceneFogDensityMultiplier = value;
    }

    Real CaelumSystem::getSceneFogDensityMultiplier () const {
        return mSceneFogDensityMultiplier;
    }

    void CaelumSystem::setGroundFogDensityMultiplier (Real value) {
        mGroundFogDensityMultiplier = value;
    }

    Real CaelumSystem::getGroundFogDensityMultiplier () const {
        return mGroundFogDensityMultiplier;
    }

    void CaelumSystem::setGlobalFogDensityMultiplier (Real value) {
        mGlobalFogDensityMultiplier = value;
    }

    Real CaelumSystem::getGlobalFogDensityMultiplier () const {
        return mGlobalFogDensityMultiplier;
    }

    void CaelumSystem::setSkyGradientsImage (const Ogre::String &filename) {
        mSkyGradientsImage.reset(new Ogre::Image ());
        mSkyGradientsImage->load (filename, RESOURCE_GROUP_NAME);
    }

    void CaelumSystem::setSunColoursImage (const Ogre::String &filename) {
        mSunColoursImage.reset(new Ogre::Image ());
        mSunColoursImage->load (filename, RESOURCE_GROUP_NAME);
    }

    Ogre::ColourValue CaelumSystem::getFogColour (Real time, const Ogre::Vector3 &sunDir) {
        if (!mSkyGradientsImage.get()) {
            return Ogre::ColourValue::Black;
        }

        Real elevation = sunDir.dotProduct (Ogre::Vector3::UNIT_Y) * 0.5 + 0.5;
        Ogre::ColourValue col = InternalUtilities::getInterpolatedColour (elevation, 1, mSkyGradientsImage.get(), false);
        return col;
    }

    Real CaelumSystem::getFogDensity (Real time, const Ogre::Vector3 &sunDir)
    {
        if (!mSkyGradientsImage.get()) {
            return 0;
        }

        Real elevation = sunDir.dotProduct (Ogre::Vector3::UNIT_Y) * 0.5 + 0.5;
        Ogre::ColourValue col = InternalUtilities::getInterpolatedColour (elevation, 1, mSkyGradientsImage.get(), false);
        return col.a;
    }

    Ogre::ColourValue CaelumSystem::getSunSphereColour (Real time, const Ogre::Vector3 &sunDir)
    {
        if (!mSunColoursImage.get()) {
            return Ogre::ColourValue::White;
        }

        Real elevation = sunDir.dotProduct (Ogre::Vector3::UNIT_Y);
        elevation = elevation * 2 + 0.4;
        return InternalUtilities::getInterpolatedColour (elevation, 1, mSunColoursImage.get(), false);
    }

    Ogre::ColourValue CaelumSystem::getSunLightColour (Real time, const Ogre::Vector3 &sunDir)
    {
        if (!mSkyGradientsImage.get()) {
            exit(-1);
            return Ogre::ColourValue::White;
        }
        Real elevation = sunDir.dotProduct (Ogre::Vector3::UNIT_Y) * 0.5 + 0.5;

        // Hack: return averaged sky colours.
        // Don't use an alpha value for lights, this can cause nasty problems.
        Ogre::ColourValue col = InternalUtilities::getInterpolatedColour (elevation, elevation, mSkyGradientsImage.get(), false);
        Real val = (col.r + col.g + col.b) / 3;
        col = Ogre::ColourValue(val, val, val, 1.0);
        assert(Ogre::Math::RealEqual(col.a, 1));
        return col;
    }

    Ogre::ColourValue CaelumSystem::getMoonBodyColour (const Ogre::Vector3 &moonDir) {
        return Ogre::ColourValue::White;
    }

    Ogre::ColourValue CaelumSystem::getMoonLightColour (const Ogre::Vector3 &moonDir)
    {
        if (!mSkyGradientsImage.get()) {
            return Ogre::ColourValue::Blue;
        }
        // Scaled version of getSunLightColor
        Real elevation = moonDir.dotProduct (Ogre::Vector3::UNIT_Y) * 0.5 + 0.5;
        Ogre::ColourValue col = InternalUtilities::getInterpolatedColour (elevation, elevation, mSkyGradientsImage.get(), false);
        Real val = (col.r + col.g + col.b) / 3;
        col = Ogre::ColourValue(val / 2.5f, val / 2.5f, val / 2.5f, 1.0);
        assert(Ogre::Math::RealEqual(col.a, 1));
        return col;
    }

    const Ogre::Vector3 CaelumSystem::makeDirection (
            Ogre::Degree azimuth, Ogre::Degree altitude)
    {
        Ogre::Vector3 res;
        res.z = -Ogre::Math::Cos (azimuth) * Ogre::Math::Cos (altitude);  // North 
        res.x =  Ogre::Math::Sin (azimuth) * Ogre::Math::Cos (altitude);  // East
        res.y = -Ogre::Math::Sin (altitude); // Zenith
        return res;
    }

    const Ogre::Vector3 CaelumSystem::getSunDirection (LongReal jday)
    {
        Ogre::Degree azimuth, altitude;
        {
            ScopedHighPrecissionFloatSwitch precissionSwitch;
    		                  
		    Astronomy::getHorizontalSunPosition(jday,
                    getObserverLongitude(), getObserverLatitude(),
                    azimuth, altitude);		
        }
        Ogre::Vector3 res = makeDirection(azimuth, altitude);

        return res;
    }

	const Ogre::Vector3 CaelumSystem::getMoonDirection (LongReal jday)
    {
        Ogre::Degree azimuth, altitude;
        {
            ScopedHighPrecissionFloatSwitch precissionSwitch;

            Astronomy::getHorizontalMoonPosition(jday,
                    getObserverLongitude (), getObserverLatitude (),
                    azimuth, altitude);
        }	
        Ogre::Vector3 res = makeDirection(azimuth, altitude);

		return res;
	}

    const Ogre::Real CaelumSystem::getMoonPhase (LongReal jday)
    {
        // Calculates julian days since January 22, 2008 13:36 (full moon)
        // and divides by the time between lunations (synodic month)
        LongReal T = (jday - 2454488.0665L) / 29.531026L;

        T = fabs(fmod(T, 1));
        return -fabs(-4 * T + 2) + 2;
    }

    void CaelumSystem::forceSubcomponentQueryFlags (uint flags)
    {
        if (getSkyDome ()) getSkyDome ()->setQueryFlags (flags);
        if (getSun ()) getSun ()->setQueryFlags (flags);
        if (getMoon ()) getMoon ()->setQueryFlags (flags);
        if (getImageStarfield ()) getImageStarfield ()->setQueryFlags (flags);
        if (getPointStarfield ()) getPointStarfield ()->setQueryFlags (flags);        
        if (getGroundFog ()) getGroundFog ()->setQueryFlags (flags);
        if (getCloudSystem ()) getCloudSystem ()->forceLayerQueryFlags (flags);
    }

    void CaelumSystem::forceSubcomponentVisibilityFlags (uint flags)
    {
        if (getSkyDome ()) getSkyDome ()->setVisibilityFlags (flags);
        if (getSun ()) getSun ()->setVisibilityFlags (flags);
        if (getMoon ()) getMoon ()->setVisibilityFlags (flags);
        if (getImageStarfield ()) getImageStarfield ()->setVisibilityFlags (flags);
        if (getPointStarfield ()) getPointStarfield ()->setVisibilityFlags (flags);        
        if (getGroundFog ()) getGroundFog ()->setVisibilityFlags (flags);
        if (getCloudSystem ()) getCloudSystem ()->forceLayerVisibilityFlags (flags);
    }
}
