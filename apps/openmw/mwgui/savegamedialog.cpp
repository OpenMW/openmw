#include "savegamedialog.hpp"

#include <iomanip>
#include <sstream>

#include <MyGUI_ComboBox.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_LanguageManager.h>
#include <MyGUI_UString.h>

#include <osg/Texture2D>
#include <osgDB/ReadFile>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/files/conversion.hpp>
#include <components/files/memorystream.hpp>
#include <components/l10n/manager.hpp>
#include <components/misc/strings/format.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/misc/timeconvert.hpp>
#include <components/myguiplatform/myguitexture.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/datetimemanager.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/timestamp.hpp"

#include "../mwstate/character.hpp"

#include "confirmationdialog.hpp"

namespace MWGui
{
    SaveGameDialog::SaveGameDialog()
        : WindowModal("openmw_savegame_dialog.layout")
        , mSaving(true)
        , mCurrentCharacter(nullptr)
        , mCurrentSlot(nullptr)
    {
        getWidget(mScreenshot, "Screenshot");
        getWidget(mCharacterSelection, "SelectCharacter");
        getWidget(mCellName, "CellName");
        getWidget(mInfoText, "InfoText");
        getWidget(mOkButton, "OkButton");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mDeleteButton, "DeleteButton");
        getWidget(mSaveList, "SaveList");
        getWidget(mSaveNameEdit, "SaveNameEdit");
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SaveGameDialog::onOkButtonClicked);
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SaveGameDialog::onCancelButtonClicked);
        mDeleteButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SaveGameDialog::onDeleteButtonClicked);
        mCharacterSelection->eventComboChangePosition += MyGUI::newDelegate(this, &SaveGameDialog::onCharacterSelected);
        mCharacterSelection->eventComboAccept += MyGUI::newDelegate(this, &SaveGameDialog::onCharacterAccept);
        mSaveList->eventListChangePosition += MyGUI::newDelegate(this, &SaveGameDialog::onSlotSelected);
        mSaveList->eventListMouseItemActivate += MyGUI::newDelegate(this, &SaveGameDialog::onSlotMouseClick);
        mSaveList->eventListSelectAccept += MyGUI::newDelegate(this, &SaveGameDialog::onSlotActivated);
        mSaveList->eventKeyButtonPressed += MyGUI::newDelegate(this, &SaveGameDialog::onKeyButtonPressed);
        mSaveNameEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &SaveGameDialog::onEditSelectAccept);
        mSaveNameEdit->eventEditTextChange += MyGUI::newDelegate(this, &SaveGameDialog::onSaveNameChanged);

        // To avoid accidental deletions
        mDeleteButton->setNeedKeyFocus(false);

        mControllerButtons.mA = "#{sSelect}";
        mControllerButtons.mB = "#{Interface:Cancel}";
    }

    void SaveGameDialog::onSlotActivated(MyGUI::ListBox* sender, size_t pos)
    {
        onSlotSelected(sender, pos);
        accept();
    }

    void SaveGameDialog::onSlotMouseClick(MyGUI::ListBox* sender, size_t pos)
    {
        onSlotSelected(sender, pos);

        if (pos != MyGUI::ITEM_NONE && MyGUI::InputManager::getInstance().isShiftPressed())
            confirmDeleteSave();
    }

    void SaveGameDialog::confirmDeleteSave()
    {
        ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
        dialog->askForConfirmation("#{OMWEngine:DeleteGameConfirmation}");
        dialog->eventOkClicked.clear();
        dialog->eventOkClicked += MyGUI::newDelegate(this, &SaveGameDialog::onDeleteSlotConfirmed);
        dialog->eventCancelClicked.clear();
        dialog->eventCancelClicked += MyGUI::newDelegate(this, &SaveGameDialog::onDeleteSlotCancel);
    }

    void SaveGameDialog::onDeleteSlotConfirmed()
    {
        MWBase::Environment::get().getStateManager()->deleteGame(mCurrentCharacter, mCurrentSlot);
        mSaveList->removeItemAt(mSaveList->getIndexSelected());
        onSlotSelected(mSaveList, mSaveList->getIndexSelected());
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveList);

        if (mSaveList->getItemCount() == 0)
        {
            size_t previousIndex = mCharacterSelection->getIndexSelected();
            mCurrentCharacter = nullptr;
            mCharacterSelection->removeItemAt(previousIndex);
            if (mCharacterSelection->getItemCount())
            {
                size_t nextCharacter = std::min(previousIndex, mCharacterSelection->getItemCount() - 1);
                mCharacterSelection->setIndexSelected(nextCharacter);
                onCharacterSelected(mCharacterSelection, nextCharacter);
            }
            else
                mCharacterSelection->setIndexSelected(MyGUI::ITEM_NONE);
        }
    }

    void SaveGameDialog::onDeleteSlotCancel()
    {
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveList);
    }

    void SaveGameDialog::onSaveNameChanged(MyGUI::EditBox* sender)
    {
        // This might have previously been a save slot from the list. If so, that is no longer the case
        mSaveList->setIndexSelected(MyGUI::ITEM_NONE);
        onSlotSelected(mSaveList, MyGUI::ITEM_NONE);
    }

    void SaveGameDialog::onEditSelectAccept(MyGUI::EditBox* sender)
    {
        accept();

        // To do not spam onEditSelectAccept() again and again
        MWBase::Environment::get().getWindowManager()->injectKeyRelease(MyGUI::KeyCode::None);
    }

    void SaveGameDialog::onClose()
    {
        mSaveList->setIndexSelected(MyGUI::ITEM_NONE);

        WindowModal::onClose();
    }

    void SaveGameDialog::onOpen()
    {
        WindowModal::onOpen();

        mSaveNameEdit->setCaption({});
        if (Settings::gui().mControllerMenus && mSaving)
        {
            // For controller mode, set a default save file name. The format is
            // "Day 24 - Last Steed 7 p.m."
            const MWWorld::DateTimeManager& timeManager = *MWBase::Environment::get().getWorld()->getTimeManager();
            std::string_view month = timeManager.getMonthName();
            int hour = static_cast<int>(timeManager.getTimeStamp().getHour());
            bool pm = hour >= 12;
            if (hour >= 13)
                hour -= 12;
            if (hour == 0)
                hour = 12;

            ESM::EpochTimeStamp currentDate = timeManager.getEpochTimeStamp();
            std::string daysPassed
                = Misc::StringUtils::format("#{Calendar:day} %i", timeManager.getTimeStamp().getDay());
            std::string_view formattedHour(pm ? "#{Calendar:pm}" : "#{Calendar:am}");
            std::string autoFilename = Misc::StringUtils::format(
                "%s - %i %s %i %s", daysPassed, currentDate.mDay, month, hour, formattedHour);

            mSaveNameEdit->setCaptionWithReplacing(autoFilename);
        }
        if (mSaving)
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveNameEdit);
        else
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveList);

        center();

        mCharacterSelection->setCaption({});
        mCharacterSelection->removeAllItems();
        mCurrentCharacter = nullptr;
        mCurrentSlot = nullptr;
        mSaveList->removeAllItems();
        onSlotSelected(mSaveList, MyGUI::ITEM_NONE);

        if (Settings::gui().mControllerMenus)
        {
            mOkButtonFocus = true;
            mOkButton->setStateSelected(true);
            mCancelButton->setStateSelected(false);
        }

        MWBase::StateManager* mgr = MWBase::Environment::get().getStateManager();
        if (mgr->characterBegin() == mgr->characterEnd())
            return;

        mCurrentCharacter = mgr->getCurrentCharacter();

        const std::string& directory = Settings::saves().mCharacter;

        size_t selectedIndex = MyGUI::ITEM_NONE;

        for (MWBase::StateManager::CharacterIterator it = mgr->characterBegin(); it != mgr->characterEnd(); ++it)
        {
            if (it->begin() != it->end())
            {
                const ESM::SavedGame& signature = it->getSignature();

                std::stringstream title;
                title << signature.mPlayerName;

                // For a custom class, we will not find it in the store (unless we loaded the savegame first).
                // Fall back to name stored in savegame header in that case.
                std::string_view className;
                if (signature.mPlayerClassId.empty())
                    className = signature.mPlayerClassName;
                else
                {
                    // Find the localised name for this class from the store
                    const ESM::Class* class_
                        = MWBase::Environment::get().getESMStore()->get<ESM::Class>().search(signature.mPlayerClassId);
                    if (class_)
                        className = class_->mName;
                    else
                        className = "?"; // From an older savegame format that did not support custom classes properly.
                }

                title << " (#{OMWEngine:Level} " << signature.mPlayerLevel << " "
                      << MyGUI::TextIterator::toTagsString(MyGUI::UString(className)) << ")";

                const MyGUI::UString playerDesc = MyGUI::LanguageManager::getInstance().replaceTags(title.str());
                mCharacterSelection->addItem(playerDesc, &*it);

                if (mCurrentCharacter == &*it
                    || (!mCurrentCharacter && !mSaving
                        && Misc::StringUtils::ciEqual(directory, Files::pathToUnicodeString(it->getPath().filename()))))
                {
                    mCurrentCharacter = &*it;
                    selectedIndex = mCharacterSelection->getItemCount() - 1;
                }
            }
        }

        if (selectedIndex == MyGUI::ITEM_NONE && !mSaving && mCharacterSelection->getItemCount() != 0)
        {
            selectedIndex = 0;
            mCurrentCharacter = *mCharacterSelection->getItemDataAt<const MWState::Character*>(0);
        }
        mCharacterSelection->setIndexSelected(selectedIndex);
        if (selectedIndex == MyGUI::ITEM_NONE)
            mCharacterSelection->setCaptionWithReplacing("#{OMWEngine:SelectCharacter}");

        fillSaveList();
    }

    void SaveGameDialog::setLoadOrSave(bool load)
    {
        mSaving = !load;
        mSaveNameEdit->setVisible(!load);
        mCharacterSelection->setUserString("Hidden", load ? "false" : "true");
        mCharacterSelection->setVisible(load);

        mDeleteButton->setUserString("Hidden", load ? "false" : "true");
        mDeleteButton->setVisible(load);

        if (!load)
        {
            mCurrentCharacter = MWBase::Environment::get().getStateManager()->getCurrentCharacter();
        }

        center();
    }

    void SaveGameDialog::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        setVisible(false);
    }

    void SaveGameDialog::onDeleteButtonClicked(MyGUI::Widget* sender)
    {
        if (mCurrentSlot)
            confirmDeleteSave();
    }

    void SaveGameDialog::onConfirmationGiven()
    {
        accept(true);
    }

    void SaveGameDialog::onConfirmationCancel()
    {
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveList);
    }

    void SaveGameDialog::accept(bool reallySure)
    {
        if (mSaving)
        {
            // If overwriting an existing slot, ask for confirmation first
            if (mCurrentSlot != nullptr && !reallySure)
            {
                ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
                dialog->askForConfirmation("#{OMWEngine:OverwriteGameConfirmation}");
                dialog->eventOkClicked.clear();
                dialog->eventOkClicked += MyGUI::newDelegate(this, &SaveGameDialog::onConfirmationGiven);
                dialog->eventCancelClicked.clear();
                dialog->eventCancelClicked += MyGUI::newDelegate(this, &SaveGameDialog::onConfirmationCancel);
                return;
            }
            if (mSaveNameEdit->getCaption().empty())
            {
                MWBase::Environment::get().getWindowManager()->messageBox("#{OMWEngine:EmptySaveNameError}");
                return;
            }
        }
        else
        {
            MWBase::StateManager::State state = MWBase::Environment::get().getStateManager()->getState();

            // If game is running, ask for confirmation first
            if (state == MWBase::StateManager::State_Running && !reallySure)
            {
                ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
                dialog->askForConfirmation("#{OMWEngine:LoadGameConfirmation}");
                dialog->eventOkClicked.clear();
                dialog->eventOkClicked += MyGUI::newDelegate(this, &SaveGameDialog::onConfirmationGiven);
                dialog->eventCancelClicked.clear();
                dialog->eventCancelClicked += MyGUI::newDelegate(this, &SaveGameDialog::onConfirmationCancel);
                return;
            }
        }

        setVisible(false);
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_MainMenu);

        if (mSaving)
        {
            MWBase::Environment::get().getStateManager()->saveGame(mSaveNameEdit->getCaption(), mCurrentSlot);
        }
        else
        {
            assert(mCurrentCharacter && mCurrentSlot);
            MWBase::Environment::get().getStateManager()->loadGame(mCurrentCharacter, mCurrentSlot->mPath);
        }
    }

    void SaveGameDialog::onKeyButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        if (key == MyGUI::KeyCode::Delete && mCurrentSlot)
            confirmDeleteSave();
    }

    void SaveGameDialog::onOkButtonClicked(MyGUI::Widget* sender)
    {
        accept();
    }

    void SaveGameDialog::onCharacterSelected(MyGUI::ComboBox* sender, size_t pos)
    {
        const MWState::Character* character = *mCharacterSelection->getItemDataAt<const MWState::Character*>(pos);

        mCurrentCharacter = character;
        mCurrentSlot = nullptr;
        fillSaveList();
    }

    void SaveGameDialog::onCharacterAccept(MyGUI::ComboBox* sender, size_t pos)
    {
        // Give key focus to save list so we can confirm the selection with Enter
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveList);
    }

    void SaveGameDialog::fillSaveList()
    {
        mSaveList->removeAllItems();
        if (!mCurrentCharacter)
            return;
        for (MWState::Character::SlotIterator it = mCurrentCharacter->begin(); it != mCurrentCharacter->end(); ++it)
        {
            mSaveList->addItem(it->mProfile.mDescription);
        }
        // When loading, Auto-select the first save, if there is one
        if (mSaveList->getItemCount() && !mSaving)
        {
            mSaveList->setIndexSelected(0);
            onSlotSelected(mSaveList, 0);
        }
        else
            onSlotSelected(mSaveList, MyGUI::ITEM_NONE);
    }

    std::string formatTimeplayed(const double timeInSeconds)
    {
        auto l10n = MWBase::Environment::get().getL10nManager()->getContext("Interface");
        int duration = static_cast<int>(timeInSeconds);
        if (duration <= 0)
            return l10n->formatMessage("DurationSecond", { "seconds" }, { 0 });

        std::string result;
        int hours = duration / 3600;
        int minutes = (duration / 60) % 60;
        int seconds = duration % 60;
        if (hours)
            result += l10n->formatMessage("DurationHour", { "hours" }, { hours });
        if (minutes)
            result += l10n->formatMessage("DurationMinute", { "minutes" }, { minutes });
        if (seconds)
            result += l10n->formatMessage("DurationSecond", { "seconds" }, { seconds });

        return result;
    }

    void SaveGameDialog::onSlotSelected(MyGUI::ListBox* sender, size_t pos)
    {
        mOkButton->setEnabled(pos != MyGUI::ITEM_NONE || mSaving);
        mDeleteButton->setEnabled(pos != MyGUI::ITEM_NONE);

        if (pos == MyGUI::ITEM_NONE || !mCurrentCharacter)
        {
            mCurrentSlot = nullptr;
            mCellName->setCaption({});
            mInfoText->setCaption({});
            mScreenshot->setImageTexture({});
            return;
        }

        if (mSaving)
            mSaveNameEdit->setCaption(sender->getItemNameAt(pos));

        mCurrentSlot = nullptr;
        size_t i = 0;
        for (MWState::Character::SlotIterator it = mCurrentCharacter->begin(); it != mCurrentCharacter->end();
             ++it, ++i)
        {
            if (i == pos)
                mCurrentSlot = &*it;
        }
        if (!mCurrentSlot)
            throw std::runtime_error("Can't find selected slot");

        std::stringstream text;

        const size_t profileIndex = mCharacterSelection->getIndexSelected();
        const std::string& slotPlayerName = mCurrentSlot->mProfile.mPlayerName;
        const ESM::SavedGame& profileSavedGame
            = (*mCharacterSelection->getItemDataAt<const MWState::Character*>(profileIndex))->getSignature();
        if (slotPlayerName != profileSavedGame.mPlayerName)
            text << slotPlayerName << "\n";

        text << "#{OMWEngine:Level} " << mCurrentSlot->mProfile.mPlayerLevel << "\n";

        if (mCurrentSlot->mProfile.mCurrentDay > 0)
            text << "#{Calendar:day} " << mCurrentSlot->mProfile.mCurrentDay << "\n";

        if (mCurrentSlot->mProfile.mMaximumHealth > 0)
            text << "#{OMWEngine:Health} " << static_cast<int>(mCurrentSlot->mProfile.mCurrentHealth) << "/"
                 << static_cast<int>(mCurrentSlot->mProfile.mMaximumHealth) << "\n";

        int hour = int(mCurrentSlot->mProfile.mInGameTime.mGameHour);
        bool pm = hour >= 12;
        if (hour >= 13)
            hour -= 12;
        if (hour == 0)
            hour = 12;

        text << mCurrentSlot->mProfile.mInGameTime.mDay << " "
             << MWBase::Environment::get().getWorld()->getTimeManager()->getMonthName(
                    mCurrentSlot->mProfile.mInGameTime.mMonth)
             << " " << hour << " " << (pm ? "#{Calendar:pm}" : "#{Calendar:am}") << "\n";

        if (mCurrentSlot->mProfile.mTimePlayed > 0)
        {
            text << "#{OMWEngine:TimePlayed}: " << formatTimeplayed(mCurrentSlot->mProfile.mTimePlayed) << "\n";
        }

        text << Misc::fileTimeToString(mCurrentSlot->mTimeStamp, "%Y.%m.%d %T") << "\n";

        mCellName->setCaptionWithReplacing("#{sCell=" + mCurrentSlot->mProfile.mPlayerCellName + "}");
        mInfoText->setCaptionWithReplacing(text.str());

        // Reset the image for the case we're unable to recover a screenshot
        mScreenshotTexture.reset();
        mScreenshot->setRenderItemTexture(nullptr);
        mScreenshot->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 0.f, 1.f, 1.f));

        // Decode screenshot
        const std::vector<char>& data = mCurrentSlot->mProfile.mScreenshot;
        if (!data.size())
        {
            Log(Debug::Warning) << "Selected save file '" << Files::pathToUnicodeString(mCurrentSlot->mPath.filename())
                                << "' has no savegame screenshot";
            return;
        }

        Files::IMemStream instream(data.data(), data.size());

        osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension("jpg");
        if (!readerwriter)
        {
            Log(Debug::Error) << "Can't open savegame screenshot, no jpg readerwriter found";
            return;
        }

        osgDB::ReaderWriter::ReadResult result = readerwriter->readImage(instream);
        if (!result.success())
        {
            Log(Debug::Error) << "Failed to read savegame screenshot: " << result.message() << " code "
                              << result.status();
            return;
        }

        osg::ref_ptr<osg::Texture2D> texture(new osg::Texture2D);
        texture->setImage(result.getImage());
        texture->setInternalFormat(GL_RGB);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        texture->setResizeNonPowerOfTwoHint(false);
        texture->setUnRefImageDataAfterApply(true);

        mScreenshotTexture = std::make_unique<MyGUIPlatform::OSGTexture>(texture);
        mScreenshot->setRenderItemTexture(mScreenshotTexture.get());
    }

    ControllerButtons* SaveGameDialog::getControllerButtons()
    {
        mControllerButtons.mY = mSaving ? "" : "#{OMWEngine:LoadingSelectCharacter}";
        return &mControllerButtons;
    }

    bool SaveGameDialog::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            if (mOkButtonFocus)
                onOkButtonClicked(mOkButton);
            else
                onCancelButtonClicked(mCancelButton);
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            onCancelButtonClicked(mCancelButton);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_Y)
        {
            size_t index = mCharacterSelection->getIndexSelected();
            index = wrap(index + 1, mCharacterSelection->getItemCount());
            mCharacterSelection->setIndexSelected(index);
            onCharacterSelected(mCharacterSelection, index);
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        {
            MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
            winMgr->setKeyFocusWidget(mSaveList);
            winMgr->injectKeyPress(MyGUI::KeyCode::ArrowUp, 0, false);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        {
            MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
            winMgr->setKeyFocusWidget(mSaveList);
            winMgr->injectKeyPress(MyGUI::KeyCode::ArrowDown, 0, false);
        }
        else if ((arg.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT && !mOkButtonFocus)
            || (arg.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT && mOkButtonFocus))
        {
            mOkButtonFocus = !mOkButtonFocus;
            mOkButton->setStateSelected(mOkButtonFocus);
            mCancelButton->setStateSelected(!mOkButtonFocus);
        }

        return true;
    }
}
