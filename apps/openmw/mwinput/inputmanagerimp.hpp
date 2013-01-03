#ifndef _MWINPUT_MWINPUTMANAGERIMP_H
#define _MWINPUT_MWINPUTMANAGERIMP_H

#include "../mwgui/mode.hpp"

#include <components/settings/settings.hpp>

#include "../mwbase/inputmanager.hpp"

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

namespace OIS
{
    class Keyboard;
    class Mouse;
    class InputManager;
}

#include <OISKeyboard.h>
#include <OISMouse.h>

#include <extern/oics/ICSChannelListener.h>
#include <extern/oics/ICSInputControlSystem.h>

namespace MWInput
{

    /**
    * @brief Class that handles all input and key bindings for OpenMW.
    */
    class InputManager : public MWBase::InputManager, public OIS::KeyListener, public OIS::MouseListener, public ICS::ChannelListener, public ICS::DetectingBindingListener
    {
    public:
        InputManager(OEngine::Render::OgreRenderer &_ogre,
            MWWorld::Player&_player,
            MWBase::WindowManager &_windows,
            bool debug,
            OMW::Engine& engine,
            const std::string& userFile, bool userFileExists);

        virtual ~InputManager();

        virtual void update(float dt, bool loading);

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
        virtual bool keyPressed( const OIS::KeyEvent &arg );
        virtual bool keyReleased( const OIS::KeyEvent &arg );

        virtual bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
        virtual bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
        virtual bool mouseMoved( const OIS::MouseEvent &arg );

        virtual void channelChanged(ICS::Channel* channel, float currentValue, float previousValue);

        virtual void mouseAxisBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , ICS::InputControlSystem::NamedAxis axis, ICS::Control::ControlChangingDirection direction);

        virtual void keyBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , OIS::KeyCode key, ICS::Control::ControlChangingDirection direction);

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
        MWWorld::Player &mPlayer;
        MWBase::WindowManager &mWindows;
        OMW::Engine& mEngine;

        ICS::InputControlSystem* mInputCtrl;

        OIS::Keyboard* mKeyboard;
        OIS::Mouse* mMouse;
        OIS::InputManager* mInputManager;

        std::string mUserFile;

        bool mDragDrop;

        bool mInvertY;

        float mCameraSensitivity;
        float mUISensitivity;
        float mCameraYMultiplier;
        float mUIYMultiplier;
        float mPreviewPOVDelay;
        float mTimeIdle;

        bool mMouseLookEnabled;
        bool mGuiCursorEnabled;

        float mMouseX;
        float mMouseY;
        int mMouseWheel;

        std::map<std::string, bool> mControlSwitch;

    private:
        void adjustMouseRegion(int width, int height);

        void resetIdleTime();
        void updateIdleTime(float dt);

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
        void exitNow();
        void rest();

        void quickKey (int index);
        void showQuickKeysMenu();

        bool actionIsActive (int id);

        void loadKeyDefaults(bool force = false);

    private:
        enum Actions
        {
            // please add new actions at the bottom, in order to preserve the channel IDs in the key configuration files

            A_GameMenu,

            A_Quit,           // Exit the program

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
            A_AlwaysRun,  //Toggle Always Run
            A_CycleSpellLeft, //cycling through spells
            A_CycleSpellRight,
            A_CycleWeaponLeft,//Cycling through weapons
            A_CycleWeaponRight,
            A_ToggleSneak,    //Toggles Sneak, add Push-Sneak later
            A_ToggleWalk, //Toggle Walking/Running
            A_Crouch,

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
