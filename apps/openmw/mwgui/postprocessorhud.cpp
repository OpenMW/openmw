#include "postprocessorhud.hpp"

#include <MyGUI_Align.h>
#include <MyGUI_Button.h>
#include <MyGUI_Delegate.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_EventPair.h>
#include <MyGUI_FactoryManager.h>
#include <MyGUI_Gui.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_Macros.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_TabItem.h>
#include <MyGUI_UString.h>
#include <MyGUI_Widget.h>
#include <MyGUI_WidgetDefines.h>
#include <MyGUI_WidgetInput.h>
#include <MyGUI_Window.h>

#include <components/files/configurationmanager.hpp>
#include <components/fx/technique.hpp>
#include <components/fx/widgets.hpp>

#include <components/misc/strings/algorithm.hpp>
#include <components/misc/utf8stream.hpp>

#include <components/widgets/box.hpp>

#include "../mwrender/postprocessor.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

namespace MWGui
{
    namespace
    {
        std::shared_ptr<fx::Technique>& getTechnique(const MyGUI::ListBox& list, size_t selected)
        {
            return *list.getItemDataAt<std::shared_ptr<fx::Technique>>(selected);
        }
    }

    void PostProcessorHud::ListWrapper::onKeyButtonPressed(MyGUI::KeyCode key, MyGUI::Char ch)
    {
        if (MyGUI::InputManager::getInstance().isShiftPressed()
            && (key == MyGUI::KeyCode::ArrowUp || key == MyGUI::KeyCode::ArrowDown))
            return;

        MyGUI::ListBox::onKeyButtonPressed(key, ch);
    }

    PostProcessorHud::PostProcessorHud(Files::ConfigurationManager& cfgMgr)
        : WindowBase("openmw_postprocessor_hud.layout")
        , mCfgMgr(cfgMgr)
    {
        getWidget(mActiveList, "ActiveList");
        getWidget(mInactiveList, "InactiveList");
        getWidget(mConfigLayout, "ConfigLayout");
        getWidget(mFilter, "Filter");
        getWidget(mButtonActivate, "ButtonActivate");
        getWidget(mButtonDeactivate, "ButtonDeactivate");
        getWidget(mButtonUp, "ButtonUp");
        getWidget(mButtonDown, "ButtonDown");

        mButtonActivate->eventMouseButtonClick += MyGUI::newDelegate(this, &PostProcessorHud::notifyActivatePressed);
        mButtonDeactivate->eventMouseButtonClick
            += MyGUI::newDelegate(this, &PostProcessorHud::notifyDeactivatePressed);
        mButtonUp->eventMouseButtonClick += MyGUI::newDelegate(this, &PostProcessorHud::notifyShaderUpPressed);
        mButtonDown->eventMouseButtonClick += MyGUI::newDelegate(this, &PostProcessorHud::notifyShaderDownPressed);

        mActiveList->eventKeyButtonPressed += MyGUI::newDelegate(this, &PostProcessorHud::notifyKeyButtonPressed);
        mInactiveList->eventKeyButtonPressed += MyGUI::newDelegate(this, &PostProcessorHud::notifyKeyButtonPressed);

        mActiveList->eventListChangePosition += MyGUI::newDelegate(this, &PostProcessorHud::notifyListChangePosition);
        mInactiveList->eventListChangePosition += MyGUI::newDelegate(this, &PostProcessorHud::notifyListChangePosition);

        mFilter->eventEditTextChange += MyGUI::newDelegate(this, &PostProcessorHud::notifyFilterChanged);

        mMainWidget->castType<MyGUI::Window>()->eventWindowChangeCoord
            += MyGUI::newDelegate(this, &PostProcessorHud::notifyWindowResize);

        mShaderInfo = mConfigLayout->createWidget<Gui::AutoSizedEditBox>("HeaderText", {}, MyGUI::Align::Default);
        mShaderInfo->setUserString("VStretch", "true");
        mShaderInfo->setUserString("HStretch", "true");
        mShaderInfo->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        mShaderInfo->setEditReadOnly(true);
        mShaderInfo->setEditWordWrap(true);
        mShaderInfo->setEditMultiLine(true);
        mShaderInfo->setNeedMouseFocus(false);

        mConfigLayout->setVisibleVScroll(true);

        mConfigArea = mConfigLayout->createWidget<MyGUI::Widget>({}, {}, MyGUI::Align::Default);

        mConfigLayout->eventMouseWheel += MyGUI::newDelegate(this, &PostProcessorHud::notifyMouseWheel);
        mConfigArea->eventMouseWheel += MyGUI::newDelegate(this, &PostProcessorHud::notifyMouseWheel);
    }

