#ifndef MWGUI_CLASS_H
#define MWGUI_CLASS_H

#include <components/esm_store/store.hpp>

#include <openengine/gui/layout.hpp>

#include <MyGUI.h>

#include "widgets.hpp"
#include "window_base.hpp"

namespace MWWorld
{
    class Environment;
}

/*
  This file contains the dialogs for choosing a class.
  Layout is defined by resources/mygui/openmw_chargen_class_layout.xml.
 */

namespace MWGui
{
    using namespace MyGUI;

    class InfoBoxDialog : public WindowBase
    {
    public:
        InfoBoxDialog(MWWorld::Environment& environment);

        typedef std::vector<std::string> ButtonList;

        void setText(const std::string &str);
        std::string getText() const;
        void setButtons(ButtonList &buttons);

        void open();
        int getChosenButton() const;

        // Events
        typedef delegates::CDelegate2<MyGUI::WidgetPtr, int> EventHandle_WidgetInt;

        /** Event : Button was clicked.\n
            signature : void method(MyGUI::WidgetPtr widget, int index)\n
        */
        EventHandle_WidgetInt eventButtonSelected;

    protected:
        void onButtonClicked(MyGUI::WidgetPtr _sender);

    private:

        void fitToText(MyGUI::StaticTextPtr widget);
        void layoutVertically(MyGUI::WidgetPtr widget, int margin);
        int currentButton;
        MyGUI::WidgetPtr textBox;
        MyGUI::StaticTextPtr text;
        MyGUI::WidgetPtr buttonBar;
        std::vector<MyGUI::ButtonPtr> buttons;
    };

    // Lets the player choose between 3 ways of creating a class
    class ClassChoiceDialog : public InfoBoxDialog
    {
    public:
        // Corresponds to the buttons that can be clicked
        enum ClassChoice
        {
            Class_Generate = 0,
            Class_Pick = 1,
            Class_Create = 2,
            Class_Back = 3
        };
        ClassChoiceDialog(MWWorld::Environment& environment);
    };

    class GenerateClassResultDialog : public WindowBase
    {
    public:
        GenerateClassResultDialog(MWWorld::Environment& environment);

        std::string getClassId() const;
        void setClassId(const std::string &classId);

        void open();

        // Events
        typedef delegates::CDelegate0 EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventDone;

    protected:
        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        MyGUI::StaticImagePtr classImage;
        MyGUI::StaticTextPtr  className;

        std::string currentClassId;
    };

    class PickClassDialog : public WindowBase
    {
    public:
        PickClassDialog(MWWorld::Environment& environment);

        const std::string &getClassId() const { return currentClassId; }
        void setClassId(const std::string &classId);

        void setNextButtonShow(bool shown);
        void open();

        // Events
        typedef delegates::CDelegate0 EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventDone;

    protected:
        void onSelectClass(MyGUI::List* _sender, size_t _index);

        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        void updateClasses();
        void updateStats();

        MyGUI::StaticImagePtr classImage;
        MyGUI::ListPtr        classList;
        MyGUI::StaticTextPtr  specializationName;
        Widgets::MWAttributePtr favoriteAttribute[2];
        Widgets::MWSkillPtr   majorSkill[5];
        Widgets::MWSkillPtr   minorSkill[5];

        std::string currentClassId;
    };

    class SelectSpecializationDialog : public WindowBase
    {
    public:
        SelectSpecializationDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize);

        ESM::Class::Specialization getSpecializationId() const { return specializationId; }

        // Events
        typedef delegates::CDelegate0 EventHandle_Void;

        /** Event : Cancel button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventCancel;

        /** Event : Dialog finished, specialization selected.\n
            signature : void method()\n
        */
        EventHandle_Void eventItemSelected;

    protected:
        void onSpecializationClicked(MyGUI::Widget* _sender);
        void onCancelClicked(MyGUI::Widget* _sender);

    private:
        MyGUI::WidgetPtr      specialization0, specialization1, specialization2;

