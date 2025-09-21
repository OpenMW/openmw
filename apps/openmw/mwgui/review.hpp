#ifndef MWGUI_REVIEW_H
#define MWGUI_REVIEW_H

#include "widgets.hpp"
#include "windowbase.hpp"
#include <components/esm/refid.hpp>
#include <components/esm3/loadclas.hpp>

namespace ESM
{
    struct Spell;
}

namespace MWGui
{
    class ReviewDialog : public WindowModal
    {
    public:
        enum Dialogs
        {
            NAME_DIALOG,
            RACE_DIALOG,
            CLASS_DIALOG,
            BIRTHSIGN_DIALOG
        };

        ReviewDialog();

        bool exit() override { return false; }

        void setPlayerName(const std::string& name);
        void setRace(const ESM::RefId& raceId);
        void setClass(const ESM::Class& playerClass);
        void setBirthSign(const ESM::RefId& signId);

        void setHealth(const MWMechanics::DynamicStat<float>& value);
        void setMagicka(const MWMechanics::DynamicStat<float>& value);
        void setFatigue(const MWMechanics::DynamicStat<float>& value);

        void setAttribute(ESM::RefId attributeId, const MWMechanics::AttributeValue& value);

        void configureSkills(const std::vector<ESM::RefId>& major, const std::vector<ESM::RefId>& minor);
        void setSkillValue(ESM::RefId id, const MWMechanics::SkillValue& value);

        void onOpen() override;

        void onFrame(float duration) override;

        // Events
        typedef MyGUI::delegates::MultiDelegate<> EventHandle_Void;
        typedef MyGUI::delegates::MultiDelegate<int> EventHandle_Int;

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
        void onOkClicked(MyGUI::Widget* sender);
        void onBackClicked(MyGUI::Widget* sender);

        void onNameClicked(MyGUI::Widget* sender);
        void onRaceClicked(MyGUI::Widget* sender);
        void onClassClicked(MyGUI::Widget* sender);
        void onBirthSignClicked(MyGUI::Widget* sender);

        void onMouseWheel(MyGUI::Widget* sender, int rel);
        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;

    private:
        void addSkills(const std::vector<ESM::RefId>& skills, const std::string& titleId,
            const std::string& titleDefault, MyGUI::IntCoord& coord1, MyGUI::IntCoord& coord2);
        void addSeparator(MyGUI::IntCoord& coord1, MyGUI::IntCoord& coord2);
        void addGroup(std::string_view label, MyGUI::IntCoord& coord1, MyGUI::IntCoord& coord2);
        MyGUI::TextBox* addValueItem(std::string_view text, const std::string& value, const std::string& state,
            MyGUI::IntCoord& coord1, MyGUI::IntCoord& coord2);
        void addItem(const std::string& text, MyGUI::IntCoord& coord1, MyGUI::IntCoord& coord2);
        void addItem(const ESM::Spell* spell, MyGUI::IntCoord& coord1, MyGUI::IntCoord& coord2);
        void updateSkillArea();

        MyGUI::TextBox *mNameWidget, *mRaceWidget, *mClassWidget, *mBirthSignWidget;
        MyGUI::ScrollView* mSkillView;

        Widgets::MWDynamicStatPtr mHealth, mMagicka, mFatigue;

        std::map<ESM::RefId, Widgets::MWAttributePtr> mAttributeWidgets;

        std::vector<ESM::RefId> mMajorSkills, mMinorSkills, mMiscSkills;
        std::map<ESM::RefId, MWMechanics::SkillValue> mSkillValues;
        std::map<ESM::RefId, MyGUI::TextBox*> mSkillWidgetMap;
        ESM::RefId mRaceId, mBirthSignId;
        std::string mName;
        ESM::Class mClass;
        std::vector<MyGUI::Widget*> mSkillWidgets; //< Skills and other information

        bool mUpdateSkillArea;

        // 0 = Name, 1 = Race, 2 = Class, 3 = BirthSign, 4 = Back, 5 = OK
        std::vector<MyGUI::Button*> mButtons;
        size_t mControllerFocus = 0;
    };
}
#endif
