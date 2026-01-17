#ifndef MWGUI_WIDGETS_H
#define MWGUI_WIDGETS_H

#include "../mwmechanics/stat.hpp"

#include <MyGUI_Delegate.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Widget.h>

#include <components/esm/attr.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/effectlist.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadskil.hpp>

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

        struct SpellEffectParams
        {
            SpellEffectParams()
                : mNoTarget(false)
                , mIsConstant(false)
                , mNoMagnitude(false)
                , mKnown(true)
                , mMagnMin(-1)
                , mMagnMax(-1)
                , mRange(-1)
                , mDuration(-1)
                , mArea(0)
            {
            }

            bool mNoTarget; // potion effects for example have no target (target is always the player)
            bool mIsConstant; // constant effect means that duration will not be displayed
            bool mNoMagnitude; // effect magnitude will not be displayed (e.g ingredients)

            bool mKnown; // is this effect known to the player? (If not, will display as a question mark instead)

            // value of EmptyRefId here means the effect is unknown to the player
            ESM::RefId mEffectID, mSkill, mAttribute;

            // value of -1 here means the value is unavailable
            int mMagnMin, mMagnMax, mRange, mDuration;

            // value of 0 -> no area effect
            int mArea;

            bool operator==(const SpellEffectParams& other) const
            {
                if (mEffectID != other.mEffectID)
                    return false;

                bool involvesAttribute = (mEffectID == ESM::MagicEffect::RestoreAttribute
                    || mEffectID == ESM::MagicEffect::AbsorbAttribute || mEffectID == ESM::MagicEffect::DrainAttribute
                    || mEffectID == ESM::MagicEffect::FortifyAttribute
                    || mEffectID == ESM::MagicEffect::DamageAttribute);
                bool involvesSkill = (mEffectID == ESM::MagicEffect::RestoreSkill
                    || mEffectID == ESM::MagicEffect::AbsorbSkill || mEffectID == ESM::MagicEffect::DrainSkill
                    || mEffectID == ESM::MagicEffect::FortifySkill || mEffectID == ESM::MagicEffect::DamageSkill);
                return ((other.mSkill == mSkill) || !involvesSkill)
                    && ((other.mAttribute == mAttribute) && !involvesAttribute) && (other.mArea == mArea);
            }
        };

        typedef std::vector<SpellEffectParams> SpellEffectList;

        class MWSkill final : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED(MWSkill)
        public:
            MWSkill();

            typedef MWMechanics::Stat<float> SkillValue;

            void setSkillId(ESM::RefId skillId);
            void setSkillValue(const SkillValue& value);

            ESM::RefId getSkillId() const { return mSkillId; }
            const SkillValue& getSkillValue() const { return mValue; }

            // Events
            typedef MyGUI::delegates::MultiDelegate<MWSkill*> EventHandle_SkillVoid;

            /** Event : Skill clicked.\n
                signature : void method(MWSkill* sender)\n
            */
            EventHandle_SkillVoid eventClicked;

            void setStateSelected(bool selected);

        protected:
            virtual ~MWSkill();

            void initialiseOverride() override;

            void onClicked(MyGUI::Widget* sender);

        private:
            void updateWidgets();

            ESM::RefId mSkillId;
            SkillValue mValue;
            MyGUI::TextBox* mSkillNameWidget;
            MyGUI::TextBox* mSkillValueWidget;
        };
        typedef MWSkill* MWSkillPtr;

        class MWAttribute final : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED(MWAttribute)
        public:
            MWAttribute();

            typedef MWMechanics::AttributeValue AttributeValue;

            void setAttributeId(ESM::RefId attributeId);
            void setAttributeValue(const AttributeValue& value);

            ESM::RefId getAttributeId() const { return mId; }
            const AttributeValue& getAttributeValue() const { return mValue; }

            // Events
            typedef MyGUI::delegates::MultiDelegate<MWAttribute*> EventHandle_AttributeVoid;

            /** Event : Attribute clicked.\n
                signature : void method(MWAttribute* sender)\n
            */
            EventHandle_AttributeVoid eventClicked;

            void setStateSelected(bool selected);

        protected:
            ~MWAttribute() override = default;

            void initialiseOverride() override;

            void onClicked(MyGUI::Widget* sender);

        private:
            void updateWidgets();

            ESM::RefId mId;
            AttributeValue mValue;
            MyGUI::TextBox* mAttributeNameWidget;
            MyGUI::TextBox* mAttributeValueWidget;
        };
        typedef MWAttribute* MWAttributePtr;

        /**
         * @todo remove this class and use MWEffectList instead
         */
        class MWSpellEffect;
        class MWSpell final : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED(MWSpell)
        public:
            MWSpell();

            void setSpellId(const ESM::RefId& id);

            /**
             * @param vector to store the created effect widgets
             * @param parent widget
             * @param coordinates to use, will be expanded if more space is needed
             * @param spell category, if this is 0, this means the spell effects are permanent and won't display e.g.
             * duration
             * @param various flags, see MWEffectList::EffectFlags
             */
            void createEffectWidgets(
                std::vector<MyGUI::Widget*>& effects, MyGUI::Widget* creator, MyGUI::IntCoord& coord, int flags);

            const ESM::RefId& getSpellId() const { return mId; }

            void setStateSelected(bool selected);

        protected:
            virtual ~MWSpell();

            void initialiseOverride() override;

        private:
            void updateWidgets();

            ESM::RefId mId;
            MyGUI::TextBox* mSpellNameWidget;
        };
        typedef MWSpell* MWSpellPtr;

        class MWEffectList final : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED(MWEffectList)
        public:
            MWEffectList();

            enum EffectFlags
            {
                EF_NoTarget = 0x01, // potions have no target (target is always the player)
                EF_Constant = 0x02, // constant effect means that duration will not be displayed
                EF_NoMagnitude = 0x04 // ingredients have no magnitude

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
            void createEffectWidgets(std::vector<MyGUI::Widget*>& effects, MyGUI::Widget* creator,
                MyGUI::IntCoord& coord, bool center, int flags);

        protected:
            virtual ~MWEffectList();

            void initialiseOverride() override;

        private:
            void updateWidgets();

            SpellEffectList mEffectList;
        };
        typedef MWEffectList* MWEffectListPtr;

        class MWSpellEffect final : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED(MWSpellEffect)
        public:
            MWSpellEffect();

            typedef ESM::ENAMstruct SpellEffectValue;

            void setSpellEffect(const SpellEffectParams& params);

            int getRequestedWidth() const { return mRequestedWidth; }

            void setStateSelected(bool selected);

        protected:
            virtual ~MWSpellEffect();

            void initialiseOverride() override;

        private:
            static constexpr int sIconOffset = 24;

            void updateWidgets();

            SpellEffectParams mEffectParams;
            MyGUI::ImageBox* mImageWidget;
            MyGUI::TextBox* mTextWidget;
            int mRequestedWidth;
        };
        typedef MWSpellEffect* MWSpellEffectPtr;

        class MWDynamicStat final : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED(MWDynamicStat)
        public:
            MWDynamicStat();

            void setValue(int value, int max);
            void setTitle(std::string_view text);

            int getValue() const { return mValue; }
            int getMax() const { return mMax; }

        protected:
            virtual ~MWDynamicStat();

            void initialiseOverride() override;

        private:
            int mValue, mMax;
            MyGUI::TextBox* mTextWidget;
            MyGUI::ProgressBar* mBarWidget;
            MyGUI::TextBox* mBarTextWidget;
        };
        typedef MWDynamicStat* MWDynamicStatPtr;
    }
}

#endif