        ESM::Class::Specialization specializationId;
    };

    class SelectAttributeDialog : public WindowBase
    {
    public:
        SelectAttributeDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize);

        ESM::Attribute::AttributeID getAttributeId() const { return attributeId; }
        Widgets::MWAttributePtr getAffectedWidget() const { return affectedWidget; }
        void setAffectedWidget(Widgets::MWAttributePtr widget) { affectedWidget = widget; }

        // Events
        typedef delegates::CDelegate0 EventHandle_Void;

        /** Event : Cancel button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventCancel;

        /** Event : Dialog finished, attribute selected.\n
            signature : void method()\n
        */
        EventHandle_Void eventItemSelected;

    protected:
        void onAttributeClicked(Widgets::MWAttributePtr _sender);
        void onCancelClicked(MyGUI::Widget* _sender);

    private:
        Widgets::MWAttributePtr affectedWidget;

        ESM::Attribute::AttributeID attributeId;
    };

    class SelectSkillDialog : public WindowBase
    {
    public:
        SelectSkillDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize);

        ESM::Skill::SkillEnum getSkillId() const { return skillId; }
        Widgets::MWSkillPtr getAffectedWidget() const { return affectedWidget; }
        void setAffectedWidget(Widgets::MWSkillPtr widget) { affectedWidget = widget; }

        // Events
        typedef delegates::CDelegate0 EventHandle_Void;

        /** Event : Cancel button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventCancel;

        /** Event : Dialog finished, skill selected.\n
            signature : void method()\n
        */
        EventHandle_Void eventItemSelected;

    protected:
        void onSkillClicked(Widgets::MWSkillPtr _sender);
        void onCancelClicked(MyGUI::Widget* _sender);

    private:
        Widgets::MWSkillPtr combatSkill[9];
        Widgets::MWSkillPtr magicSkill[9];
        Widgets::MWSkillPtr stealthSkill[9];
        Widgets::MWSkillPtr affectedWidget;

        ESM::Skill::SkillEnum skillId;
    };

    class DescriptionDialog : public WindowBase
    {
    public:
        DescriptionDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize);

        std::string getTextInput() const { return textEdit ? textEdit->getOnlyText() : ""; }
        void setTextInput(const std::string &text) { if (textEdit) textEdit->setOnlyText(text); }

        // Events
        typedef delegates::CDelegate0 EventHandle_Void;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventDone;

    protected:
        void onOkClicked(MyGUI::Widget* _sender);

    private:
        MyGUI::EditPtr textEdit;
    };

    class CreateClassDialog : public WindowBase
    {
    public:
        CreateClassDialog(MWWorld::Environment& environment);
        virtual ~CreateClassDialog();

        std::string getName() const;
        std::string getDescription() const;
        ESM::Class::Specialization getSpecializationId() const;
        std::vector<int> getFavoriteAttributes() const;
        std::vector<ESM::Skill::SkillEnum> getMajorSkills() const;
        std::vector<ESM::Skill::SkillEnum> getMinorSkills() const;

        void setNextButtonShow(bool shown);
        void open();

        // Events
        typedef delegates::CDelegate0 EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventDone;

    protected:
        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

        void onSpecializationClicked(MyGUI::WidgetPtr _sender);
        void onSpecializationSelected();
        void onAttributeClicked(Widgets::MWAttributePtr _sender);
        void onAttributeSelected();
        void onSkillClicked(Widgets::MWSkillPtr _sender);
        void onSkillSelected();
        void onDescriptionClicked(MyGUI::Widget* _sender);
        void onDescriptionEntered();
        void onDialogCancel();

    private:
        MyGUI::EditPtr          editName;
        MyGUI::WidgetPtr        specializationName;
        Widgets::MWAttributePtr favoriteAttribute0, favoriteAttribute1;
        Widgets::MWSkillPtr     majorSkill[5];
        Widgets::MWSkillPtr     minorSkill[5];
        std::vector<Widgets::MWSkillPtr> skills;
        std::string             description;

        SelectSpecializationDialog *specDialog;
        SelectAttributeDialog *attribDialog;
        SelectSkillDialog *skillDialog;
        DescriptionDialog *descDialog;

        ESM::Class::Specialization specializationId;
    };
}
#endif
