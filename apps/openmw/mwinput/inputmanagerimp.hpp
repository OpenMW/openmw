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

#include <OIS/OISKeyboard.h>
#include <OIS/OISMouse.h>

#include <extern/oics/ICSChannelListener.h>

namespace MWInput
{

    /**
    * @brief Class that handles all input and key bindings for OpenMW.
    */
    class InputManager : public MWBase::InputManager, public OIS::KeyListener, public OIS::MouseListener, public ICS::ChannelListener
    {
    public:
        InputManager(OEngine::Render::OgreRenderer &_ogre,
            MWWorld::Player&_player,
            MWBase::WindowManager &_windows,
            bool debug,
            OMW::Engine& engine,
            const std::string& userFile);

        virtual ~InputManager();

        virtual void update(float dt);

        virtual void changeInputMode(bool guiMode);

        virtual void processChangedSettings(const Settings::CategorySettingVector& changed);

        virtual void setDragDrop(bool dragDrop);

        virtual void toggleControlSwitch (const std::string& sw, bool value);


    public:
        virtual bool keyPressed( const OIS::KeyEvent &arg );
        virtual bool keyReleased( const OIS::KeyEvent &arg );

        virtual bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
        virtual bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
        virtual bool mouseMoved( const OIS::MouseEvent &arg );

        virtual void channelChanged(ICS::Channel* channel, float currentValue, float previousValue);

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

        bool mMouseLookEnabled;
        bool mGuiCursorEnabled;

        int mMouseX;
        int mMouseY;

        std::map<std::string, bool> mControlSwitch;


    private:
        void adjustMouseRegion(int width, int height);

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

        bool actionIsActive (int id);

        void loadKeyDefaults();

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

            A_LAST            // Marker for the last item
        };


    };
}
#endif
