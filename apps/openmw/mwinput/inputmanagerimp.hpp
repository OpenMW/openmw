#ifndef MWINPUT_MWINPUTMANAGERIMP_H
#define MWINPUT_MWINPUTMANAGERIMP_H

#include "../mwgui/mode.hpp"

#include <components/settings/settings.hpp>

#include "../mwbase/inputmanager.hpp"
#include <extern/sdl4ogre/sdlinputwrapper.hpp>

namespace OEngine
{
    namespace Render
    {
        class OgreRenderer;
    }
}

namespace MWWorld
{
    class Player;
}

namespace MWBase
{
    class WindowManager;
}

namespace OMW
{
    class Engine;
}

namespace ICS
{
    class InputControlSystem;
}

namespace MyGUI
{
    class MouseButton;
}

#include <extern/oics/ICSChannelListener.h>
#include <extern/oics/ICSInputControlSystem.h>

namespace MWInput
{

    /**
    * @brief Class that handles all input and key bindings for OpenMW.
    */
    class InputManager :
            public MWBase::InputManager,
            public SFO::KeyListener,
            public SFO::MouseListener,
            public SFO::WindowListener,
            public ICS::ChannelListener,
            public ICS::DetectingBindingListener
    {
    public:
        InputManager(OEngine::Render::OgreRenderer &_ogre,
            OMW::Engine& engine,
            const std::string& userFile, bool userFileExists, bool grab);

        virtual ~InputManager();

        /// Clear all savegame-specific data
        virtual void clear();

        virtual void update(float dt, bool disableControls=false, bool disableEvents=false);

        void setPlayer (MWWorld::Player* player) { mPlayer = player; }

        virtual void changeInputMode(bool guiMode);

        virtual void processChangedSettings(const Settings::CategorySettingVector& changed);

        virtual void setDragDrop(bool dragDrop);

        virtual void toggleControlSwitch (const std::string& sw, bool value);
        virtual bool getControlSwitch (const std::string& sw);

        virtual std::string getActionDescription (int action);
        virtual std::string getActionBindingName (int action);
        virtual int getNumActions() { return A_Last; }
        virtual std::vector<int> getActionSorting ();
        virtual void enableDetectingBindingMode (int action);
        virtual void resetToDefaultBindings();

    public:
        virtual void keyPressed(const SDL_KeyboardEvent &arg );
        virtual void keyReleased( const SDL_KeyboardEvent &arg );
        virtual void textInput (const SDL_TextInputEvent &arg);

        virtual void mousePressed( const SDL_MouseButtonEvent &arg, Uint8 id );
        virtual void mouseReleased( const SDL_MouseButtonEvent &arg, Uint8 id );
        virtual void mouseMoved( const SFO::MouseMotionEvent &arg );

        virtual void windowVisibilityChange( bool visible );
        virtual void windowFocusChange( bool have_focus );
        virtual void windowResized (int x, int y);
        virtual void windowClosed ();

        virtual void channelChanged(ICS::Channel* channel, float currentValue, float previousValue);

        virtual void mouseAxisBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , ICS::InputControlSystem::NamedAxis axis, ICS::Control::ControlChangingDirection direction);

        virtual void keyBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , SDL_Keycode key, ICS::Control::ControlChangingDirection direction);

        virtual void mouseButtonBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , unsigned int button, ICS::Control::ControlChangingDirection direction);

        virtual void joystickAxisBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , int deviceId, int axis, ICS::Control::ControlChangingDirection direction);

        virtual void joystickButtonBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , int deviceId, unsigned int button, ICS::Control::ControlChangingDirection direction);

        virtual void joystickPOVBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , int deviceId, int pov,ICS:: InputControlSystem::POVAxis axis, ICS::Control::ControlChangingDirection direction);

        virtual void joystickSliderBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , int deviceId, int slider, ICS::Control::ControlChangingDirection direction);

        void clearAllBindings (ICS::Control* control);

    private:
        OEngine::Render::OgreRenderer &mOgre;
        MWWorld::Player* mPlayer;
        OMW::Engine& mEngine;

        ICS::InputControlSystem* mInputBinder;


        SFO::InputWrapper* mInputManager;

        std::string mUserFile;

        bool mDragDrop;

        bool mGrabCursor;

        bool mInvertY;

        bool mControlsDisabled;

        float mCameraSensitivity;
        float mUISensitivity;
        float mCameraYMultiplier;
        float mPreviewPOVDelay;
        float mTimeIdle;

        bool mMouseLookEnabled;
        bool mGuiCursorEnabled;

        float mOverencumberedMessageDelay;

        float mMouseX;
        float mMouseY;
        int mMouseWheel;
        bool mUserFileExists;
        bool mAlwaysRunActive;
        bool mAttemptJump;

        std::map<std::string, bool> mControlSwitch;

    private:
        void adjustMouseRegion(int width, int height);
        MyGUI::MouseButton sdlButtonToMyGUI(Uint8 button);

        void resetIdleTime();
        void updateIdleTime(float dt);

        void setPlayerControlsEnabled(bool enabled);

    private:
        void toggleMainMenu();
        void toggleSpell();
        void toggleWeapon();
        void toggleInventory();
        void toggleConsole();
        void screenshot();
        void toggleJournal();
        void activate();
        void toggleWalking();
        void toggleAutoMove();
        void rest();
        void quickLoad();
        void quickSave();

        void quickKey (int index);
        void showQuickKeysMenu();

        bool actionIsActive (int id);

        void loadKeyDefaults(bool force = false);

    private:
        enum Actions
        {
            // please add new actions at the bottom, in order to preserve the channel IDs in the key configuration files

            A_GameMenu,

            A_Unused,

            A_Screenshot,     // Take a screenshot

            A_Inventory,      // Toggle inventory screen

            A_Console,        // Toggle console screen

            A_MoveLeft,       // Move player left / right
            A_MoveRight,
            A_MoveForward,    // Forward / Backward
            A_MoveBackward,

            A_Activate,

            A_Use,        //Use weapon, spell, etc.
            A_Jump,
            A_AutoMove,   //Toggle Auto-move forward
            A_Rest,       //Rest
            A_Journal,    //Journal
            A_Weapon,     //Draw/Sheath weapon
            A_Spell,      //Ready/Unready Casting
            A_Run,        //Run when held
            A_CycleSpellLeft, //cycling through spells
            A_CycleSpellRight,
            A_CycleWeaponLeft,//Cycling through weapons
            A_CycleWeaponRight,
            A_ToggleSneak,    //Toggles Sneak
            A_AlwaysRun, //Toggle Walking/Running
            A_Sneak,

            A_QuickSave,
            A_QuickLoad,
            A_QuickMenu,
            A_ToggleWeapon,
            A_ToggleSpell,

            A_TogglePOV,

            A_QuickKey1,
            A_QuickKey2,
            A_QuickKey3,
            A_QuickKey4,
            A_QuickKey5,
            A_QuickKey6,
            A_QuickKey7,
            A_QuickKey8,
            A_QuickKey9,
            A_QuickKey10,

            A_QuickKeysMenu,

            A_ToggleHUD,

            A_Last            // Marker for the last item
        };
    };
}
#endif
