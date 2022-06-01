#include "widgets.hpp"

#include <components/widgets/box.hpp>

namespace
{
    template <class T, class WidgetT>
    void createVectorWidget(const std::shared_ptr<fx::Types::UniformBase>& uniform, MyGUI::Widget* client, fx::Widgets::UniformBase* base)
    {
        int height = client->getHeight();
        base->setSize(base->getSize().width, (base->getSize().height - height) + (height * T::num_components));
        client->setSize(client->getSize().width, height * T::num_components);

        for (int i = 0; i < T::num_components; ++i)
        {
            auto* widget = client->createWidget<WidgetT>("MW_ValueEditNumber", {0, height * i, client->getWidth(), height}, MyGUI::Align::Default);
            widget->setData(uniform, static_cast<fx::Widgets::Index>(i));
            base->addItem(widget);
        }
    }
}

namespace fx
{
    namespace Widgets
    {
        void EditBool::setValue(bool value)
        {
            auto uniform = mUniform.lock();

            if (!uniform)
                return;

            mCheckbutton->setCaptionWithReplacing(value ? "#{sOn}" : "#{sOff}");
            mFill->setVisible(value);

            uniform->setValue<bool>(value);
        }

        void EditBool::setValueFromUniform()
        {
            auto uniform = mUniform.lock();

            if (!uniform)
                return;

            setValue(uniform->template getValue<bool>());
        }

        void EditBool::toDefault()
        {
            auto uniform = mUniform.lock();

            if (!uniform)
                return;

            setValue(uniform->getDefault<bool>());
        }

        void EditBool::initialiseOverride()
        {
            Base::initialiseOverride();

            assignWidget(mCheckbutton, "Checkbutton");
            assignWidget(mFill, "Fill");

            mCheckbutton->eventMouseButtonClick += MyGUI::newDelegate(this, &EditBool::notifyMouseButtonClick);
        }


        void EditBool::notifyMouseButtonClick(MyGUI::Widget* sender)
        {
            auto uniform = mUniform.lock();

            if (!uniform)
                return;

            setValue(!uniform->getValue<bool>());
        }

        void UniformBase::init(const std::shared_ptr<fx::Types::UniformBase>& uniform)
        {
            mLabel->setCaption(uniform->mDisplayName.empty() ? uniform->mName : uniform->mDisplayName);

            if (uniform->mDescription.empty())
            {
                mLabel->setUserString("ToolTipType", "");
            }
            else
            {
                mLabel->setUserString("ToolTipType", "Layout");
                mLabel->setUserString("ToolTipLayout", "TextToolTip");
                mLabel->setUserString("Caption_Text", uniform->mDescription);
            }

            std::visit([this, &uniform](auto&& arg) {
                using T = typename std::decay_t<decltype(arg)>::value_type;

                if constexpr (std::is_same_v<osg::Vec4f, T>)
                {
                    createVectorWidget<T, EditNumberFloat4>(uniform, mClient, this);
                }
                else if constexpr (std::is_same_v<osg::Vec3f, T>)
                {
                    createVectorWidget<T, EditNumberFloat3>(uniform, mClient, this);
                }
                else if constexpr (std::is_same_v<osg::Vec2f, T>)
                {
                    createVectorWidget<T, EditNumberFloat2>(uniform, mClient, this);
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    auto* widget = mClient->createWidget<EditNumberFloat>("MW_ValueEditNumber", {0, 0, mClient->getWidth(), mClient->getHeight()}, MyGUI::Align::Stretch);
                    widget->setData(uniform);
                    mBases.emplace_back(widget);
                }
                else if constexpr (std::is_same_v<T, int>)
                {
                    auto* widget = mClient->createWidget<EditNumberInt>("MW_ValueEditNumber", {0, 0, mClient->getWidth(), mClient->getHeight()}, MyGUI::Align::Stretch);
                    widget->setData(uniform);
                    mBases.emplace_back(widget);
                }
                else if constexpr (std::is_same_v<T, bool>)
                {
                    auto* widget = mClient->createWidget<EditBool>("MW_ValueEditBool", {0, 0, mClient->getWidth(), mClient->getHeight()}, MyGUI::Align::Stretch);
                    widget->setData(uniform);
                    mBases.emplace_back(widget);
                }

                mReset->eventMouseButtonClick += MyGUI::newDelegate(this, &UniformBase::notifyResetClicked);

                for (EditBase* base : mBases)
                    base->setValueFromUniform();

            }, uniform->mData);
        }

        void UniformBase::addItem(EditBase* item)
        {
            mBases.emplace_back(item);
        }

        void UniformBase::toDefault()
        {
            for (EditBase* base : mBases)
            {
                if (base)
                    base->toDefault();
            }
        }

        void UniformBase::notifyResetClicked(MyGUI::Widget* sender)
        {
            toDefault();
        }

        void UniformBase::initialiseOverride()
        {
            Base::initialiseOverride();

            assignWidget(mReset, "Reset");
            assignWidget(mLabel, "Label");
            assignWidget(mClient, "Client");
        }
    }
}