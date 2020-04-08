#ifndef MWINPUT_ACTIONMANAGER_H
#define MWINPUT_ACTIONMANAGER_H

#include <osg/ref_ptr>
#include <osgViewer/ViewerEventHandlers>

namespace ICS
{
    class InputControlSystem;
}

namespace osgViewer
{
    class Viewer;
    class ScreenCaptureHandler;
}

namespace MWInput
{
    class ActionManager
    {
    public:

        ActionManager(ICS::InputControlSystem* inputBinder,
            osgViewer::ScreenCaptureHandler::CaptureOperation* screenCaptureOperation,
            osg::ref_ptr<osgViewer::Viewer> viewer,
            osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler);

        void clear();

        void executeAction(int action);

        bool checkAllowedToUseItems() const;

        void toggleMainMenu();
        void toggleSpell();
        void toggleWeapon();
        void toggleInventory();
        void toggleConsole();
        void screenshot();
        void toggleJournal();
        void activate();
        void toggleWalking();
        void toggleSneaking();
        void toggleAutoMove();
        void rest();
        void quickLoad();
        void quickSave();

        void quickKey (int index);
        void showQuickKeysMenu();

        bool isAlwaysRunActive() const { return mAlwaysRunActive; };
        bool isSneaking() const { return mSneaking; };

    private:
        void handleGuiArrowKey(int action);

        ICS::InputControlSystem* mInputBinder;
        osg::ref_ptr<osgViewer::Viewer> mViewer;
        osg::ref_ptr<osgViewer::ScreenCaptureHandler> mScreenCaptureHandler;
        osgViewer::ScreenCaptureHandler::CaptureOperation* mScreenCaptureOperation;

        bool mAlwaysRunActive;
        bool mSneaking;
    };
}
#endif
