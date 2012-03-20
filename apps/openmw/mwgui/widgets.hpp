#ifndef MWGUI_WIDGETS_H
#define MWGUI_WIDGETS_H

#include <components/esm_store/store.hpp>

#include <MyGUI.h>

#include "../mwmechanics/stat.hpp"

/*
  This file contains various custom widgets used in OpenMW.
 */

namespace MWGui
{
    using namespace MyGUI;
    class WindowManager;

    namespace Widgets
    {
        void fixTexturePath(std::string &path);

        class MYGUI_EXPORT MWSkill : public Widget
        {
            MYGUI_RTTI_DERIVED( MWSkill );
        public:
            MWSkill();

            typedef MWMechanics::Stat<float> SkillValue;

            void setWindowManager(WindowManager *m) { manager = m; }
            void setSkillId(ESM::Skill::SkillEnum skillId);
            void setSkillNumber(int skillId);
            void setSkillValue(const SkillValue& value);

            WindowManager *getWindowManager() const { return manager; }
            ESM::Skill::SkillEnum getSkillId() const { return skillId; }
            const SkillValue& getSkillValue() const { return value; }

            // Events
            typedef delegates::CDelegate1<MWSkill*> EventHandle_SkillVoid;

            /** Event : Skill clicked.\n
                signature : void method(MWSkill* _sender)\n
            */
            EventHandle_SkillVoid eventClicked;

        protected:
            virtual ~MWSkill();

            virtual void initialiseOverride();

            void onClicked(MyGUI::Widget* _sender);

        private:

            void updateWidgets();

            WindowManager *manager;
            ESM::Skill::SkillEnum skillId;
            SkillValue value;
            MyGUI::WidgetPtr skillNameWidget, skillValueWidget;
        };
        typedef MWSkill* MWSkillPtr;

        class MYGUI_EXPORT MWAttribute : public Widget
        {
            MYGUI_RTTI_DERIVED( MWAttribute );
        public:
            MWAttribute();

            typedef MWMechanics::Stat<int> AttributeValue;

            void setWindowManager(WindowManager *m) { manager = m; }
            void setAttributeId(int attributeId);
            void setAttributeValue(const AttributeValue& value);

            WindowManager *getWindowManager() const { return manager; }
            int getAttributeId() const { return id; }
            const AttributeValue& getAttributeValue() const { return value; }

            // Events
            typedef delegates::CDelegate1<MWAttribute*> EventHandle_AttributeVoid;

            /** Event : Attribute clicked.\n
                signature : void method(MWAttribute* _sender)\n
            */
            EventHandle_AttributeVoid eventClicked;

        protected:
            virtual ~MWAttribute();

            virtual void initialiseOverride();

            void onClicked(MyGUI::Widget* _sender);

        private:

            void updateWidgets();

            WindowManager *manager;
            int id;
            AttributeValue value;
            MyGUI::WidgetPtr attributeNameWidget, attributeValueWidget;
        };
        typedef MWAttribute* MWAttributePtr;

        class MWSpellEffect;
        class MYGUI_EXPORT MWSpell : public Widget
        {
            MYGUI_RTTI_DERIVED( MWSpell );
        public:
            MWSpell();

            typedef MWMechanics::Stat<int> SpellValue;

            void setWindowManager(WindowManager* parWindowManager) { mWindowManager = parWindowManager; }
            void setSpellId(const std::string &id);
            void createEffectWidgets(std::vector<MyGUI::WidgetPtr> &effects, MyGUI::WidgetPtr creator, MyGUI::IntCoord &coord);

            const std::string &getSpellId() const { return id; }

        protected:
            virtual ~MWSpell();

		virtual void initialiseOverride();

        private:
            void updateWidgets();

            WindowManager* mWindowManager;
            std::string id;
            MyGUI::StaticTextPtr spellNameWidget;
        };
        typedef MWSpell* MWSpellPtr;

        class MYGUI_EXPORT MWSpellEffect : public Widget
        {
            MYGUI_RTTI_DERIVED( MWSpellEffect );
        public:
            MWSpellEffect();

            typedef ESM::ENAMstruct SpellEffectValue;

            void setWindowManager(WindowManager* parWindowManager) { mWindowManager = parWindowManager; }
            void setSpellEffect(SpellEffectValue value);

            const SpellEffectValue &getSpellEffect() const { return effect; }

        protected:
            virtual ~MWSpellEffect();

            virtual void initialiseOverride();
            
        private:

            void updateWidgets();

            WindowManager* mWindowManager;
            SpellEffectValue effect;
            MyGUI::StaticImagePtr imageWidget;
            MyGUI::StaticTextPtr textWidget;
        };
        typedef MWSpellEffect* MWSpellEffectPtr;

        class MYGUI_EXPORT MWDynamicStat : public Widget
        {
            MYGUI_RTTI_DERIVED( MWDynamicStat );
        public:
            MWDynamicStat();

            void setValue(int value, int max);
            void setTitle(const std::string text);

            int getValue() const { return value; }
            int getMax() const { return max; }

        protected:
            virtual ~MWDynamicStat();

            virtual void initialiseOverride();

        private:

            int value, max;
            MyGUI::StaticTextPtr textWidget;
            MyGUI::ProgressPtr barWidget;
            MyGUI::StaticTextPtr barTextWidget;
        };
        typedef MWDynamicStat* MWDynamicStatPtr;

    }
}

#endif
