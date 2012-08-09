#ifndef MWGUI_CLASS_H
#define MWGUI_CLASS_H

#include <MyGUI.h>
#include "widgets.hpp"
#include "window_base.hpp"

/*
  This file contains the dialogs for choosing a class.
  Layout is defined by resources/mygui/openmw_chargen_class.layout.
 */

namespace MWGui
{
    using namespace MyGUI;

    class WindowManager;

    class InfoBoxDialog : public WindowBase
    {
    public:
        InfoBoxDialog(WindowManager& parWindowManager);

        typedef std::vector<std::string> ButtonList;

        void setText(const std::string &str);
        std::string getText() const;
        void setButtons(ButtonList &buttons);

        void open();
        int getChosenButton() const;

        // Events
        typedef delegates::CMultiDelegate1<int> EventHandle_Int;

        /** Event : Button was clicked.\n
            signature : void method(int index)\n
        */
        EventHandle_Int eventButtonSelected;

    protected:
        void onButtonClicked(MyGUI::WidgetPtr _sender);

    private:

        void fitToText(MyGUI::TextBox* widget);
        void layoutVertically(MyGUI::WidgetPtr widget, int margin);
        int mCurrentButton;
        MyGUI::WidgetPtr mTextBox;
        MyGUI::TextBox* mText;
        MyGUI::WidgetPtr mButtonBar;
        std::vector<MyGUI::ButtonPtr> mButtons;
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
        ClassChoiceDialog(WindowManager& parWindowManager);
    };

    class GenerateClassResultDialog : public WindowBase
    {
    public:
        GenerateClassResultDialog(WindowManager& parWindowManager);

        std::string getClassId() const;
        void setClassId(const std::string &classId);

        void open();

        // Events
        typedef delegates::CMultiDelegate0 EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

    protected:
        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        MyGUI::ImageBox* mClassImage;
        MyGUI::TextBox*  mClassName;

        std::string mCurrentClassId;
    };

    class PickClassDialog : public WindowBase
    {
    public:
        PickClassDialog(WindowManager& parWindowManager);

        const std::string &getClassId() const { return mCurrentClassId; }
        void setClassId(const std::string &classId);

        void setNextButtonShow(bool shown);
        void open();

        // Events
        typedef delegates::CMultiDelegate0 EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

    protected:
        void onSelectClass(MyGUI::ListBox* _sender, size_t _index);

        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        void updateClasses();
        void updateStats();

        MyGUI::ImageBox* mClassImage;
        MyGUI::ListBox*  mClassList;
        MyGUI::TextBox*  mSpecializationName;
        Widgets::MWAttributePtr mFavoriteAttribute[2];
        Widgets::MWSkillPtr   mMajorSkill[5];
        Widgets::MWSkillPtr   mMinorSkill[5];

        std::string mCurrentClassId;
    };

    class SelectSpecializationDialog : public WindowBase
    {
    public:
        SelectSpecializationDialog(WindowManager& parWindowManager);
        ~SelectSpecializationDialog();

        ESM::Class::Specialization getSpecializationId() const { return mSpecializationId; }

        // Events
        typedef delegates::CMultiDelegate0 EventHandle_Void;

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
        MyGUI::TextBox *mSpecialization0, *mSpecialization1, *mSpecialization2;

        ESM::Class::Specialization mSpecializationId;
    };

    class SelectAttributeDialog : public WindowBase
    {
    public:
        SelectAttributeDialog(WindowManager& parWindowManager);
        ~SelectAttributeDialog();

        ESM::Attribute::AttributeID getAttributeId() const { return mAttributeId; }
        Widgets::MWAttributePtr getAffectedWidget() const { return mAffectedWidget; }
        void setAffectedWidget(Widgets::MWAttributePtr widget) { mAffectedWidget = widget; }

        // Events
        typedef delegates::CMultiDelegate0 EventHandle_Void;

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
        Widgets::MWAttributePtr mAffectedWidget;

        ESM::Attribute::AttributeID mAttributeId;
    };

    class SelectSkillDialog : public WindowBase
    {
    public:
        SelectSkillDialog(WindowManager& parWindowManager);
        ~SelectSkillDialog();

        ESM::Skill::SkillEnum getSkillId() const { return mSkillId; }
        Widgets::MWSkillPtr getAffectedWidget() const { return mAffectedWidget; }
        void setAffectedWidget(Widgets::MWSkillPtr widget) { mAffectedWidget = widget; }

        // Events
        typedef delegates::CMultiDelegate0 EventHandle_Void;

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
        Widgets::MWSkillPtr mCombatSkill[9];
        Widgets::MWSkillPtr mMagicSkill[9];
        Widgets::MWSkillPtr mStealthSkill[9];
        Widgets::MWSkillPtr mAffectedWidget;

        ESM::Skill::SkillEnum mSkillId;
    };

    class DescriptionDialog : public WindowBase
    {
    public:
        DescriptionDialog(WindowManager& parWindowManager);
        ~DescriptionDialog();

        std::string getTextInput() const { return mTextEdit ? mTextEdit->getOnlyText() : ""; }
        void setTextInput(const std::string &text) { if (mTextEdit) mTextEdit->setOnlyText(text); }

    protected:
        void onOkClicked(MyGUI::Widget* _sender);

    private:
        MyGUI::EditPtr mTextEdit;
    };

    class CreateClassDialog : public WindowBase
    {
    public:
        CreateClassDialog(WindowManager& parWindowManager);
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
        typedef delegates::CMultiDelegate0 EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

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
        void onDescriptionEntered(WindowBase* parWindow);
        void onDialogCancel();

        void setSpecialization(int id);

        void update();

    private:
        MyGUI::EditPtr                   mEditName;
        MyGUI::TextBox*                  mSpecializationName;
        Widgets::MWAttributePtr          mFavoriteAttribute0, mFavoriteAttribute1;
        Widgets::MWSkillPtr              mMajorSkill[5];
        Widgets::MWSkillPtr              mMinorSkill[5];
        std::vector<Widgets::MWSkillPtr> mSkills;
        std::string                      mDescription;

        SelectSpecializationDialog       *mSpecDialog;
        SelectAttributeDialog            *mAttribDialog;
        SelectSkillDialog                *mSkillDialog;
        DescriptionDialog                *mDescDialog;

        ESM::Class::Specialization       mSpecializationId;
    };
}
#endif
