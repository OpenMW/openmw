#ifndef MWGUI_WIDGETS_H
#define MWGUI_WIDGETS_H

#include "../mwworld/esmstore.hpp"

#include <MyGUI.h>

#include "../mwmechanics/stat.hpp"

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
                : mMagnMin(-1)
                , mMagnMax(-1)
                , mRange(-1)
                , mDuration(-1)
                , mSkill(-1)
                , mArea(0)
                , mAttribute(-1)
                , mEffectID(-1)
                , mNoTarget(false)
                , mIsConstant(false)
                , mKnown(true)
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

            void setWindowManager(MWBase::WindowManager *m) { mManager = m; } /// \todo remove
            void setSkillId(ESM::Skill::SkillEnum skillId);
            void setSkillNumber(int skillId);
            void setSkillValue(const SkillValue& value);

            MWBase::WindowManager *getWindowManager() const { return mManager; }
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

            MWBase::WindowManager *mManager;
            ESM::Skill::SkillEnum mSkillId;
            SkillValue mValue;
            MyGUI::WidgetPtr mSkillNameWidget, mSkillValueWidget;
        };
        typedef MWSkill* MWSkillPtr;

        class MWAttribute : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWAttribute )
        public:
            MWAttribute();

            typedef MWMechanics::Stat<int> AttributeValue;

            void setWindowManager(MWBase::WindowManager *m) { mManager = m; }
            void setAttributeId(int attributeId);
            void setAttributeValue(const AttributeValue& value);

            MWBase::WindowManager *getWindowManager() const { return mManager; }
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

            MWBase::WindowManager *mManager;
            int mId;
            AttributeValue mValue;
            MyGUI::WidgetPtr mAttributeNameWidget, mAttributeValueWidget;
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

            void setWindowManager(MWBase::WindowManager* parWindowManager) { mWindowManager = parWindowManager; }
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

            MWBase::WindowManager* mWindowManager;
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

            void setWindowManager(MWBase::WindowManager* parWindowManager) { mWindowManager = parWindowManager; }
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

            MWBase::WindowManager* mWindowManager;
            SpellEffectList mEffectList;
        };
        typedef MWEffectList* MWEffectListPtr;

        class MWSpellEffect : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( MWSpellEffect )
        public:
            MWSpellEffect();

            typedef ESM::ENAMstruct SpellEffectValue;

            void setWindowManager(MWBase::WindowManager* parWindowManager) { mWindowManager = parWindowManager; }
            void setSpellEffect(const SpellEffectParams& params);

            int getRequestedWidth() const { return mRequestedWidth; }

        protected:
            virtual ~MWSpellEffect();

            virtual void initialiseOverride();

        private:

            void updateWidgets();

            MWBase::WindowManager* mWindowManager;
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
            MyGUI::ProgressPtr mBarWidget;
            MyGUI::TextBox* mBarTextWidget;
        };
        typedef MWDynamicStat* MWDynamicStatPtr;





        // ---------------------------------------------------------------------------------------------------------------------



        class AutoSizedWidget
        {
        public:
            virtual MyGUI::IntSize getRequestedSize() = 0;

        protected:
            void notifySizeChange(MyGUI::Widget* w);

            MyGUI::Align mExpandDirection;
        };

        class AutoSizedTextBox : public AutoSizedWidget, public MyGUI::TextBox
        {
            MYGUI_RTTI_DERIVED( AutoSizedTextBox )

        public:
            virtual MyGUI::IntSize getRequestedSize();
            virtual void setCaption(const MyGUI::UString& _value);

        protected:
            virtual void setPropertyOverride(const std::string& _key, const std::string& _value);
        };

        class AutoSizedButton : public AutoSizedWidget, public MyGUI::Button
        {
            MYGUI_RTTI_DERIVED( AutoSizedButton )

        public:
            virtual MyGUI::IntSize getRequestedSize();
            virtual void setCaption(const MyGUI::UString& _value);

        protected:
            virtual void setPropertyOverride(const std::string& _key, const std::string& _value);
        };

        /**
         * @brief A container widget that automatically sizes its children
         * @note the box being an AutoSizedWidget as well allows to put boxes inside a box
         */
        class Box : public AutoSizedWidget
        {
        public:
            Box();

            void notifyChildrenSizeChanged();

        protected:
            virtual void align() = 0;

            virtual void _setPropertyImpl(const std::string& _key, const std::string& _value);

            int mSpacing; // how much space to put between elements

            int mPadding; // outer padding

            bool mAutoResize; // auto resize the box so that it exactly fits all elements
        };

        class HBox : public Box, public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( HBox )

        public:
            virtual void setSize (const MyGUI::IntSize &_value);
            virtual void setCoord (const MyGUI::IntCoord &_value);

        protected:
            virtual void align();
            virtual MyGUI::IntSize getRequestedSize();

            virtual void setPropertyOverride(const std::string& _key, const std::string& _value);

            virtual void onWidgetCreated(MyGUI::Widget* _widget);
            virtual void onWidgetDestroy(MyGUI::Widget* _widget);
        };

        class VBox : public Box, public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED( VBox)

        public:
            virtual void setSize (const MyGUI::IntSize &_value);
            virtual void setCoord (const MyGUI::IntCoord &_value);

        protected:
            virtual void align();
            virtual MyGUI::IntSize getRequestedSize();

            virtual void setPropertyOverride(const std::string& _key, const std::string& _value);

            virtual void onWidgetCreated(MyGUI::Widget* _widget);
            virtual void onWidgetDestroy(MyGUI::Widget* _widget);
        };
    }
}

#endif