    void PostProcessorHud::notifyFilterChanged(MyGUI::EditBox* sender)
    {
        updateTechniques();
    }

    void PostProcessorHud::notifyWindowResize(MyGUI::Window* sender)
    {
        layout();
    }

    void PostProcessorHud::notifyResetButtonClicked(MyGUI::Widget* sender)
    {
        for (size_t i = 1; i < mConfigArea->getChildCount(); ++i)
        {
            if (auto* child = dynamic_cast<fx::Widgets::UniformBase*>(mConfigArea->getChildAt(i)))
                child->toDefault();
        }
    }

    void PostProcessorHud::notifyListChangePosition(MyGUI::ListBox* sender, size_t index)
    {
        if (sender == mActiveList)
            mInactiveList->clearIndexSelected();
        else if (sender == mInactiveList)
            mActiveList->clearIndexSelected();

        if (index >= sender->getItemCount())
            return;

        updateConfigView(getTechnique(*sender, index)->getFileName());
    }

    void PostProcessorHud::toggleTechnique(bool enabled)
    {
        auto* list = enabled ? mInactiveList : mActiveList;

        size_t selected = list->getIndexSelected();

        if (selected != MyGUI::ITEM_NONE)
        {
            auto* processor = MWBase::Environment::get().getWorld()->getPostProcessor();
            mOverrideHint = list->getItemNameAt(selected);

            auto technique = getTechnique(*list, selected);
            if (technique->getDynamic())
                return;

            if (enabled)
                processor->enableTechnique(std::move(technique));
            else
                processor->disableTechnique(std::move(technique));
            processor->saveChain();
        }
    }

    void PostProcessorHud::notifyActivatePressed(MyGUI::Widget* sender)
    {
        toggleTechnique(true);
    }

    void PostProcessorHud::notifyDeactivatePressed(MyGUI::Widget* sender)
    {
        toggleTechnique(false);
    }

    void PostProcessorHud::moveShader(Direction direction)
    {
        auto* processor = MWBase::Environment::get().getWorld()->getPostProcessor();

        size_t selected = mActiveList->getIndexSelected();

        if (selected == MyGUI::ITEM_NONE)
            return;

        int index = direction == Direction::Up ? static_cast<int>(selected) - 1 : selected + 1;
        index = std::clamp<int>(index, 0, mActiveList->getItemCount() - 1);

        if (static_cast<size_t>(index) != selected)
        {
            auto technique = getTechnique(*mActiveList, selected);
            if (technique->getDynamic() || technique->getInternal())
                return;

            if (processor->enableTechnique(std::move(technique), index - mOffset)
                != MWRender::PostProcessor::Status_Error)
                processor->saveChain();
        }
    }

    void PostProcessorHud::notifyShaderUpPressed(MyGUI::Widget* sender)
    {
        moveShader(Direction::Up);
    }

    void PostProcessorHud::notifyShaderDownPressed(MyGUI::Widget* sender)
    {
        moveShader(Direction::Down);
    }

