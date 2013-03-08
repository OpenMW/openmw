#ifndef MWGUI_REVIEW_H
#define MWGUI_REVIEW_H

#include "window_base.hpp"
#include "../mwmechanics/stat.hpp"
#include "widgets.hpp"

namespace MWGui
{
    class WindowManager;
}

/*
This file contains the dialog for reviewing the generated character.
Layout is defined by resources/mygui/openmw_chargen_review.layout.
*/

namespace MWGui
{
    class ReviewDialog : public WindowModal
    {
    public:
        enum Dialogs {
            NAME_DIALOG,
            RACE_DIALOG,
            CLASS_DIALOG,
            BIRTHSIGN_DIALOG
        };
        typedef std::vector<int> SkillList;

        ReviewDialog(MWBase::WindowManager& parWindowManager);

        void setPlayerName(const std::string &name);
        void setRace(const std::string &raceId);
        void setClass(const ESM::Class& class_);
        void setBirthSign (const std::string &signId);

        void setHealth(const MWMechanics::DynamicStat<float>& value);
        void setMagicka(const MWMechanics::DynamicStat<float>& value);
        void setFatigue(const MWMechanics::DynamicStat<float>& value);

        void setAttribute(ESM::Attribute::AttributeID attributeId, const MWMechanics::Stat<int>& value);

        void configureSkills(const SkillList& major, const SkillList& minor);
        void setSkillValue(ESM::Skill::SkillEnum skillId, const MWMechanics::Stat<float>& value);

        virtual void open();

        // Events
        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;
        typedef MyGUI::delegates::CMultiDelegate1<int> EventHandle_Int;

        /** Event : Back button clicked.\n
        signature : void method()\n
        */
        EventHandle_Void eventBack;

        EventHandle_Int eventActivateDialog;

    protected:
        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

        void onNameClicked(MyGUI::Widget* _sender);
        void onRaceClicked(MyGUI::Widget* _sender);
        void onClassClicked(MyGUI::Widget* _sender);
        void onBirthSignClicked(MyGUI::Widget* _sender);

        void onMouseWheel(MyGUI::Widget* _sender, int _rel);

    private:
        void addSkills(const SkillList &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
        void addSeparator(MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
        void addGroup(const std::string &label, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
        MyGUI::TextBox* addValueItem(const std::string& text, const std::string &value, const std::string& state, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
        void addItem(const std::string& text, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
        void updateSkillArea();

        static const int sLineHeight;

        MyGUI::TextBox *mNameWidget, *mRaceWidget, *mClassWidget, *mBirthSignWidget;
        MyGUI::ScrollView* mSkillView;
        int mLastPos, mClientHeight;

        Widgets::MWDynamicStatPtr mHealth, mMagicka, mFatigue;

        std::map<int, Widgets::MWAttributePtr> mAttributeWidgets;

        SkillList mMajorSkills, mMinorSkills, mMiscSkills;
        std::map<int, MWMechanics::Stat<float> > mSkillValues;
        std::map<int, MyGUI::TextBox*> mSkillWidgetMap;
        std::string mName, mRaceId, mBirthSignId;
        ESM::Class mKlass;
        std::vector<MyGUI::Widget*> mSkillWidgets; //< Skills and other information
    };
}
#endif
