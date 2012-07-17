#ifndef MWGUI_WIDGETS_H
#define MWGUI_WIDGETS_H

#include <components/esm_store/store.hpp>

#include <MyGUI.h>

#include "../mwmechanics/stat.hpp"

#undef MYGUI_EXPORT
#define MYGUI_EXPORT

/*
  This file contains various custom widgets used in OpenMW.
 */

namespace MWGui
{
    using namespace MyGUI;
    class WindowManager;

    namespace Widgets
    {
        class MWEffectList;

        void fixTexturePath(std::string &path);

        struct SpellEffectParams
        {
            SpellEffectParams()
                : mMagnMin(-1)
                , mMagnMax(-1)
                , mRange(-1)
                , mDuration(-1)
                , mSkill(-1)
                , mAttribute(-1)
                , mEffectID(-1)
                , mNoTarget(false)
                , mIsConstant(false)
            {
            }

            bool mNoTarget; // potion effects for example have no target (target is always the player)
            bool mIsConstant; // constant effect means that duration will not be displayed

            // value of -1 here means the effect is unknown to the player
            short mEffectID;

            // value of -1 here means there is no skill/attribute
            signed char mSkill, mAttribute;

            // value of -1 here means the value is unavailable
            int mMagnMin, mMagnMax, mRange, mDuration;

            bool operator==(const SpellEffectParams& other) const
            {
                if (mEffectID !=  other.mEffectID)
                    return false;

                bool involvesAttribute = (mEffectID == 74 // restore attribute
                                        || mEffectID == 85 // absorb attribute
                                        || mEffectID == 17 // drain attribute
                                        || mEffectID == 79 // fortify attribute
                                        || mEffectID == 22); // damage attribute
                bool involvesSkill = (mEffectID == 78 // restore skill
                                        || mEffectID == 89 // absorb skill
                                        || mEffectID == 21 // drain skill
                                        || mEffectID == 83 // fortify skill
                                        || mEffectID == 26); // damage skill
                return ((other.mSkill == mSkill) || !involvesSkill) && ((other.mAttribute == mAttribute) && !involvesAttribute);
            }
        };

        typedef std::vector<SpellEffectParams> SpellEffectList;

        class MYGUI_EXPORT MWSkill : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWSkill );
        public:
            MWSkill();

            typedef MWMechanics::Stat<float> SkillValue;

            void setWindowManager(WindowManager *m) { mManager = m; }
            void setSkillId(ESM::Skill::SkillEnum skillId);
            void setSkillNumber(int skillId);
            void setSkillValue(const SkillValue& value);

            WindowManager *getWindowManager() const { return mManager; }
            ESM::Skill::SkillEnum getSkillId() const { return mSkillId; }
            const SkillValue& getSkillValue() const { return mValue; }

            // Events
            typedef delegates::CMultiDelegate1<MWSkill*> EventHandle_SkillVoid;

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

