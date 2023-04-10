#ifndef OPENMW_COMPONENTS_SETTINGS_VALUES_H
#define OPENMW_COMPONENTS_SETTINGS_VALUES_H

#include "categories/camera.hpp"
#include "categories/cells.hpp"
#include "categories/fog.hpp"
#include "categories/game.hpp"
#include "categories/general.hpp"
#include "categories/groundcover.hpp"
#include "categories/gui.hpp"
#include "categories/hud.hpp"
#include "categories/input.hpp"
#include "categories/lua.hpp"
#include "categories/map.hpp"
#include "categories/models.hpp"
#include "categories/navigator.hpp"
#include "categories/physics.hpp"
#include "categories/postprocessing.hpp"
#include "categories/saves.hpp"
#include "categories/shaders.hpp"
#include "categories/shadows.hpp"
#include "categories/sound.hpp"
#include "categories/stereo.hpp"
#include "categories/stereoview.hpp"
#include "categories/terrain.hpp"
#include "categories/video.hpp"
#include "categories/water.hpp"
#include "categories/windows.hpp"

namespace Settings
{
    class Values
    {
    public:
        CameraCategory mCamera;
        CellsCategory mCells;
        TerrainCategory mTerrain;
        FogCategory mFog;
        MapCategory mMap;
        GUICategory mGUI;
        HUDCategory mHUD;
        GameCategory mGame;
        GeneralCategory mGeneral;
        ShadersCategory mShaders;
        InputCategory mInput;
        SavesCategory mSaves;
        SoundCategory mSound;
        VideoCategory mVideo;
        WaterCategory mWater;
        WindowsCategory mWindows;
        NavigatorCategory mNavigator;
        ShadowsCategory mShadows;
        PhysicsCategory mPhysics;
        ModelsCategory mModels;
        GroundcoverCategory mGroundcover;
        LuaCategory mLua;
        StereoCategory mStereo;
        StereoViewCategory mStereoView;
        PostProcessingCategory mPostProcessing;

        static void init();

    private:
        static Values* sValues;

        friend Values& values();
    };

    inline Values& values()
    {
        return *Values::sValues;
    }

    inline CameraCategory& camera()
    {
        return values().mCamera;
    }

    inline CellsCategory& cells()
    {
        return values().mCells;
    }

    inline TerrainCategory& terrain()
    {
        return values().mTerrain;
    }

    inline FogCategory& fog()
    {
        return values().mFog;
    }

    inline MapCategory& map()
    {
        return values().mMap;
    }

    inline GUICategory& gui()
    {
        return values().mGUI;
    }

    inline HUDCategory& hud()
    {
        return values().mHUD;
    }

    inline GameCategory& game()
    {
        return values().mGame;
    }

    inline GeneralCategory& general()
    {
        return values().mGeneral;
    }

    inline ShadersCategory& shaders()
    {
        return values().mShaders;
    }

    inline InputCategory& input()
    {
        return values().mInput;
    }

    inline SavesCategory& saves()
    {
        return values().mSaves;
    }

    inline SoundCategory& sound()
    {
        return values().mSound;
    }

    inline VideoCategory& video()
    {
        return values().mVideo;
    }

    inline WaterCategory& water()
    {
        return values().mWater;
    }

    inline WindowsCategory& windows()
    {
        return values().mWindows;
    }

    inline NavigatorCategory& navigator()
    {
        return values().mNavigator;
    }

    inline ShadowsCategory& shadows()
    {
        return values().mShadows;
    }

    inline PhysicsCategory& physics()
    {
        return values().mPhysics;
    }

    inline ModelsCategory& models()
    {
        return values().mModels;
    }

    inline GroundcoverCategory& groundcover()
    {
        return values().mGroundcover;
    }

    inline LuaCategory& lua()
    {
        return values().mLua;
    }

    inline StereoCategory& stereo()
    {
        return values().mStereo;
    }

    inline StereoViewCategory& stereoView()
    {
        return values().mStereoView;
    }

    inline PostProcessingCategory& postProcessing()
    {
        return values().mPostProcessing;
    }

}

#endif
