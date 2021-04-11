#ifndef MWGUI_REVIEW_H
#define MWGUI_REVIEW_H

#include <components/esm/attr.hpp>
#include <components/esm/loadclas.hpp>
#include "windowbase.hpp"
#include "widgets.hpp"

namespace ESM
{
    struct Spell;
}

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

        ReviewDialog();

        bool exit() override { return false; }

        void setPlayerName(const std::string &name);
        void setRace(const std::string &raceId);
        void setClass(const ESM::Class& class_);
        void setBirthSign (const std::string &signId);

        void setHealth(const MWMechanics::DynamicStat<float>& value);
        void setMagicka(const MWMechanics::DynamicStat<float>& value);
        void setFatigue(const MWMechanics::DynamicStat<float>& value);

        void setAttribute(ESM::Attribute::AttributeID attributeId, const MWMechanics::AttributeValue& value);

        void configureSkills(const SkillList& major, const SkillList& minor);
        void setSkillValue(ESM::Skill::SkillEnum skillId, const MWMechanics::SkillValue& value);

        void onOpen() override;

        void onFrame(float duration) override;

        // Events
        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;
        typedef MyGUI::delegates::CMultiDelegate1<int> EventHandle_Int;

        /** Event : Back button clicked.\n
        signature : void method()\n
        */
        EventHandle_Void eventBack;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_WindowBase eventDone;

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
        void addItem(const ESM::Spell* spell, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
        void updateSkillArea();

        MyGUI::TextBox *mNameWidget, *mRaceWidget, *mClassWidget, *mBirthSignWidget;
        MyGUI::ScrollView* mSkillView;

        Widgets::MWDynamicStatPtr mHealth, mMagicka, mFatigue;

        std::map<int, Widgets::MWAttributePtr> mAttributeWidgets;

        SkillList mMajorSkills, mMinorSkills, mMiscSkills;
        std::map<int, MWMechanics::SkillValue > mSkillValues;
        std::map<int, MyGUI::TextBox*> mSkillWidgetMap;
        std::string mName, mRaceId, mBirthSignId;
        ESM::Class mKlass;
        std::vector<MyGUI::Widget*> mSkillWidgets; //< Skills and other information

        bool mUpdateSkillArea;
    };
}
#endif
