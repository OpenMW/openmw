#ifndef MWGUI_TRAININGWINDOW_H
#define MWGUI_TRAININGWINDOW_H

#include "windowbase.hpp"
#include "referenceinterface.hpp"

namespace MWGui
{

    class TrainingWindow : public WindowBase, public ReferenceInterface
    {
    public:
        TrainingWindow();

        virtual void open();

        virtual void exit();

        void startTraining(MWWorld::Ptr actor);

        void onFrame(float dt);

    protected:
        virtual void onReferenceUnavailable ();

        void onCancelButtonClicked (MyGUI::Widget* sender);
        void onTrainingSelected(MyGUI::Widget* sender);

        MyGUI::Widget* mTrainingOptions;
        MyGUI::Button* mCancelButton;
        MyGUI::TextBox* mPlayerGold;

        float mFadeTimeRemaining;
    };

}

#endif
