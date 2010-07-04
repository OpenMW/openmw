/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2008 Caelum team. See Contributors.txt for details.

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
#include "CaelumPrerequisites.h"

#if CAELUM_TYPE_DESCRIPTORS

#include "TypeDescriptor.h"
#include "CaelumSystem.h"
#include "FlatCloudLayer.h"

using namespace Ogre;

namespace Caelum
{
    CaelumDefaultTypeDescriptorData::CaelumDefaultTypeDescriptorData ():
            CaelumSystemTypeDescriptor(0),
            PointStarfieldTypeDescriptor(0),
            BaseSkyLightTypeDescriptor(0),
            GroundFogTypeDescriptor(0),
            PrecipitationTypeDescriptor(0),
            DepthComposerTypeDescriptor(0),
            FlatCloudLayerTypeDescriptor(0),
            SkyDomeTypeDescriptor(0)
    {
        try {
            load ();
        } catch (...) {
            unload ();
            throw;
        }
    }

    CaelumDefaultTypeDescriptorData::~CaelumDefaultTypeDescriptorData ()
    {
        unload ();
    }

    template<class T>
    inline void delete_zero(T*& member) {
        // Remember: delete 0 is a legal no-op.
        delete member;
        member = 0;
    }

    void CaelumDefaultTypeDescriptorData::unload ()
    {
        delete_zero(CaelumSystemTypeDescriptor);
        delete_zero(PointStarfieldTypeDescriptor);
        delete_zero(BaseSkyLightTypeDescriptor);
        delete_zero(GroundFogTypeDescriptor);
        delete_zero(PrecipitationTypeDescriptor);
        delete_zero(DepthComposerTypeDescriptor);
        delete_zero(FlatCloudLayerTypeDescriptor);
        delete_zero(SkyDomeTypeDescriptor);
    }

