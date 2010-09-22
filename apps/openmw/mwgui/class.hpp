#ifndef MWGUI_CLASS_H
#define MWGUI_CLASS_H

#include <components/esm_store/store.hpp>

#include <openengine/gui/layout.hpp>

#include <MyGUI.h>

#include "widgets.hpp"

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

    class PickClassDialog : public OEngine::GUI::Layout
    {
    public:
        PickClassDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize);

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

        MWWorld::Environment& environment;

        MyGUI::StaticImagePtr classImage;
        MyGUI::ListPtr        classList;
        MyGUI::StaticTextPtr  specializationName;
        Widgets::MWAttributePtr favoriteAttribute0, favoriteAttribute1;
        Widgets::MWSkillPtr   majorSkill0, majorSkill1, majorSkill2, majorSkill3, majorSkill4;
        Widgets::MWSkillPtr   minorSkill0, minorSkill1, minorSkill2, minorSkill3, minorSkill4;

        std::string currentClassId;
    };

    class CreateClassDialog : public OEngine::GUI::Layout
    {
    public:
        CreateClassDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize);

//        const std::string &getClassId() const { return currentClassId; }
//        void setClassId(const std::string &classId);

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
        void onDescriptionClicked(MyGUI::Widget* _sender);
        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        void updateStats();

        MWWorld::Environment& environment;

        MyGUI::EditPtr        editName;
        MyGUI::StaticTextPtr  specializationName;
        Widgets::MWAttributePtr favoriteAttribute0, favoriteAttribute1;
        Widgets::MWSkillPtr   majorSkill0, majorSkill1, majorSkill2, majorSkill3, majorSkill4;
        Widgets::MWSkillPtr   minorSkill0, minorSkill1, minorSkill2, minorSkill3, minorSkill4;

        std::string currentClassId;
    };
}
#endif
