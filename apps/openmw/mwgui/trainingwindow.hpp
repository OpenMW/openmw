#ifndef MWGUI_TRAININGWINDOW_H
#define MWGUI_TRAININGWINDOW_H

#include "windowbase.hpp"
#include "referenceinterface.hpp"
#include "timeadvancer.hpp"
#include "waitdialog.hpp"

namespace MWMechanics
{
    class NpcStats;
}

namespace MWGui
{

    class TrainingWindow : public WindowBase, public ReferenceInterface
    {
    public:
        TrainingWindow();

        virtual void onOpen();

        bool exit();

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

        // Retrieve the base skill value if the setting 'training skills based on base skill' is set;
        // otherwise returns the modified skill
        float getSkillForTraining(const MWMechanics::NpcStats& stats, int skillId) const;

        MyGUI::Widget* mTrainingOptions;
        MyGUI::Button* mCancelButton;
        MyGUI::TextBox* mPlayerGold;

        WaitDialogProgressBar mProgressBar;
        TimeAdvancer mTimeAdvancer;
        bool mTrainingSkillBasedOnBaseSkill;    //corresponds to the setting 'training skills based on base skill'
    };

}

#endif
