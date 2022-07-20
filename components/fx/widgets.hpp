#ifndef OPENMW_COMPONENTS_FX_WIDGETS_H
#define OPENMW_COMPONENTS_FX_WIDGETS_H

#include <MyGUI_Gui.h>
#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <components/misc/stringops.hpp>

#include "technique.hpp"
#include "types.hpp"

namespace Gui
{
    class AutoSizedTextBox;
    class AutoSizedButton;
}

namespace fx
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

            void setData(const std::shared_ptr<fx::Types::UniformBase>& uniform, Index index = None)
            {
                mUniform = uniform;
                mIndex = index;
            }

            virtual void setValueFromUniform() = 0;

            virtual void toDefault() = 0;

        protected:
            std::weak_ptr<fx::Types::UniformBase> mUniform;
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

            MyGUI::Button* mCheckbutton{nullptr};
            MyGUI::Widget* mFill{nullptr};
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
                    mValueLabel->setCaption(Misc::StringUtils::format("%.3f", mValue));
                else
                    mValueLabel->setCaption(std::to_string(mValue));

                float range = 0.f;
                float min = 0.f;

                if (auto uniform = mUniform.lock())
                {
                    if constexpr (std::is_fundamental_v<UType>)
                    {
                        uniform->template setValue<UType>(mValue);
                        range = uniform->template getMax<UType>() - uniform->template getMin<UType>();
                        min = uniform->template getMin<UType>();
                    }
                    else
                    {
                        UType uvalue = uniform->template getValue<UType>();
                        uvalue[mIndex] = mValue;
                        uniform->template setValue<UType>(uvalue);
                        range = uniform->template getMax<UType>()[mIndex] - uniform->template getMin<UType>()[mIndex];
                        min = uniform->template getMin<UType>()[mIndex];
                    }
                }

                float fill = (range == 0.f) ? 1.f : (mValue - min) / range;
                mFill->setRealSize(fill, 1.0);
            }

            void setValueFromUniform() override
            {
                if (auto uniform = mUniform.lock())
                {
                    T value;

                    if constexpr (std::is_fundamental_v<UType>)
                        value = uniform->template getValue<UType>();
                    else
                        value = uniform->template getValue<UType>()[mIndex];

                    setValue(value);
                }
            }

            void toDefault() override
            {
                if (auto uniform = mUniform.lock())
                {
                    if constexpr (std::is_fundamental_v<UType>)
                        setValue(uniform->template getDefault<UType>());
                    else
                        setValue(uniform->template getDefault<UType>()[mIndex]);
                }
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

            void notifyMouseWheel(MyGUI::Widget* sender, int rel)
            {
                auto uniform = mUniform.lock();

                if (!uniform)
                    return;

                if (rel > 0)
                    increment(uniform->mStep);
                else
                    increment(-uniform->mStep);
            }

            void notifyMouseButtonDragged(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id)
            {
                if (id != MyGUI::MouseButton::Left)
                    return;

                auto uniform = mUniform.lock();

                if (!uniform)
                    return;

                int delta = left - mLastPointerX;

                // allow finer tuning when shift is pressed
                constexpr double scaling = 20.0;
                T step = MyGUI::InputManager::getInstance().isShiftPressed() ? uniform->mStep / scaling : uniform->mStep;

                if (step == 0)
                {
                    if constexpr (std::is_integral_v<T>)
                        step = 1;
                    else
                        step = uniform->mStep;
                }

                if (delta > 0)
                    increment(step);
                else if (delta < 0)
                    increment(-step);

                mLastPointerX = left;
            }

            void notifyMouseButtonPressed(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id)
            {
                if (id != MyGUI::MouseButton::Left)
                    return;

                mLastPointerX = left;
            }

            void increment(T step)
            {
                auto uniform = mUniform.lock();

                if (!uniform)
                    return;

                if constexpr (std::is_fundamental_v<UType>)
                    setValue(std::clamp<T>(uniform->template getValue<UType>() + step, uniform->template getMin<UType>(), uniform->template getMax<T>()));
                else
                    setValue(std::clamp<T>(uniform->template getValue<UType>()[mIndex] + step, uniform->template getMin<UType>()[mIndex], uniform->template getMax<UType>()[mIndex]));
            }

            void notifyButtonClicked(MyGUI::Widget* sender)
            {
                auto uniform = mUniform.lock();

                if (!uniform)
                    return;

                if (sender == mButtonDecrease)
                    increment(-uniform->mStep);
                else if (sender == mButtonIncrease)
                    increment(uniform->mStep);
            }

            MyGUI::Button* mButtonDecrease{nullptr};
            MyGUI::Button* mButtonIncrease{nullptr};
            MyGUI::Widget* mDragger{nullptr};
            MyGUI::Widget* mFill{nullptr};
            MyGUI::TextBox* mValueLabel{nullptr};
            T mValue{};

            int mLastPointerX{0};
        };

        class EditNumberFloat4 : public EditNumber<float, osg::Vec4f> { MYGUI_RTTI_DERIVED(EditNumberFloat4) };
        class EditNumberFloat3 : public EditNumber<float, osg::Vec3f> { MYGUI_RTTI_DERIVED(EditNumberFloat3) };
        class EditNumberFloat2 : public EditNumber<float, osg::Vec2f> { MYGUI_RTTI_DERIVED(EditNumberFloat2) };
        class EditNumberFloat : public EditNumber<float, float> { MYGUI_RTTI_DERIVED(EditNumberFloat) };
        class EditNumberInt : public EditNumber<int, int> { MYGUI_RTTI_DERIVED(EditNumberInt) };

        class UniformBase final : public MyGUI::Widget
        {
            MYGUI_RTTI_DERIVED(UniformBase)

        public:
            void init(const std::shared_ptr<fx::Types::UniformBase>& uniform);

            void toDefault();

            void addItem(EditBase* item);

            Gui::AutoSizedTextBox* getLabel() { return mLabel; }

        private:

            void notifyResetClicked(MyGUI::Widget* sender);

            void initialiseOverride() override;

            Gui::AutoSizedButton* mReset{nullptr};
            Gui::AutoSizedTextBox* mLabel{nullptr};
            MyGUI::Widget* mClient{nullptr};
            std::vector<EditBase*> mBases;
        };
    }
}

#endif
