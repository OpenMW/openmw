#ifndef OPENMW_COMPONENTS_FX_WIDGETS_HPP
#define OPENMW_COMPONENTS_FX_WIDGETS_HPP

#include <MyGUI_Button.h>
#include <MyGUI_Delegate.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_MouseButton.h>
#include <MyGUI_RTTI.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Widget.h>

#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include "types.hpp"

namespace Gui
{
    class AutoSizedTextBox;
    class AutoSizedButton;
}

namespace Fx
{
    namespace Widgets
    {
        enum Index
        {
            None = -1,
            Zero = 0,
            One = 1,
            Two = 2,
            Three = 3
        };

        class EditBase
        {
        public:
            virtual ~EditBase() = default;

            void setData(const std::shared_ptr<Fx::Types::UniformBase>& uniform, Index index = None)
            {
                mUniform = uniform;
                mIndex = index;
            }

            virtual void setValueFromUniform() = 0;

            virtual void toDefault() = 0;

        protected:
            std::shared_ptr<Fx::Types::UniformBase> mUniform;
            Index mIndex;
        };

        class EditBool : public EditBase, public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED(EditBool)

        public:
            void setValue(bool value);
            void setValueFromUniform() override;
            void toDefault() override;

        private:
            void initialiseOverride() override;
            void notifyMouseButtonClick(MyGUI::Widget* sender);

            MyGUI::Button* mCheckbutton{ nullptr };
            MyGUI::Widget* mFill{ nullptr };
        };

        template <class T, class UType>
        class EditNumber : public EditBase, public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED(EditNumber)

        public:
            void setValue(T value)
            {
                mValue = value;
                if constexpr (std::is_floating_point_v<T>)
                    mValueLabel->setCaption(std::format("{:.3f}", mValue));
                else
                    mValueLabel->setCaption(std::to_string(mValue));

                float range = 0.f;
                float min = 0.f;

                if constexpr (std::is_fundamental_v<UType>)
                {
                    mUniform->template setValue<UType>(mValue);
                    range = static_cast<float>(mUniform->template getMax<UType>() - mUniform->template getMin<UType>());
                    min = static_cast<float>(mUniform->template getMin<UType>());
                }
                else
                {
                    UType uvalue = mUniform->template getValue<UType>();
                    uvalue[mIndex] = mValue;
                    mUniform->template setValue<UType>(uvalue);
                    range = mUniform->template getMax<UType>()[mIndex] - mUniform->template getMin<UType>()[mIndex];
                    min = mUniform->template getMin<UType>()[mIndex];
                }

                float fill = (range == 0.f) ? 1.f : (mValue - min) / range;
                mFill->setRealSize(fill, 1.0);
            }

            void setValueFromUniform() override
            {
                T value;

                if constexpr (std::is_fundamental_v<UType>)
                    value = mUniform->template getValue<UType>();
                else
                    value = mUniform->template getValue<UType>()[mIndex];

                setValue(value);
            }

            void toDefault() override
            {
                if constexpr (std::is_fundamental_v<UType>)
                    setValue(mUniform->template getDefault<UType>());
                else
                    setValue(mUniform->template getDefault<UType>()[mIndex]);
            }

        private:
            void initialiseOverride() override
            {
                Base::initialiseOverride();

                assignWidget(mDragger, "Dragger");
                assignWidget(mValueLabel, "Value");
                assignWidget(mButtonIncrease, "ButtonIncrease");
                assignWidget(mButtonDecrease, "ButtonDecrease");
                assignWidget(mFill, "Fill");

                mButtonIncrease->eventMouseButtonClick += MyGUI::newDelegate(this, &EditNumber::notifyButtonClicked);
                mButtonDecrease->eventMouseButtonClick += MyGUI::newDelegate(this, &EditNumber::notifyButtonClicked);

                mDragger->eventMouseButtonPressed += MyGUI::newDelegate(this, &EditNumber::notifyMouseButtonPressed);
                mDragger->eventMouseDrag += MyGUI::newDelegate(this, &EditNumber::notifyMouseButtonDragged);
                mDragger->eventMouseWheel += MyGUI::newDelegate(this, &EditNumber::notifyMouseWheel);
            }

