#ifndef MWGUI_TRAININGWINDOW_H
#define MWGUI_TRAININGWINDOW_H

#include "windowbase.hpp"
#include "referenceinterface.hpp"
#include "timeadvancer.hpp"
#include "waitdialog.hpp"

namespace MWGui
{

    class TrainingWindow : public WindowBase, public ReferenceInterface
    {
    public:
        TrainingWindow();

        virtual void onOpen();

        bool exit() { return false; }

        void setPtr(const MWWorld::Ptr& actor);

        void onFrame(float dt);

        WindowBase* getProgressBar() { return &mProgressBar; }

        void clear() { resetReference(); }

    protected:
        virtual void onReferenceUnavailable ();

        void onCancelButtonClicked (MyGUI::Widget* sender);
        void onTrainingSelected(MyGUI::Widget* sender);

        void onTrainingProgressChanged(int cur, int total);
        void onTrainingFinished();

        MyGUI::Widget* mTrainingOptions;
        MyGUI::Button* mCancelButton;
        MyGUI::TextBox* mPlayerGold;

        WaitDialogProgressBar mProgressBar;
        TimeAdvancer mTimeAdvancer;
    };

}

#endif
