#ifndef MWGUI_WIDGETS_H
#define MWGUI_WIDGETS_H

#include "../mwmechanics/stat.hpp"

#include <components/esm/effectlist.hpp>
#include <components/esm/loadskil.hpp>

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_ScrollBar.h>

namespace MyGUI
{
    class ImageBox;
    class ControllerItem;
}

namespace MWBase
{
    class WindowManager;
}

/*
  This file contains various custom widgets used in OpenMW.
 */

namespace MWGui
{
    namespace Widgets
    {
        class MWEffectList;

        void fixTexturePath(std::string &path);

        struct SpellEffectParams
        {
            SpellEffectParams()
                : mNoTarget(false)
                , mIsConstant(false)
                , mKnown(true)
                , mEffectID(-1)
                , mSkill(-1)
                , mAttribute(-1)
                , mMagnMin(-1)
                , mMagnMax(-1)
                , mRange(-1)
                , mDuration(-1)
                , mArea(0)
            {
            }

            bool mNoTarget; // potion effects for example have no target (target is always the player)
            bool mIsConstant; // constant effect means that duration will not be displayed

            bool mKnown; // is this effect known to the player? (If not, will display as a question mark instead)

            // value of -1 here means the effect is unknown to the player
            short mEffectID;

            // value of -1 here means there is no skill/attribute
            signed char mSkill, mAttribute;

            // value of -1 here means the value is unavailable
            int mMagnMin, mMagnMax, mRange, mDuration;

            // value of 0 -> no area effect
            int mArea;

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
                return ((other.mSkill == mSkill) || !involvesSkill) && ((other.mAttribute == mAttribute) && !involvesAttribute) && (other.mArea == mArea);
            }
        };

        typedef std::vector<SpellEffectParams> SpellEffectList;

        class MWSkill : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWSkill )
        public:
            MWSkill();

            typedef MWMechanics::Stat<float> SkillValue;

            void setSkillId(ESM::Skill::SkillEnum skillId);
            void setSkillNumber(int skillId);
            void setSkillValue(const SkillValue& value);

            ESM::Skill::SkillEnum getSkillId() const { return mSkillId; }
            const SkillValue& getSkillValue() const { return mValue; }

            // Events
            typedef MyGUI::delegates::CMultiDelegate1<MWSkill*> EventHandle_SkillVoid;

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

            ESM::Skill::SkillEnum mSkillId;
            SkillValue mValue;
            MyGUI::TextBox* mSkillNameWidget;
            MyGUI::TextBox* mSkillValueWidget;
        };
        typedef MWSkill* MWSkillPtr;

        class MWAttribute : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWAttribute )
        public:
            MWAttribute();

            typedef MWMechanics::AttributeValue AttributeValue;

            void setAttributeId(int attributeId);
            void setAttributeValue(const AttributeValue& value);

            int getAttributeId() const { return mId; }
            const AttributeValue& getAttributeValue() const { return mValue; }

            // Events
            typedef MyGUI::delegates::CMultiDelegate1<MWAttribute*> EventHandle_AttributeVoid;

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

            int mId;
            AttributeValue mValue;
            MyGUI::TextBox* mAttributeNameWidget;
            MyGUI::TextBox* mAttributeValueWidget;
        };
        typedef MWAttribute* MWAttributePtr;

        /**
         * @todo remove this class and use MWEffectList instead
         */
        class MWSpellEffect;
        class MWSpell : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWSpell )
        public:
            MWSpell();

            typedef MWMechanics::Stat<int> SpellValue;

            void setSpellId(const std::string &id);

            /**
             * @param vector to store the created effect widgets
             * @param parent widget
             * @param coordinates to use, will be expanded if more space is needed
             * @param spell category, if this is 0, this means the spell effects are permanent and won't display e.g. duration
             * @param various flags, see MWEffectList::EffectFlags
             */
            void createEffectWidgets(std::vector<MyGUI::Widget*> &effects, MyGUI::Widget* creator, MyGUI::IntCoord &coord, int flags);

            const std::string &getSpellId() const { return mId; }

        protected:
            virtual ~MWSpell();

            virtual void initialiseOverride();

        private:
            void updateWidgets();

            std::string mId;
            MyGUI::TextBox* mSpellNameWidget;
        };
        typedef MWSpell* MWSpellPtr;

        class MWEffectList : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWEffectList )
        public:
            MWEffectList();

            typedef MWMechanics::Stat<int> EnchantmentValue;

            enum EffectFlags
            {
                EF_NoTarget = 0x01, // potions have no target (target is always the player)
                EF_Constant = 0x02 // constant effect means that duration will not be displayed
            };

            void setEffectList(const SpellEffectList& list);

            static SpellEffectList effectListFromESM(const ESM::EffectList* effects);

            /**
             * @param vector to store the created effect widgets
             * @param parent widget
             * @param coordinates to use, will be expanded if more space is needed
             * @param center the effect widgets horizontally
             * @param various flags, see MWEffectList::EffectFlags
             */
            void createEffectWidgets(std::vector<MyGUI::Widget*> &effects, MyGUI::Widget* creator, MyGUI::IntCoord &coord, bool center, int flags);

        protected:
            virtual ~MWEffectList();

            virtual void initialiseOverride();

        private:
            void updateWidgets();

            SpellEffectList mEffectList;
        };
        typedef MWEffectList* MWEffectListPtr;

        class MWSpellEffect : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWSpellEffect )
        public:
            MWSpellEffect();

            typedef ESM::ENAMstruct SpellEffectValue;

            void setSpellEffect(const SpellEffectParams& params);

            int getRequestedWidth() const { return mRequestedWidth; }

        protected:
            virtual ~MWSpellEffect();

            virtual void initialiseOverride();

        private:

            void updateWidgets();

            SpellEffectParams mEffectParams;
            MyGUI::ImageBox* mImageWidget;
            MyGUI::TextBox* mTextWidget;
            int mRequestedWidth;
        };
        typedef MWSpellEffect* MWSpellEffectPtr;

        class MWDynamicStat : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWDynamicStat )
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
            MyGUI::ProgressBar* mBarWidget;
            MyGUI::TextBox* mBarTextWidget;
        };
        typedef MWDynamicStat* MWDynamicStatPtr;

        // Should be removed when upgrading to MyGUI 3.2.2 (current git), it has ScrollBar autorepeat support
        class MWScrollBar : public MyGUI::ScrollBar
        {
            MYGUI_RTTI_DERIVED(MWScrollBar)

        public:
            MWScrollBar();
            virtual ~MWScrollBar();

            void setRepeat(float trigger, float step);

        protected:
            virtual void initialiseOverride();
            void repeatClick(MyGUI::Widget* _widget, MyGUI::ControllerItem* _controller);

            bool mEnableRepeat;
            float mRepeatTriggerTime;
            float mRepeatStepTime;
            bool mIsIncreasing;

        private:
            void onDecreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
            void onDecreaseButtonReleased(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
            void onIncreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
            void onIncreaseButtonReleased(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        };
    }
}

#endif