            WindowManager *mManager;
            ESM::Skill::SkillEnum mSkillId;
            SkillValue mValue;
            MyGUI::WidgetPtr mSkillNameWidget, mSkillValueWidget;
        };
        typedef MWSkill* MWSkillPtr;

        class MYGUI_EXPORT MWAttribute : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWAttribute );
        public:
            MWAttribute();

            typedef MWMechanics::Stat<int> AttributeValue;

            void setWindowManager(WindowManager *m) { mManager = m; }
            void setAttributeId(int attributeId);
            void setAttributeValue(const AttributeValue& value);

            WindowManager *getWindowManager() const { return mManager; }
            int getAttributeId() const { return mId; }
            const AttributeValue& getAttributeValue() const { return mValue; }

            // Events
            typedef delegates::CMultiDelegate1<MWAttribute*> EventHandle_AttributeVoid;

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

            WindowManager *mManager;
            int mId;
            AttributeValue mValue;
            MyGUI::WidgetPtr mAttributeNameWidget, mAttributeValueWidget;
        };
        typedef MWAttribute* MWAttributePtr;

        /**
         * @todo remove this class and use MWEffectList instead
         */
        class MWSpellEffect;
        class MYGUI_EXPORT MWSpell : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWSpell );
        public:
            MWSpell();

            typedef MWMechanics::Stat<int> SpellValue;

            void setWindowManager(WindowManager* parWindowManager) { mWindowManager = parWindowManager; }
            void setSpellId(const std::string &id);

            /**
             * @param vector to store the created effect widgets
             * @param parent widget
             * @param coordinates to use, will be expanded if more space is needed
             * @param spell category, if this is 0, this means the spell effects are permanent and won't display e.g. duration
             * @param various flags, see MWEffectList::EffectFlags
             */
            void createEffectWidgets(std::vector<MyGUI::WidgetPtr> &effects, MyGUI::WidgetPtr creator, MyGUI::IntCoord &coord, int flags);

            const std::string &getSpellId() const { return mId; }

        protected:
            virtual ~MWSpell();

            virtual void initialiseOverride();

        private:
            void updateWidgets();

            WindowManager* mWindowManager;
            std::string mId;
            MyGUI::TextBox* mSpellNameWidget;
        };
        typedef MWSpell* MWSpellPtr;

        class MYGUI_EXPORT MWEffectList : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWEffectList );
        public:
            MWEffectList();

            typedef MWMechanics::Stat<int> EnchantmentValue;

            enum EffectFlags
            {
                EF_NoTarget = 0x01, // potions have no target (target is always the player)
                EF_Constant = 0x02 // constant effect means that duration will not be displayed
            };

            void setWindowManager(WindowManager* parWindowManager) { mWindowManager = parWindowManager; }
            void setEffectList(const SpellEffectList& list);

            static SpellEffectList effectListFromESM(const ESM::EffectList* effects);

            /**
             * @param vector to store the created effect widgets
             * @param parent widget
             * @param coordinates to use, will be expanded if more space is needed
             * @param center the effect widgets horizontally
             * @param various flags, see MWEffectList::EffectFlags
             */
            void createEffectWidgets(std::vector<MyGUI::WidgetPtr> &effects, MyGUI::WidgetPtr creator, MyGUI::IntCoord &coord, bool center, int flags);

        protected:
            virtual ~MWEffectList();

            virtual void initialiseOverride();

        private:
            void updateWidgets();

            WindowManager* mWindowManager;
            SpellEffectList mEffectList;
        };
        typedef MWEffectList* MWEffectListPtr;

        class MYGUI_EXPORT MWSpellEffect : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWSpellEffect );
        public:
            MWSpellEffect();

            typedef ESM::ENAMstruct SpellEffectValue;

            void setWindowManager(WindowManager* parWindowManager) { mWindowManager = parWindowManager; }
            void setSpellEffect(const SpellEffectParams& params);

            std::string effectIDToString(const short effectID);
            bool effectHasMagnitude (const std::string& effect);
            bool effectHasDuration (const std::string& effect);
            bool effectInvolvesAttribute (const std::string& effect);
            bool effectInvolvesSkill (const std::string& effect);

            int getRequestedWidth() const { return mRequestedWidth; }

        protected:
            virtual ~MWSpellEffect();

            virtual void initialiseOverride();
            
        private:

            void updateWidgets();

            WindowManager* mWindowManager;
            SpellEffectParams mEffectParams;
            MyGUI::ImageBox* mImageWidget;
            MyGUI::TextBox* mTextWidget;
            int mRequestedWidth;
        };
        typedef MWSpellEffect* MWSpellEffectPtr;

        class MYGUI_EXPORT MWDynamicStat : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWDynamicStat );
        public:
            MWDynamicStat();

            void setValue(int value, int max);
            void setTitle(const std::string& text);

            int getValue() const { return mValue; }
            int getMax() const { return mMax; }

        protected:
            virtual ~MWDynamicStat();

            virtual void initialiseOverride();

        private:

            int mValue, mMax;
            MyGUI::TextBox* mTextWidget;
            MyGUI::ProgressPtr mBarWidget;
            MyGUI::TextBox* mBarTextWidget;
        };
        typedef MWDynamicStat* MWDynamicStatPtr;
    }
}

#endif
