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
#include "settingvalue.hpp"

#include <cassert>
#include <memory>
#include <string_view>

namespace Settings
{
    struct Values : WithIndex
    {
        using WithIndex::WithIndex;

        CameraCategory mCamera{ mIndex };
        CellsCategory mCells{ mIndex };
        TerrainCategory mTerrain{ mIndex };
        FogCategory mFog{ mIndex };
        MapCategory mMap{ mIndex };
        GUICategory mGUI{ mIndex };
        HUDCategory mHUD{ mIndex };
        GameCategory mGame{ mIndex };
        GeneralCategory mGeneral{ mIndex };
        ShadersCategory mShaders{ mIndex };
        InputCategory mInput{ mIndex };
        SavesCategory mSaves{ mIndex };
        SoundCategory mSound{ mIndex };
        VideoCategory mVideo{ mIndex };
        WaterCategory mWater{ mIndex };
        WindowsCategory mWindows{ mIndex };
        NavigatorCategory mNavigator{ mIndex };
        ShadowsCategory mShadows{ mIndex };
        PhysicsCategory mPhysics{ mIndex };
        ModelsCategory mModels{ mIndex };
        GroundcoverCategory mGroundcover{ mIndex };
        LuaCategory mLua{ mIndex };
        StereoCategory mStereo{ mIndex };
        StereoViewCategory mStereoView{ mIndex };
        PostProcessingCategory mPostProcessing{ mIndex };
    };

    class StaticValues
    {
    public:
        static void initDefaults();

        static void init();

        static void clear();

    private:
        static std::unique_ptr<Index> sIndex;
        static std::unique_ptr<Values> sDefaultValues;
        static std::unique_ptr<Values> sValues;

        friend Values& values();

        template <class T>
        friend SettingValue<T>* find(std::string_view category, std::string_view name);

        template <class T>
        friend SettingValue<T>& get(std::string_view category, std::string_view name);
    };

    inline Values& values()
    {
        assert(StaticValues::sValues != nullptr);
        return *StaticValues::sValues;
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

    template <class T>
    SettingValue<T>* find(std::string_view category, std::string_view name)
    {
        return StaticValues::sIndex->find<T>(category, name);
    }

    template <class T>
    SettingValue<T>& get(std::string_view category, std::string_view name)
    {
        return StaticValues::sIndex->get<T>(category, name);
    }
}

#endif