    void PostProcessorHud::notifyKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch)
    {
        MyGUI::ListBox* list = static_cast<MyGUI::ListBox*>(sender);

        if (list->getIndexSelected() == MyGUI::ITEM_NONE)
            return;

        if (key == MyGUI::KeyCode::ArrowLeft && list == mActiveList)
        {
            if (MyGUI::InputManager::getInstance().isShiftPressed())
            {
                toggleTechnique(false);
            }
            else
            {
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mInactiveList);
                mActiveList->clearIndexSelected();
                select(mInactiveList, 0);
            }
        }
        else if (key == MyGUI::KeyCode::ArrowRight && list == mInactiveList)
        {
            if (MyGUI::InputManager::getInstance().isShiftPressed())
            {
                toggleTechnique(true);
            }
            else
            {
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mActiveList);
                mInactiveList->clearIndexSelected();
                select(mActiveList, 0);
            }
        }
        else if (list == mActiveList && MyGUI::InputManager::getInstance().isShiftPressed()
            && (key == MyGUI::KeyCode::ArrowUp || key == MyGUI::KeyCode::ArrowDown))
        {
            moveShader(key == MyGUI::KeyCode::ArrowUp ? Direction::Up : Direction::Down);
        }
    }

    void PostProcessorHud::onOpen()
    {
        toggleMode(Settings::ShaderManager::Mode::Debug);
        updateTechniques();
    }

    void PostProcessorHud::onClose()
    {
        Settings::ShaderManager::get().save();
        Settings::Manager::saveUser(mCfgMgr.getUserConfigPath() / "settings.cfg");
        toggleMode(Settings::ShaderManager::Mode::Normal);
    }

    void PostProcessorHud::layout()
    {
        constexpr int padding = 12;
        constexpr int padding2 = padding * 2;
        mShaderInfo->setCoord(
            padding, padding, mConfigLayout->getSize().width - padding2 - padding, mShaderInfo->getTextSize().height);

        int totalHeight = mShaderInfo->getTop() + mShaderInfo->getTextSize().height + padding;

        mConfigArea->setCoord({ padding, totalHeight, mShaderInfo->getSize().width, mConfigLayout->getHeight() });

        int childHeights = 0;
        MyGUI::EnumeratorWidgetPtr enumerator = mConfigArea->getEnumerator();
        while (enumerator.next())
        {
            enumerator.current()->setCoord(padding, childHeights + padding, mShaderInfo->getSize().width - padding2,
                enumerator.current()->getHeight());
            childHeights += enumerator.current()->getHeight() + padding;
        }
        totalHeight += childHeights;

        mConfigArea->setSize(mConfigArea->getWidth(), childHeights);

        mConfigLayout->setCanvasSize(mConfigLayout->getWidth() - padding2, totalHeight);
        mConfigLayout->setSize(mConfigLayout->getWidth(), mConfigLayout->getParentSize().height - padding2);
    }

    void PostProcessorHud::notifyMouseWheel(MyGUI::Widget* sender, int rel)
    {
        int offset = mConfigLayout->getViewOffset().top + rel * 0.3;
        if (offset > 0)
            mConfigLayout->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mConfigLayout->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(offset)));
    }

    void PostProcessorHud::select(ListWrapper* list, size_t index)
    {
        list->setIndexSelected(index);
        notifyListChangePosition(list, index);
    }

    void PostProcessorHud::toggleMode(Settings::ShaderManager::Mode mode)
    {
        Settings::ShaderManager::get().setMode(mode);

        MWBase::Environment::get().getWorld()->getPostProcessor()->toggleMode();

        if (!isVisible())
            return;

        if (mInactiveList->getIndexSelected() != MyGUI::ITEM_NONE)
            updateConfigView(getTechnique(*mInactiveList, mInactiveList->getIndexSelected())->getFileName());
        else if (mActiveList->getIndexSelected() != MyGUI::ITEM_NONE)
            updateConfigView(getTechnique(*mActiveList, mActiveList->getIndexSelected())->getFileName());
    }

    void PostProcessorHud::updateConfigView(VFS::Path::NormalizedView path)
    {
        auto* processor = MWBase::Environment::get().getWorld()->getPostProcessor();

        auto technique = processor->loadTechnique(path);

        if (technique->getStatus() == fx::Technique::Status::File_Not_exists)
            return;

        while (mConfigArea->getChildCount() > 0)
            MyGUI::Gui::getInstance().destroyWidget(mConfigArea->getChildAt(0));

        mShaderInfo->setCaption({});

        std::ostringstream ss;

        const std::string_view NA = "#{Interface:NotAvailableShort}";
        const char endl = '\n';

        std::string_view author = technique->getAuthor().empty() ? NA : technique->getAuthor();
        std::string_view version = technique->getVersion().empty() ? NA : technique->getVersion();
        std::string_view description = technique->getDescription().empty() ? NA : technique->getDescription();

        auto serializeBool = [](bool value) { return value ? "#{Interface:Yes}" : "#{Interface:No}"; };

        const auto flags = technique->getFlags();

        const auto flag_interior = serializeBool(!(flags & fx::Technique::Flag_Disable_Interiors));
        const auto flag_exterior = serializeBool(!(flags & fx::Technique::Flag_Disable_Exteriors));
        const auto flag_underwater = serializeBool(!(flags & fx::Technique::Flag_Disable_Underwater));
        const auto flag_abovewater = serializeBool(!(flags & fx::Technique::Flag_Disable_Abovewater));

        switch (technique->getStatus())
        {
            case fx::Technique::Status::Success:
            case fx::Technique::Status::Uncompiled:
            {
                if (technique->getDynamic())
                    ss << "#{fontcolourhtml=header}#{OMWShaders:ShaderLocked}:      #{fontcolourhtml=normal} "
                          "#{OMWShaders:ShaderLockedDescription}"
                       << endl
                       << endl;
                ss << "#{fontcolourhtml=header}#{OMWShaders:Author}:      #{fontcolourhtml=normal} " << author << endl
                   << endl
                   << "#{fontcolourhtml=header}#{OMWShaders:Version}:     #{fontcolourhtml=normal} " << version << endl
                   << endl
                   << "#{fontcolourhtml=header}#{OMWShaders:Description}: #{fontcolourhtml=normal} " << description
                   << endl
                   << endl
                   << "#{fontcolourhtml=header}#{OMWShaders:InInteriors}: #{fontcolourhtml=normal} " << flag_interior
                   << "#{fontcolourhtml=header}   #{OMWShaders:InExteriors}: #{fontcolourhtml=normal} " << flag_exterior
                   << "#{fontcolourhtml=header}   #{OMWShaders:Underwater}: #{fontcolourhtml=normal} "
                   << flag_underwater
                   << "#{fontcolourhtml=header}   #{OMWShaders:Abovewater}: #{fontcolourhtml=normal} "
                   << flag_abovewater;
                break;
            }
            case fx::Technique::Status::Parse_Error:
                ss << "#{fontcolourhtml=negative}Shader Compile Error: #{fontcolourhtml=normal} <"
                   << std::string(technique->getName()) << "> failed to compile." << endl
                   << endl
                   << technique->getLastError();
                break;
            case fx::Technique::Status::File_Not_exists:
                break;
        }

        mShaderInfo->setCaptionWithReplacing(ss.str());

        if (Settings::ShaderManager::get().getMode() == Settings::ShaderManager::Mode::Debug)
        {
            if (technique->getUniformMap().size() > 0)
            {
                MyGUI::Button* resetButton
                    = mConfigArea->createWidget<MyGUI::Button>("MW_Button", { 0, 0, 0, 24 }, MyGUI::Align::Default);
                resetButton->setCaptionWithReplacing("#{OMWShaders:ResetShader}");
                resetButton->setTextAlign(MyGUI::Align::Center);
                resetButton->eventMouseWheel += MyGUI::newDelegate(this, &PostProcessorHud::notifyMouseWheel);
                resetButton->eventMouseButtonClick
                    += MyGUI::newDelegate(this, &PostProcessorHud::notifyResetButtonClicked);
            }

            for (const auto& uniform : technique->getUniformMap())
            {
                if (!uniform->mStatic || uniform->mSamplerType)
                    continue;

                if (!uniform->mHeader.empty())
                {
                    Gui::AutoSizedTextBox* divider = mConfigArea->createWidget<Gui::AutoSizedTextBox>(
                        "MW_UniformGroup", { 0, 0, 0, 34 }, MyGUI::Align::Default);
                    divider->setNeedMouseFocus(false);
                    divider->setCaptionWithReplacing(uniform->mHeader);
                }

                fx::Widgets::UniformBase* uwidget = mConfigArea->createWidget<fx::Widgets::UniformBase>(
                    "MW_UniformEdit", { 0, 0, 0, 22 }, MyGUI::Align::Default);
                uwidget->init(uniform);
                uwidget->getLabel()->eventMouseWheel += MyGUI::newDelegate(this, &PostProcessorHud::notifyMouseWheel);
            }
        }

        layout();
    }

    void PostProcessorHud::updateTechniques()
    {
        if (!isVisible())
            return;

        std::string hint;
        ListWrapper* hintWidget = nullptr;
        if (mInactiveList->getIndexSelected() != MyGUI::ITEM_NONE)
        {
            hint = mInactiveList->getItemNameAt(mInactiveList->getIndexSelected());
            hintWidget = mInactiveList;
        }
        else if (mActiveList->getIndexSelected() != MyGUI::ITEM_NONE)
        {
            hint = mActiveList->getItemNameAt(mActiveList->getIndexSelected());
            hintWidget = mActiveList;
        }

        mInactiveList->removeAllItems();
        mActiveList->removeAllItems();

        auto* processor = MWBase::Environment::get().getWorld()->getPostProcessor();

        std::vector<VFS::Path::NormalizedView> techniques;
        for (const auto& vfsPath : processor->getTechniqueFiles())
            techniques.emplace_back(vfsPath);
        std::sort(techniques.begin(), techniques.end());

        for (VFS::Path::NormalizedView path : techniques)
        {
            auto technique = processor->loadTechnique(path);

            if (!technique->getHidden() && !processor->isTechniqueEnabled(technique))
            {
                std::string lowerName = Utf8Stream::lowerCaseUtf8(technique->getName());
                std::string lowerCaption = mFilter->getCaption();
                lowerCaption = Utf8Stream::lowerCaseUtf8(lowerCaption);
                if (lowerName.find(lowerCaption) != std::string::npos)
                    mInactiveList->addItem(technique->getName(), technique);
            }
        }

        mOffset = 0;
        for (auto technique : processor->getTechniques())
        {
            if (!technique->getHidden())
            {
                mActiveList->addItem(technique->getName(), technique);

                if (technique->getInternal())
                    mOffset++;
            }
        }

        auto tryFocus = [this](ListWrapper* widget, const std::string& hint) {
            MyGUI::Widget* oldFocus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
            if (oldFocus == mFilter)
                return;
            size_t index = widget->findItemIndexWith(hint);

            if (index != MyGUI::ITEM_NONE)
            {
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(widget);
                select(widget, index);
            }
        };

        if (!mOverrideHint.empty())
        {
            tryFocus(mActiveList, mOverrideHint);
            tryFocus(mInactiveList, mOverrideHint);

            mOverrideHint.clear();
        }
        else if (hintWidget && !hint.empty())
            tryFocus(hintWidget, hint);
    }

    void PostProcessorHud::registerMyGUIComponents()
    {
        MyGUI::FactoryManager& factory = MyGUI::FactoryManager::getInstance();
        factory.registerFactory<fx::Widgets::UniformBase>("Widget");
        factory.registerFactory<fx::Widgets::EditNumberFloat4>("Widget");
        factory.registerFactory<fx::Widgets::EditNumberFloat3>("Widget");
        factory.registerFactory<fx::Widgets::EditNumberFloat2>("Widget");
        factory.registerFactory<fx::Widgets::EditNumberFloat>("Widget");
        factory.registerFactory<fx::Widgets::EditNumberInt>("Widget");
        factory.registerFactory<fx::Widgets::EditBool>("Widget");
        factory.registerFactory<fx::Widgets::EditChoice>("Widget");
        factory.registerFactory<ListWrapper>("Widget");
    }
}