    void CaelumDefaultTypeDescriptorData::load ()
    {
        if (!CaelumSystemTypeDescriptor)
        {
            std::auto_ptr<DefaultTypeDescriptor> td (new DefaultTypeDescriptor ());

            // Timing settings.
            td->add("time_scale",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, Real, Real, Real>(
                            &Caelum::CaelumSystem::getTimeScale,
                            &Caelum::CaelumSystem::setTimeScale));
            td->add("julian_day",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, LongReal, LongReal, LongReal>(
                            &Caelum::CaelumSystem::getJulianDay,
                            &Caelum::CaelumSystem::setJulianDay));

            // Latitude/longitude
            td->add("latitude",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, Degree, Degree, const Degree>(
                            &Caelum::CaelumSystem::getObserverLatitude,
                            &Caelum::CaelumSystem::setObserverLatitude));
            td->add("longitude",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, Degree, Degree, const Degree>(
                            &Caelum::CaelumSystem::getObserverLongitude,
                            &Caelum::CaelumSystem::setObserverLongitude));

            // Fog settings.
            td->add("global_fog_density_multiplier",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, Real, Real, Real>(
                            &Caelum::CaelumSystem::getGlobalFogDensityMultiplier,
                            &Caelum::CaelumSystem::setGlobalFogDensityMultiplier));
            td->add("global_fog_colour_multiplier",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, ColourValue>(
                            &Caelum::CaelumSystem::getGlobalFogColourMultiplier,
                            &Caelum::CaelumSystem::setGlobalFogColourMultiplier));
            td->add("manage_scene_fog",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, bool, bool, bool>(
                            &Caelum::CaelumSystem::getManageSceneFog,
                            &Caelum::CaelumSystem::setManageSceneFog));
            td->add("scene_fog_density_multiplier",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, Real, Real, Real>(
                            &Caelum::CaelumSystem::getSceneFogDensityMultiplier,
                            &Caelum::CaelumSystem::setSceneFogDensityMultiplier));
            td->add("scene_fog_colour_multiplier",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, ColourValue>(
                            &Caelum::CaelumSystem::getSceneFogColourMultiplier,
                            &Caelum::CaelumSystem::setSceneFogColourMultiplier));
            td->add("ground_fog_density_multiplier",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, Real, Real, Real>(
                            &Caelum::CaelumSystem::getGroundFogDensityMultiplier,
                            &Caelum::CaelumSystem::setGroundFogDensityMultiplier));
            td->add("ground_fog_colour_multiplier",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, ColourValue>(
                            &Caelum::CaelumSystem::getGroundFogColourMultiplier,
                            &Caelum::CaelumSystem::setGroundFogColourMultiplier));

            // Lighting settings.
            td->add("manage_ambient_light",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, bool, bool, bool>(
                            &Caelum::CaelumSystem::getManageAmbientLight,
                            &Caelum::CaelumSystem::setManageAmbientLight));
            td->add("minimum_ambient_light",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, ColourValue>(
                            &Caelum::CaelumSystem::getMinimumAmbientLight,
                            &Caelum::CaelumSystem::setMinimumAmbientLight));
            td->add("ensure_single_light_source",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, bool, bool, bool>(
                            &Caelum::CaelumSystem::getEnsureSingleLightSource,
                            &Caelum::CaelumSystem::setEnsureSingleLightSource));
            td->add("ensure_single_shadow_source",
                    new AccesorPropertyDescriptor<Caelum::CaelumSystem, bool, bool, bool>(
                            &Caelum::CaelumSystem::getEnsureSingleShadowSource,
                            &Caelum::CaelumSystem::setEnsureSingleShadowSource));

            CaelumSystemTypeDescriptor = td.release ();
        }

        if (!PointStarfieldTypeDescriptor)
        {
            std::auto_ptr<DefaultTypeDescriptor> td (new DefaultTypeDescriptor ());
            td->add("magnitude_scale",
                    new AccesorPropertyDescriptor<Caelum::PointStarfield, Real, Real, Real>(
                            &Caelum::PointStarfield::getMagnitudeScale,
                            &Caelum::PointStarfield::setMagnitudeScale));
            td->add("mag0_pixel_size",
                    new AccesorPropertyDescriptor<Caelum::PointStarfield, Real, Real, Real>(
                            &Caelum::PointStarfield::getMag0PixelSize,
                            &Caelum::PointStarfield::setMag0PixelSize));
            td->add("min_pixel_size",
                    new AccesorPropertyDescriptor<Caelum::PointStarfield, Real, Real, Real>(
                            &Caelum::PointStarfield::getMinPixelSize,
                            &Caelum::PointStarfield::setMinPixelSize));
            td->add("max_pixel_size",
                    new AccesorPropertyDescriptor<Caelum::PointStarfield, Real, Real, Real>(
                            &Caelum::PointStarfield::getMaxPixelSize,
                            &Caelum::PointStarfield::setMaxPixelSize));
            td->add("latitude",
                    new AccesorPropertyDescriptor<Caelum::PointStarfield, Degree, Degree, Degree>(
                            &Caelum::PointStarfield::getObserverLatitude,
                            &Caelum::PointStarfield::setObserverLatitude));
            td->add("longitude",
                    new AccesorPropertyDescriptor<Caelum::PointStarfield, Degree, Degree, Degree>(
                            &Caelum::PointStarfield::getObserverLongitude,
                            &Caelum::PointStarfield::setObserverLongitude));
            td->add("observer_position_rebuild_delta",
                    new AccesorPropertyDescriptor<Caelum::PointStarfield, Degree, Degree, Degree>(
                            &Caelum::PointStarfield::getObserverPositionRebuildDelta,
                            &Caelum::PointStarfield::setObserverPositionRebuildDelta));
            PointStarfieldTypeDescriptor = td.release ();
        }

        if (!BaseSkyLightTypeDescriptor)
        {
            std::auto_ptr<DefaultTypeDescriptor> td (new DefaultTypeDescriptor ());
            td->add("ambient_multiplier",
                    new AccesorPropertyDescriptor<Caelum::BaseSkyLight, ColourValue>(
                            &Caelum::BaseSkyLight::getAmbientMultiplier,
                            &Caelum::BaseSkyLight::setAmbientMultiplier));
            td->add("specular_multiplier",
                    new AccesorPropertyDescriptor<Caelum::BaseSkyLight, ColourValue>(
                            &Caelum::BaseSkyLight::getSpecularMultiplier,
                            &Caelum::BaseSkyLight::setSpecularMultiplier));
            td->add("diffuse_multiplier",
                    new AccesorPropertyDescriptor<Caelum::BaseSkyLight, ColourValue>(
                            &Caelum::BaseSkyLight::getDiffuseMultiplier,
                            &Caelum::BaseSkyLight::setDiffuseMultiplier));
            td->add("light_colour",
                    new AccesorPropertyDescriptor<Caelum::BaseSkyLight, ColourValue>(
                            &Caelum::BaseSkyLight::getLightColour,
                            &Caelum::BaseSkyLight::setLightColour));
            td->add("body_colour",
                    new AccesorPropertyDescriptor<Caelum::BaseSkyLight, ColourValue>(
                            &Caelum::BaseSkyLight::getBodyColour,
                            &Caelum::BaseSkyLight::setBodyColour));
            td->add("auto_disable_threshold",
                    new AccesorPropertyDescriptor<Caelum::BaseSkyLight, Real, Real, Real>(
                            &Caelum::BaseSkyLight::getAutoDisableThreshold,
                            &Caelum::BaseSkyLight::setAutoDisableThreshold));
            td->add("auto_disable",
                    new AccesorPropertyDescriptor<Caelum::BaseSkyLight, bool, bool, bool>(
                            &Caelum::BaseSkyLight::getAutoDisable,
                            &Caelum::BaseSkyLight::setAutoDisable));
            BaseSkyLightTypeDescriptor = td.release ();
        }

        if (!GroundFogTypeDescriptor)
        {
            std::auto_ptr<DefaultTypeDescriptor> td (new DefaultTypeDescriptor ());
            td->add("density",
                    new AccesorPropertyDescriptor<Caelum::GroundFog, Real, Real, Real>(
                            &Caelum::GroundFog::getDensity,
                            &Caelum::GroundFog::setDensity));
            td->add("vertical_decay",
                    new AccesorPropertyDescriptor<Caelum::GroundFog, Real, Real, Real>(
                            &Caelum::GroundFog::getVerticalDecay,
                            &Caelum::GroundFog::setVerticalDecay));
            td->add("ground_level",
                    new AccesorPropertyDescriptor<Caelum::GroundFog, Real, Real, Real>(
                            &Caelum::GroundFog::getGroundLevel,
                            &Caelum::GroundFog::setGroundLevel));
            td->add("colour",
                    new AccesorPropertyDescriptor<Caelum::GroundFog, ColourValue>(
                            &Caelum::GroundFog::getColour,
                            &Caelum::GroundFog::setColour));
            GroundFogTypeDescriptor = td.release ();
        }

        if (!DepthComposerTypeDescriptor)
        {
            std::auto_ptr<DefaultTypeDescriptor> td (new DefaultTypeDescriptor ());
            td->add("debug_depth_render",
                    new AccesorPropertyDescriptor<Caelum::DepthComposer, bool, bool, bool>(
                            &Caelum::DepthComposer::getDebugDepthRender,
                            &Caelum::DepthComposer::setDebugDepthRender));

            // Legacy haze
            td->add("haze_enabled",
                    new AccesorPropertyDescriptor<Caelum::DepthComposer, bool, bool, bool>(
                            &Caelum::DepthComposer::getSkyDomeHazeEnabled,
                            &Caelum::DepthComposer::setSkyDomeHazeEnabled));
            td->add("haze_colour",
                    new AccesorPropertyDescriptor<Caelum::DepthComposer, ColourValue>(
                            &Caelum::DepthComposer::getHazeColour,
                            &Caelum::DepthComposer::setHazeColour));
            td->add("haze_sun_direction",
                    new AccesorPropertyDescriptor<Caelum::DepthComposer, Vector3>(
                            &Caelum::DepthComposer::getSunDirection,
                            &Caelum::DepthComposer::setSunDirection));

            // Ground fog
            td->add("ground_fog_enabled",
                    new AccesorPropertyDescriptor<Caelum::DepthComposer, bool, bool, bool>(
                            &Caelum::DepthComposer::getGroundFogEnabled,
                            &Caelum::DepthComposer::setGroundFogEnabled));
            td->add("ground_fog_density",
                    new AccesorPropertyDescriptor<Caelum::DepthComposer, Real, Real, Real>(
                            &Caelum::DepthComposer::getGroundFogDensity,
                            &Caelum::DepthComposer::setGroundFogDensity));
            td->add("ground_fog_vertical_decay",
                    new AccesorPropertyDescriptor<Caelum::DepthComposer, Real, Real, Real>(
                            &Caelum::DepthComposer::getGroundFogVerticalDecay,
                            &Caelum::DepthComposer::setGroundFogVerticalDecay));
            td->add("ground_fog_base_level",
                    new AccesorPropertyDescriptor<Caelum::DepthComposer, Real, Real, Real>(
                            &Caelum::DepthComposer::getGroundFogBaseLevel,
                            &Caelum::DepthComposer::setGroundFogBaseLevel));
            td->add("ground_fog_colour",
                    new AccesorPropertyDescriptor<Caelum::DepthComposer, ColourValue>(
                            &Caelum::DepthComposer::getGroundFogColour,
                            &Caelum::DepthComposer::setGroundFogColour));

            DepthComposerTypeDescriptor = td.release ();
        }

        if (!PrecipitationTypeDescriptor)
        {
            std::auto_ptr<DefaultTypeDescriptor> td (new DefaultTypeDescriptor ());

            td->add("texture",
                    new AccesorPropertyDescriptor<Caelum::PrecipitationController, String>(
                            &Caelum::PrecipitationController::getTextureName,
                            &Caelum::PrecipitationController::setTextureName));
            td->add("precipitation_colour",
                    new AccesorPropertyDescriptor<Caelum::PrecipitationController, ColourValue>(
                            &Caelum::PrecipitationController::getColour,
                            &Caelum::PrecipitationController::setColour));
            td->add("falling_speed",
                    new AccesorPropertyDescriptor<Caelum::PrecipitationController, Real, Real, Real>(
                            &Caelum::PrecipitationController::getSpeed,
                            &Caelum::PrecipitationController::setSpeed));
            td->add("wind_speed",
                    new AccesorPropertyDescriptor<Caelum::PrecipitationController, Vector3>(
                            &Caelum::PrecipitationController::getWindSpeed,
                            &Caelum::PrecipitationController::setWindSpeed));
            td->add("camera_speed_scale",
                    new AccesorPropertyDescriptor<Caelum::PrecipitationController, Vector3>(
                            &Caelum::PrecipitationController::getCameraSpeedScale,
                            &Caelum::PrecipitationController::setCameraSpeedScale));
            td->add("intensity",
                    new AccesorPropertyDescriptor<Caelum::PrecipitationController, Real, Real, Real>(
                            &Caelum::PrecipitationController::getIntensity,
                            &Caelum::PrecipitationController::setIntensity));
            td->add("auto_disable_intensity",
                    new AccesorPropertyDescriptor<Caelum::PrecipitationController, Real, Real, Real>(
                            &Caelum::PrecipitationController::getAutoDisableThreshold,
                            &Caelum::PrecipitationController::setAutoDisableThreshold));
            td->add("falling_direction",
                    new AccesorPropertyDescriptor<Caelum::PrecipitationController, Vector3>(
                            &Caelum::PrecipitationController::getFallingDirection,
                            &Caelum::PrecipitationController::setFallingDirection));

            PrecipitationTypeDescriptor = td.release ();
        }

        if (!FlatCloudLayerTypeDescriptor)
        {
            std::auto_ptr<DefaultTypeDescriptor> td (new DefaultTypeDescriptor ());

            // Height.
            td->add("height",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Real, Real, Real>(
                            &Caelum::FlatCloudLayer::getHeight,
                            &Caelum::FlatCloudLayer::setHeight));

            // Coverage parameters.
            td->add("coverage",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Real, Real, Real>(
                            &Caelum::FlatCloudLayer::getCloudCover,
                            &Caelum::FlatCloudLayer::setCloudCover));
            td->add("cloud_cover_visibility_threshold",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Real, Real, Real>(
                            &Caelum::FlatCloudLayer::getCloudCoverVisibilityThreshold,
                            &Caelum::FlatCloudLayer::setCloudCoverVisibilityThreshold));
            td->add("cloud_cover_lookup",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, String>(
                            &Caelum::FlatCloudLayer::getCloudCoverLookupFileName,
                            &Caelum::FlatCloudLayer::setCloudCoverLookup));

            // Overwritten by CaelumSystem; included for completeness.
            td->add("sun_direction",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Vector3>(
                            &Caelum::FlatCloudLayer::getSunDirection,
                            &Caelum::FlatCloudLayer::setSunDirection));
            td->add("sun_light_colour",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, ColourValue>(
                            &Caelum::FlatCloudLayer::getSunLightColour,
                            &Caelum::FlatCloudLayer::setSunLightColour));
            td->add("sun_sphere_colour",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, ColourValue>(
                            &Caelum::FlatCloudLayer::getSunSphereColour,
                            &Caelum::FlatCloudLayer::setSunSphereColour));
            td->add("fog_colour",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, ColourValue>(
                            &Caelum::FlatCloudLayer::getFogColour,
                            &Caelum::FlatCloudLayer::setFogColour));

            // Moving noise textures.
            td->add("cloud_speed",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Vector2>(
                            &Caelum::FlatCloudLayer::getCloudSpeed,
                            &Caelum::FlatCloudLayer::setCloudSpeed)); 

            // Blending time between noise textures.
            td->add("blend_time",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Real, Real, Real>(
                            &Caelum::FlatCloudLayer::getCloudBlendTime,
                            &Caelum::FlatCloudLayer::setCloudBlendTime));

            // Mesh properties
            td->add("mesh_width",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Real, Real, Real>(
                            &Caelum::FlatCloudLayer::getMeshWidth,
                            &Caelum::FlatCloudLayer::setMeshWidth));
            td->add("mesh_height",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Real, Real, Real>(
                            &Caelum::FlatCloudLayer::getMeshHeight,
                            &Caelum::FlatCloudLayer::setMeshHeight));
            td->add("mesh_width_segments",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, int, int, int>(
                            &Caelum::FlatCloudLayer::getMeshWidthSegments,
                            &Caelum::FlatCloudLayer::setMeshWidthSegments));
            td->add("mesh_height_segments",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, int, int, int>(
                            &Caelum::FlatCloudLayer::getMeshHeightSegments,
                            &Caelum::FlatCloudLayer::setMeshHeightSegments));

            // Misc hacks
            td->add("cloud_uv_factor",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Real, Real, Real>(
                            &Caelum::FlatCloudLayer::getCloudUVFactor,
                            &Caelum::FlatCloudLayer::setCloudUVFactor));
            td->add("height_red_factor",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Real, Real, Real>(
                            &Caelum::FlatCloudLayer::getHeightRedFactor,
                            &Caelum::FlatCloudLayer::setHeightRedFactor));

            // Fading
            td->add("near_fade_dist",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Real, Real, Real>(
                            &Caelum::FlatCloudLayer::getNearFadeDist,
                            &Caelum::FlatCloudLayer::setNearFadeDist));
            td->add("far_fade_dist",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Real, Real, Real>(
                            &Caelum::FlatCloudLayer::getFarFadeDist,
                            &Caelum::FlatCloudLayer::setFarFadeDist));
            td->add("fade_dist_measurement_vector",
                    new AccesorPropertyDescriptor<Caelum::FlatCloudLayer, Vector3>(
                            &Caelum::FlatCloudLayer::getFadeDistMeasurementVector,
                            &Caelum::FlatCloudLayer::setFadeDistMeasurementVector));

            FlatCloudLayerTypeDescriptor = td.release ();
        }

        if (!SkyDomeTypeDescriptor)
        {
            std::auto_ptr<DefaultTypeDescriptor> td (new DefaultTypeDescriptor ());

            // SkyDome is slightly special because most properties are write-only.

            // Reset by CaelumSystem every frame anyway
            td->add("sun_direction",
                    new AccesorPropertyDescriptor<Caelum::SkyDome, Ogre::Vector3>(
                            0, &Caelum::SkyDome::setSunDirection));
            td->add("haze_colour",
                    new AccesorPropertyDescriptor<Caelum::SkyDome, Ogre::ColourValue>(
                            0, &Caelum::SkyDome::setHazeColour));

            // Different files not supported anyway
            td->add("sky_gradients_image",
                    new AccesorPropertyDescriptor<Caelum::SkyDome, Ogre::String>(
                            0, &Caelum::SkyDome::setSkyGradientsImage));
            td->add("atmosphere_depth_image",
                    new AccesorPropertyDescriptor<Caelum::SkyDome, Ogre::String>(
                            0, &Caelum::SkyDome::setAtmosphereDepthImage));

            // This does actually make sense.
            td->add("haze_enabled",
                    new AccesorPropertyDescriptor<Caelum::SkyDome, bool, bool, bool>(
                            &Caelum::SkyDome::getHazeEnabled,
                            &Caelum::SkyDome::setHazeEnabled));

            SkyDomeTypeDescriptor = td.release ();
        }
    }
}

#endif // CAELUM_TYPE_DESCRIPTORS