            void notifyMouseWheel(MyGUI::Widget* /*sender*/, int rel)
            {
                if (rel > 0)
                    increment(static_cast<T>(mUniform->mStep));
                else
                    increment(static_cast<T>(-mUniform->mStep));
            }

            void notifyMouseButtonDragged(MyGUI::Widget* /*sender*/, int left, int top, MyGUI::MouseButton id)
            {
                if (id != MyGUI::MouseButton::Left)
                    return;

                int delta = left - mLastPointerX;

                // allow finer tuning when shift is pressed
                constexpr double scaling = 20.0;
                T step = static_cast<T>(
                    MyGUI::InputManager::getInstance().isShiftPressed() ? mUniform->mStep / scaling : mUniform->mStep);

                if (step == 0)
                {
                    if constexpr (std::is_integral_v<T>)
                        step = 1;
                    else
                        step = static_cast<T>(mUniform->mStep);
                }

                if (delta > 0)
                    increment(step);
                else if (delta < 0)
                    increment(-step);

                mLastPointerX = left;
            }

            void notifyMouseButtonPressed(MyGUI::Widget* /*sender*/, int left, int top, MyGUI::MouseButton id)
            {
                if (id != MyGUI::MouseButton::Left)
                    return;

                mLastPointerX = left;
            }

            void increment(T step)
            {
                if constexpr (std::is_fundamental_v<UType>)
                    setValue(std::clamp<T>(mUniform->template getValue<UType>() + step,
                        mUniform->template getMin<UType>(), mUniform->template getMax<T>()));
                else
                    setValue(std::clamp<T>(mUniform->template getValue<UType>()[mIndex] + step,
                        mUniform->template getMin<UType>()[mIndex], mUniform->template getMax<UType>()[mIndex]));
            }

            void notifyButtonClicked(MyGUI::Widget* sender)
            {
                if (sender == mButtonDecrease)
                    increment(static_cast<T>(-mUniform->mStep));
                else if (sender == mButtonIncrease)
                    increment(static_cast<T>(mUniform->mStep));
            }

            MyGUI::Button* mButtonDecrease{ nullptr };
            MyGUI::Button* mButtonIncrease{ nullptr };
            MyGUI::Widget* mDragger{ nullptr };
            MyGUI::Widget* mFill{ nullptr };
            MyGUI::TextBox* mValueLabel{ nullptr };
            T mValue{};

            int mLastPointerX{ 0 };
        };

        class EditNumberFloat4 : public EditNumber<float, osg::Vec4f>
        {
            MYGUI_RTTI_DERIVED(EditNumberFloat4)
        };
        class EditNumberFloat3 : public EditNumber<float, osg::Vec3f>
        {
            MYGUI_RTTI_DERIVED(EditNumberFloat3)
        };
        class EditNumberFloat2 : public EditNumber<float, osg::Vec2f>
        {
            MYGUI_RTTI_DERIVED(EditNumberFloat2)
        };
        class EditNumberFloat : public EditNumber<float, float>
        {
            MYGUI_RTTI_DERIVED(EditNumberFloat)
        };
        class EditNumberInt : public EditNumber<int, int>
        {
            MYGUI_RTTI_DERIVED(EditNumberInt)
        };

        class EditChoice : public EditBase, public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED(EditChoice)

        public:
            template <class T>
            void setValue(const T& value);
            void setValueFromUniform() override;
            void toDefault() override;

        private:
            void initialiseOverride() override;
            void notifyComboBoxChanged(MyGUI::ComboBox* sender, size_t pos);

            MyGUI::ComboBox* mChoices{ nullptr };
        };

        class UniformBase final : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED(UniformBase)

        public:
            void init(const std::shared_ptr<Fx::Types::UniformBase>& uniform);

            void toDefault();

            void addItem(EditBase* item);

            Gui::AutoSizedTextBox* getLabel() { return mLabel; }

        private:
            void notifyResetClicked(MyGUI::Widget* sender);

            void initialiseOverride() override;

            Gui::AutoSizedButton* mReset{ nullptr };
            Gui::AutoSizedTextBox* mLabel{ nullptr };
            MyGUI::Widget* mClient{ nullptr };
            std::vector<EditBase*> mBases;
        };
    }
}

#endif
