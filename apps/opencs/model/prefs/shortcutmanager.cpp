#include "shortcutmanager.hpp"

#include <algorithm>

#include <QApplication>
#include <QStringList>

#include "shortcut.hpp"
#include "shortcuteventhandler.hpp"

namespace CSMPrefs
{
    ShortcutManager::ShortcutManager()
    {
        createLookupTables();
        mEventHandler = new ShortcutEventHandler(this);
    }

    void ShortcutManager::addShortcut(Shortcut* shortcut)
    {
        mShortcuts.insert(std::make_pair(shortcut->getName(), shortcut));
        mShortcuts.insert(std::make_pair(shortcut->getModifierName(), shortcut));
        mEventHandler->addShortcut(shortcut);
    }

    void ShortcutManager::removeShortcut(Shortcut* shortcut)
    {
        std::pair<ShortcutMap::iterator, ShortcutMap::iterator> range = mShortcuts.equal_range(shortcut->getName());

        for (ShortcutMap::iterator it = range.first; it != range.second;)
        {
            if (it->second == shortcut)
            {
                mShortcuts.erase(it++);
            }
            else
            {
                ++it;
            }
        }

        mEventHandler->removeShortcut(shortcut);
    }

    bool ShortcutManager::getSequence(const std::string& name, QKeySequence& sequence) const
    {
        SequenceMap::const_iterator item = mSequences.find(name);
        if (item != mSequences.end())
        {
            sequence = item->second;

            return true;
        }
        else
            return false;
    }

    void ShortcutManager::setSequence(const std::string& name, const QKeySequence& sequence)
    {
        // Add to map/modify
        SequenceMap::iterator item = mSequences.find(name);
        if (item != mSequences.end())
        {
            item->second = sequence;
        }
        else
        {
            mSequences.insert(std::make_pair(name, sequence));
        }

        // Change active shortcuts
        std::pair<ShortcutMap::iterator, ShortcutMap::iterator> rangeS = mShortcuts.equal_range(name);

        for (ShortcutMap::iterator it = rangeS.first; it != rangeS.second; ++it)
        {
            it->second->setSequence(sequence);
        }
    }

    bool ShortcutManager::getModifier(const std::string& name, int& modifier) const
    {
        ModifierMap::const_iterator item = mModifiers.find(name);
        if (item != mModifiers.end())
        {
            modifier = item->second;

            return true;
        }
        else
            return false;
    }

    void ShortcutManager::setModifier(const std::string& name, int modifier)
    {
        // Add to map/modify
        ModifierMap::iterator item = mModifiers.find(name);
        if (item != mModifiers.end())
        {
            item->second = modifier;
        }
        else
        {
            mModifiers.insert(std::make_pair(name, modifier));
        }

        // Change active shortcuts
        std::pair<ShortcutMap::iterator, ShortcutMap::iterator> rangeS = mShortcuts.equal_range(name);

        for (ShortcutMap::iterator it = rangeS.first; it != rangeS.second; ++it)
        {
            it->second->setModifier(modifier);
        }
    }

    std::string ShortcutManager::convertToString(const QKeySequence& sequence) const
    {
        const int MouseKeyMask = 0x01FFFFFF;
        const int ModMask = 0x7E000000;

        std::string result;

        for (int i = 0; i < (int)sequence.count(); ++i)
        {
            int mods = sequence[i] & ModMask;
            int key = sequence[i] & MouseKeyMask;

            if (key)
            {
                NameMap::const_iterator searchResult = mNames.find(key);
                if (searchResult != mNames.end())
                {
                    if (mods && i == 0)
                    {
                        if (mods & Qt::ControlModifier)
                            result.append("Ctrl+");
                        if (mods & Qt::ShiftModifier)
                            result.append("Shift+");
                        if (mods & Qt::AltModifier)
                            result.append("Alt+");
                        if (mods & Qt::MetaModifier)
                            result.append("Meta+");
                        if (mods & Qt::KeypadModifier)
                            result.append("Keypad+");
                        if (mods & Qt::GroupSwitchModifier)
                            result.append("GroupSwitch+");
                    }
                    else if (i > 0)
                    {
                        result.append("+");
                    }

                    result.append(searchResult->second);
                }
            }
        }

        return result;
    }

    std::string ShortcutManager::convertToString(int modifier) const
    {
        NameMap::const_iterator searchResult = mNames.find(modifier);
        if (searchResult != mNames.end())
        {
            return searchResult->second;
        }
        else
            return "";
    }

    std::string ShortcutManager::convertToString(const QKeySequence& sequence, int modifier) const
    {
        std::string concat = convertToString(sequence) + ";" + convertToString(modifier);
        return concat;
    }

    void ShortcutManager::convertFromString(const std::string& data, QKeySequence& sequence) const
    {
        const int MaxKeys = 4; // A limitation of QKeySequence

        size_t end = data.find(';');
        size_t size = std::min(end, data.size());

        std::string value = data.substr(0, size);
        size_t start = 0;

        int keyPos = 0;
        int mods = 0;

        int keys[MaxKeys] = {};

        while (start < value.size())
        {
            end = data.find('+', start);
            end = std::min(end, value.size());

            std::string name = value.substr(start, end - start);

            if (name == "Ctrl")
            {
                mods |= Qt::ControlModifier;
            }
            else if (name == "Shift")
            {
                mods |= Qt::ShiftModifier;
            }
            else if (name == "Alt")
            {
                mods |= Qt::AltModifier;
            }
            else if (name == "Meta")
            {
                mods |= Qt::MetaModifier;
            }
            else if (name == "Keypad")
            {
                mods |= Qt::KeypadModifier;
            }
            else if (name == "GroupSwitch")
            {
                mods |= Qt::GroupSwitchModifier;
            }
            else
            {
                KeyMap::const_iterator searchResult = mKeys.find(name);
                if (searchResult != mKeys.end())
                {
                    keys[keyPos] = mods | searchResult->second;

                    mods = 0;
                    keyPos += 1;

                    if (keyPos >= MaxKeys)
                        break;
                }
            }

            start = end + 1;
        }

        sequence = QKeySequence(keys[0], keys[1], keys[2], keys[3]);
    }

    void ShortcutManager::convertFromString(const std::string& data, int& modifier) const
    {
        size_t start = data.find(';') + 1;
        start = std::min(start, data.size());

        std::string name = data.substr(start);
        KeyMap::const_iterator searchResult = mKeys.find(name);
        if (searchResult != mKeys.end())
        {
            modifier = searchResult->second;
        }
        else
        {
            modifier = 0;
        }
    }

    void ShortcutManager::convertFromString(const std::string& data, QKeySequence& sequence, int& modifier) const
    {
        convertFromString(data, sequence);
        convertFromString(data, modifier);
    }

    void ShortcutManager::createLookupTables()
    {
        // Mouse buttons
        mNames.insert(std::make_pair(Qt::LeftButton, "LMB"));
        mNames.insert(std::make_pair(Qt::RightButton, "RMB"));
        mNames.insert(std::make_pair(Qt::MiddleButton, "MMB"));
        mNames.insert(std::make_pair(Qt::XButton1, "Mouse4"));
        mNames.insert(std::make_pair(Qt::XButton2, "Mouse5"));

        // Keyboard buttons
        for (size_t i = 0; QtKeys[i].first != 0; ++i)
        {
            mNames.insert(QtKeys[i]);
        }

        // Generate inverse map
        for (NameMap::const_iterator it = mNames.begin(); it != mNames.end(); ++it)
        {
            mKeys.insert(std::make_pair(it->second, it->first));
        }
    }

    QString ShortcutManager::processToolTip(const QString& toolTip) const
    {
        const QChar SequenceStart = '{';
        const QChar SequenceEnd = '}';

        QStringList substrings;

        int prevIndex = 0;
        int startIndex = toolTip.indexOf(SequenceStart);
        int endIndex = (startIndex != -1) ? toolTip.indexOf(SequenceEnd, startIndex) : -1;

        // Process every valid shortcut escape sequence
        while (startIndex != -1 && endIndex != -1)
        {
            int count = startIndex - prevIndex;
            if (count > 0)
            {
                substrings.push_back(toolTip.mid(prevIndex, count));
            }

            // Find sequence name
            startIndex += 1; // '{' character
            count = endIndex - startIndex;
            if (count > 0)
            {
                QString settingName = toolTip.mid(startIndex, count);

                QKeySequence sequence;
                int modifier;

                if (getSequence(settingName.toUtf8().data(), sequence))
                {
                    QString value = QString::fromUtf8(convertToString(sequence).c_str());
                    substrings.push_back(value);
                }
                else if (getModifier(settingName.toUtf8().data(), modifier))
                {
                    QString value = QString::fromUtf8(convertToString(modifier).c_str());
                    substrings.push_back(value);
                }

                prevIndex = endIndex + 1; // '}' character
            }

            startIndex = toolTip.indexOf(SequenceStart, endIndex);
            endIndex = (startIndex != -1) ? toolTip.indexOf(SequenceEnd, startIndex) : -1;
        }

        if (prevIndex < toolTip.size())
        {
            substrings.push_back(toolTip.mid(prevIndex));
        }

        return substrings.join("");
    }

    const std::pair<int, const char*> ShortcutManager::QtKeys[] =
    {
        std::make_pair((int)Qt::Key_Space                  , "Space"),
        std::make_pair((int)Qt::Key_Exclam                 , "Exclam"),
        std::make_pair((int)Qt::Key_QuoteDbl               , "QuoteDbl"),
        std::make_pair((int)Qt::Key_NumberSign             , "NumberSign"),
        std::make_pair((int)Qt::Key_Dollar                 , "Dollar"),
        std::make_pair((int)Qt::Key_Percent                , "Percent"),
        std::make_pair((int)Qt::Key_Ampersand              , "Ampersand"),
        std::make_pair((int)Qt::Key_Apostrophe             , "Apostrophe"),
        std::make_pair((int)Qt::Key_ParenLeft              , "ParenLeft"),
        std::make_pair((int)Qt::Key_ParenRight             , "ParenRight"),
        std::make_pair((int)Qt::Key_Asterisk               , "Asterisk"),
        std::make_pair((int)Qt::Key_Plus                   , "Plus"),
        std::make_pair((int)Qt::Key_Comma                  , "Comma"),
        std::make_pair((int)Qt::Key_Minus                  , "Minus"),
        std::make_pair((int)Qt::Key_Period                 , "Period"),
        std::make_pair((int)Qt::Key_Slash                  , "Slash"),
        std::make_pair((int)Qt::Key_0                      , "0"),
        std::make_pair((int)Qt::Key_1                      , "1"),
        std::make_pair((int)Qt::Key_2                      , "2"),
        std::make_pair((int)Qt::Key_3                      , "3"),
        std::make_pair((int)Qt::Key_4                      , "4"),
        std::make_pair((int)Qt::Key_5                      , "5"),
        std::make_pair((int)Qt::Key_6                      , "6"),
        std::make_pair((int)Qt::Key_7                      , "7"),
        std::make_pair((int)Qt::Key_8                      , "8"),
        std::make_pair((int)Qt::Key_9                      , "9"),
        std::make_pair((int)Qt::Key_Colon                  , "Colon"),
        std::make_pair((int)Qt::Key_Semicolon              , "Semicolon"),
        std::make_pair((int)Qt::Key_Less                   , "Less"),
        std::make_pair((int)Qt::Key_Equal                  , "Equal"),
        std::make_pair((int)Qt::Key_Greater                , "Greater"),
        std::make_pair((int)Qt::Key_Question               , "Question"),
        std::make_pair((int)Qt::Key_At                     , "At"),
        std::make_pair((int)Qt::Key_A                      , "A"),
        std::make_pair((int)Qt::Key_B                      , "B"),
        std::make_pair((int)Qt::Key_C                      , "C"),
        std::make_pair((int)Qt::Key_D                      , "D"),
        std::make_pair((int)Qt::Key_E                      , "E"),
        std::make_pair((int)Qt::Key_F                      , "F"),
        std::make_pair((int)Qt::Key_G                      , "G"),
        std::make_pair((int)Qt::Key_H                      , "H"),
        std::make_pair((int)Qt::Key_I                      , "I"),
        std::make_pair((int)Qt::Key_J                      , "J"),
        std::make_pair((int)Qt::Key_K                      , "K"),
        std::make_pair((int)Qt::Key_L                      , "L"),
        std::make_pair((int)Qt::Key_M                      , "M"),
        std::make_pair((int)Qt::Key_N                      , "N"),
        std::make_pair((int)Qt::Key_O                      , "O"),
        std::make_pair((int)Qt::Key_P                      , "P"),
        std::make_pair((int)Qt::Key_Q                      , "Q"),
        std::make_pair((int)Qt::Key_R                      , "R"),
        std::make_pair((int)Qt::Key_S                      , "S"),
        std::make_pair((int)Qt::Key_T                      , "T"),
        std::make_pair((int)Qt::Key_U                      , "U"),
        std::make_pair((int)Qt::Key_V                      , "V"),
        std::make_pair((int)Qt::Key_W                      , "W"),
        std::make_pair((int)Qt::Key_X                      , "X"),
        std::make_pair((int)Qt::Key_Y                      , "Y"),
        std::make_pair((int)Qt::Key_Z                      , "Z"),
        std::make_pair((int)Qt::Key_BracketLeft            , "BracketLeft"),
        std::make_pair((int)Qt::Key_Backslash              , "Backslash"),
        std::make_pair((int)Qt::Key_BracketRight           , "BracketRight"),
        std::make_pair((int)Qt::Key_AsciiCircum            , "AsciiCircum"),
        std::make_pair((int)Qt::Key_Underscore             , "Underscore"),
        std::make_pair((int)Qt::Key_QuoteLeft              , "QuoteLeft"),
        std::make_pair((int)Qt::Key_BraceLeft              , "BraceLeft"),
        std::make_pair((int)Qt::Key_Bar                    , "Bar"),
        std::make_pair((int)Qt::Key_BraceRight             , "BraceRight"),
        std::make_pair((int)Qt::Key_AsciiTilde             , "AsciiTilde"),
        std::make_pair((int)Qt::Key_nobreakspace           , "nobreakspace"),
        std::make_pair((int)Qt::Key_exclamdown             , "exclamdown"),
        std::make_pair((int)Qt::Key_cent                   , "cent"),
        std::make_pair((int)Qt::Key_sterling               , "sterling"),
        std::make_pair((int)Qt::Key_currency               , "currency"),
        std::make_pair((int)Qt::Key_yen                    , "yen"),
        std::make_pair((int)Qt::Key_brokenbar              , "brokenbar"),
        std::make_pair((int)Qt::Key_section                , "section"),
        std::make_pair((int)Qt::Key_diaeresis              , "diaeresis"),
        std::make_pair((int)Qt::Key_copyright              , "copyright"),
        std::make_pair((int)Qt::Key_ordfeminine            , "ordfeminine"),
        std::make_pair((int)Qt::Key_guillemotleft          , "guillemotleft"),
        std::make_pair((int)Qt::Key_notsign                , "notsign"),
        std::make_pair((int)Qt::Key_hyphen                 , "hyphen"),
        std::make_pair((int)Qt::Key_registered             , "registered"),
        std::make_pair((int)Qt::Key_macron                 , "macron"),
        std::make_pair((int)Qt::Key_degree                 , "degree"),
        std::make_pair((int)Qt::Key_plusminus              , "plusminus"),
        std::make_pair((int)Qt::Key_twosuperior            , "twosuperior"),
        std::make_pair((int)Qt::Key_threesuperior          , "threesuperior"),
        std::make_pair((int)Qt::Key_acute                  , "acute"),
        std::make_pair((int)Qt::Key_mu                     , "mu"),
        std::make_pair((int)Qt::Key_paragraph              , "paragraph"),
        std::make_pair((int)Qt::Key_periodcentered         , "periodcentered"),
        std::make_pair((int)Qt::Key_cedilla                , "cedilla"),
        std::make_pair((int)Qt::Key_onesuperior            , "onesuperior"),
        std::make_pair((int)Qt::Key_masculine              , "masculine"),
        std::make_pair((int)Qt::Key_guillemotright         , "guillemotright"),
        std::make_pair((int)Qt::Key_onequarter             , "onequarter"),
        std::make_pair((int)Qt::Key_onehalf                , "onehalf"),
        std::make_pair((int)Qt::Key_threequarters          , "threequarters"),
        std::make_pair((int)Qt::Key_questiondown           , "questiondown"),
        std::make_pair((int)Qt::Key_Agrave                 , "Agrave"),
        std::make_pair((int)Qt::Key_Aacute                 , "Aacute"),
        std::make_pair((int)Qt::Key_Acircumflex            , "Acircumflex"),
        std::make_pair((int)Qt::Key_Atilde                 , "Atilde"),
        std::make_pair((int)Qt::Key_Adiaeresis             , "Adiaeresis"),
        std::make_pair((int)Qt::Key_Aring                  , "Aring"),
        std::make_pair((int)Qt::Key_AE                     , "AE"),
        std::make_pair((int)Qt::Key_Ccedilla               , "Ccedilla"),
        std::make_pair((int)Qt::Key_Egrave                 , "Egrave"),
        std::make_pair((int)Qt::Key_Eacute                 , "Eacute"),
        std::make_pair((int)Qt::Key_Ecircumflex            , "Ecircumflex"),
        std::make_pair((int)Qt::Key_Ediaeresis             , "Ediaeresis"),
        std::make_pair((int)Qt::Key_Igrave                 , "Igrave"),
        std::make_pair((int)Qt::Key_Iacute                 , "Iacute"),
        std::make_pair((int)Qt::Key_Icircumflex            , "Icircumflex"),
        std::make_pair((int)Qt::Key_Idiaeresis             , "Idiaeresis"),
        std::make_pair((int)Qt::Key_ETH                    , "ETH"),
        std::make_pair((int)Qt::Key_Ntilde                 , "Ntilde"),
        std::make_pair((int)Qt::Key_Ograve                 , "Ograve"),
        std::make_pair((int)Qt::Key_Oacute                 , "Oacute"),
        std::make_pair((int)Qt::Key_Ocircumflex            , "Ocircumflex"),
        std::make_pair((int)Qt::Key_Otilde                 , "Otilde"),
        std::make_pair((int)Qt::Key_Odiaeresis             , "Odiaeresis"),
        std::make_pair((int)Qt::Key_multiply               , "multiply"),
        std::make_pair((int)Qt::Key_Ooblique               , "Ooblique"),
        std::make_pair((int)Qt::Key_Ugrave                 , "Ugrave"),
        std::make_pair((int)Qt::Key_Uacute                 , "Uacute"),
        std::make_pair((int)Qt::Key_Ucircumflex            , "Ucircumflex"),
        std::make_pair((int)Qt::Key_Udiaeresis             , "Udiaeresis"),
        std::make_pair((int)Qt::Key_Yacute                 , "Yacute"),
        std::make_pair((int)Qt::Key_THORN                  , "THORN"),
        std::make_pair((int)Qt::Key_ssharp                 , "ssharp"),
        std::make_pair((int)Qt::Key_division               , "division"),
        std::make_pair((int)Qt::Key_ydiaeresis             , "ydiaeresis"),
        std::make_pair((int)Qt::Key_Escape                 , "Escape"),
        std::make_pair((int)Qt::Key_Tab                    , "Tab"),
        std::make_pair((int)Qt::Key_Backtab                , "Backtab"),
        std::make_pair((int)Qt::Key_Backspace              , "Backspace"),
        std::make_pair((int)Qt::Key_Return                 , "Return"),
        std::make_pair((int)Qt::Key_Enter                  , "Enter"),
        std::make_pair((int)Qt::Key_Insert                 , "Insert"),
        std::make_pair((int)Qt::Key_Delete                 , "Delete"),
        std::make_pair((int)Qt::Key_Pause                  , "Pause"),
        std::make_pair((int)Qt::Key_Print                  , "Print"),
        std::make_pair((int)Qt::Key_SysReq                 , "SysReq"),
        std::make_pair((int)Qt::Key_Clear                  , "Clear"),
        std::make_pair((int)Qt::Key_Home                   , "Home"),
        std::make_pair((int)Qt::Key_End                    , "End"),
        std::make_pair((int)Qt::Key_Left                   , "Left"),
        std::make_pair((int)Qt::Key_Up                     , "Up"),
        std::make_pair((int)Qt::Key_Right                  , "Right"),
        std::make_pair((int)Qt::Key_Down                   , "Down"),
        std::make_pair((int)Qt::Key_PageUp                 , "PageUp"),
        std::make_pair((int)Qt::Key_PageDown               , "PageDown"),
        std::make_pair((int)Qt::Key_Shift                  , "Shift"),
        std::make_pair((int)Qt::Key_Control                , "Control"),
        std::make_pair((int)Qt::Key_Meta                   , "Meta"),
        std::make_pair((int)Qt::Key_Alt                    , "Alt"),
        std::make_pair((int)Qt::Key_CapsLock               , "CapsLock"),
        std::make_pair((int)Qt::Key_NumLock                , "NumLock"),
        std::make_pair((int)Qt::Key_ScrollLock             , "ScrollLock"),
        std::make_pair((int)Qt::Key_F1                     , "F1"),
        std::make_pair((int)Qt::Key_F2                     , "F2"),
        std::make_pair((int)Qt::Key_F3                     , "F3"),
        std::make_pair((int)Qt::Key_F4                     , "F4"),
        std::make_pair((int)Qt::Key_F5                     , "F5"),
        std::make_pair((int)Qt::Key_F6                     , "F6"),
        std::make_pair((int)Qt::Key_F7                     , "F7"),
        std::make_pair((int)Qt::Key_F8                     , "F8"),
        std::make_pair((int)Qt::Key_F9                     , "F9"),
        std::make_pair((int)Qt::Key_F10                    , "F10"),
        std::make_pair((int)Qt::Key_F11                    , "F11"),
        std::make_pair((int)Qt::Key_F12                    , "F12"),
        std::make_pair((int)Qt::Key_F13                    , "F13"),
        std::make_pair((int)Qt::Key_F14                    , "F14"),
        std::make_pair((int)Qt::Key_F15                    , "F15"),
        std::make_pair((int)Qt::Key_F16                    , "F16"),
        std::make_pair((int)Qt::Key_F17                    , "F17"),
        std::make_pair((int)Qt::Key_F18                    , "F18"),
        std::make_pair((int)Qt::Key_F19                    , "F19"),
        std::make_pair((int)Qt::Key_F20                    , "F20"),
        std::make_pair((int)Qt::Key_F21                    , "F21"),
        std::make_pair((int)Qt::Key_F22                    , "F22"),
        std::make_pair((int)Qt::Key_F23                    , "F23"),
        std::make_pair((int)Qt::Key_F24                    , "F24"),
        std::make_pair((int)Qt::Key_F25                    , "F25"),
        std::make_pair((int)Qt::Key_F26                    , "F26"),
        std::make_pair((int)Qt::Key_F27                    , "F27"),
        std::make_pair((int)Qt::Key_F28                    , "F28"),
        std::make_pair((int)Qt::Key_F29                    , "F29"),
        std::make_pair((int)Qt::Key_F30                    , "F30"),
        std::make_pair((int)Qt::Key_F31                    , "F31"),
        std::make_pair((int)Qt::Key_F32                    , "F32"),
        std::make_pair((int)Qt::Key_F33                    , "F33"),
        std::make_pair((int)Qt::Key_F34                    , "F34"),
        std::make_pair((int)Qt::Key_F35                    , "F35"),
        std::make_pair((int)Qt::Key_Super_L                , "Super_L"),
        std::make_pair((int)Qt::Key_Super_R                , "Super_R"),
        std::make_pair((int)Qt::Key_Menu                   , "Menu"),
        std::make_pair((int)Qt::Key_Hyper_L                , "Hyper_L"),
        std::make_pair((int)Qt::Key_Hyper_R                , "Hyper_R"),
        std::make_pair((int)Qt::Key_Help                   , "Help"),
        std::make_pair((int)Qt::Key_Direction_L            , "Direction_L"),
        std::make_pair((int)Qt::Key_Direction_R            , "Direction_R"),
        std::make_pair((int)Qt::Key_Back                   , "Back"),
        std::make_pair((int)Qt::Key_Forward                , "Forward"),
        std::make_pair((int)Qt::Key_Stop                   , "Stop"),
        std::make_pair((int)Qt::Key_Refresh                , "Refresh"),
        std::make_pair((int)Qt::Key_VolumeDown             , "VolumeDown"),
        std::make_pair((int)Qt::Key_VolumeMute             , "VolumeMute"),
        std::make_pair((int)Qt::Key_VolumeUp               , "VolumeUp"),
        std::make_pair((int)Qt::Key_BassBoost              , "BassBoost"),
        std::make_pair((int)Qt::Key_BassUp                 , "BassUp"),
        std::make_pair((int)Qt::Key_BassDown               , "BassDown"),
        std::make_pair((int)Qt::Key_TrebleUp               , "TrebleUp"),
        std::make_pair((int)Qt::Key_TrebleDown             , "TrebleDown"),
        std::make_pair((int)Qt::Key_MediaPlay              , "MediaPlay"),
        std::make_pair((int)Qt::Key_MediaStop              , "MediaStop"),
        std::make_pair((int)Qt::Key_MediaPrevious          , "MediaPrevious"),
        std::make_pair((int)Qt::Key_MediaNext              , "MediaNext"),
        std::make_pair((int)Qt::Key_MediaRecord            , "MediaRecord"),
        std::make_pair((int)Qt::Key_MediaPause             , "MediaPause"),
        std::make_pair((int)Qt::Key_MediaTogglePlayPause   , "MediaTogglePlayPause"),
        std::make_pair((int)Qt::Key_HomePage               , "HomePage"),
        std::make_pair((int)Qt::Key_Favorites              , "Favorites"),
        std::make_pair((int)Qt::Key_Search                 , "Search"),
        std::make_pair((int)Qt::Key_Standby                , "Standby"),
        std::make_pair((int)Qt::Key_OpenUrl                , "OpenUrl"),
        std::make_pair((int)Qt::Key_LaunchMail             , "LaunchMail"),
        std::make_pair((int)Qt::Key_LaunchMedia            , "LaunchMedia"),
        std::make_pair((int)Qt::Key_Launch0                , "Launch0"),
        std::make_pair((int)Qt::Key_Launch1                , "Launch1"),
        std::make_pair((int)Qt::Key_Launch2                , "Launch2"),
        std::make_pair((int)Qt::Key_Launch3                , "Launch3"),
        std::make_pair((int)Qt::Key_Launch4                , "Launch4"),
        std::make_pair((int)Qt::Key_Launch5                , "Launch5"),
        std::make_pair((int)Qt::Key_Launch6                , "Launch6"),
        std::make_pair((int)Qt::Key_Launch7                , "Launch7"),
        std::make_pair((int)Qt::Key_Launch8                , "Launch8"),
        std::make_pair((int)Qt::Key_Launch9                , "Launch9"),
        std::make_pair((int)Qt::Key_LaunchA                , "LaunchA"),
        std::make_pair((int)Qt::Key_LaunchB                , "LaunchB"),
        std::make_pair((int)Qt::Key_LaunchC                , "LaunchC"),
        std::make_pair((int)Qt::Key_LaunchD                , "LaunchD"),
        std::make_pair((int)Qt::Key_LaunchE                , "LaunchE"),
        std::make_pair((int)Qt::Key_LaunchF                , "LaunchF"),
        std::make_pair((int)Qt::Key_MonBrightnessUp        , "MonBrightnessUp"),
        std::make_pair((int)Qt::Key_MonBrightnessDown      , "MonBrightnessDown"),
        std::make_pair((int)Qt::Key_KeyboardLightOnOff     , "KeyboardLightOnOff"),
        std::make_pair((int)Qt::Key_KeyboardBrightnessUp   , "KeyboardBrightnessUp"),
        std::make_pair((int)Qt::Key_KeyboardBrightnessDown , "KeyboardBrightnessDown"),
        std::make_pair((int)Qt::Key_PowerOff               , "PowerOff"),
        std::make_pair((int)Qt::Key_WakeUp                 , "WakeUp"),
        std::make_pair((int)Qt::Key_Eject                  , "Eject"),
        std::make_pair((int)Qt::Key_ScreenSaver            , "ScreenSaver"),
        std::make_pair((int)Qt::Key_WWW                    , "WWW"),
        std::make_pair((int)Qt::Key_Memo                   , "Memo"),
        std::make_pair((int)Qt::Key_LightBulb              , "LightBulb"),
        std::make_pair((int)Qt::Key_Shop                   , "Shop"),
        std::make_pair((int)Qt::Key_History                , "History"),
        std::make_pair((int)Qt::Key_AddFavorite            , "AddFavorite"),
        std::make_pair((int)Qt::Key_HotLinks               , "HotLinks"),
        std::make_pair((int)Qt::Key_BrightnessAdjust       , "BrightnessAdjust"),
        std::make_pair((int)Qt::Key_Finance                , "Finance"),
        std::make_pair((int)Qt::Key_Community              , "Community"),
        std::make_pair((int)Qt::Key_AudioRewind            , "AudioRewind"),
        std::make_pair((int)Qt::Key_BackForward            , "BackForward"),
        std::make_pair((int)Qt::Key_ApplicationLeft        , "ApplicationLeft"),
        std::make_pair((int)Qt::Key_ApplicationRight       , "ApplicationRight"),
        std::make_pair((int)Qt::Key_Book                   , "Book"),
        std::make_pair((int)Qt::Key_CD                     , "CD"),
        std::make_pair((int)Qt::Key_Calculator             , "Calculator"),
        std::make_pair((int)Qt::Key_ToDoList               , "ToDoList"),
        std::make_pair((int)Qt::Key_ClearGrab              , "ClearGrab"),
        std::make_pair((int)Qt::Key_Close                  , "Close"),
        std::make_pair((int)Qt::Key_Copy                   , "Copy"),
        std::make_pair((int)Qt::Key_Cut                    , "Cut"),
        std::make_pair((int)Qt::Key_Display                , "Display"),
        std::make_pair((int)Qt::Key_DOS                    , "DOS"),
        std::make_pair((int)Qt::Key_Documents              , "Documents"),
        std::make_pair((int)Qt::Key_Excel                  , "Excel"),
        std::make_pair((int)Qt::Key_Explorer               , "Explorer"),
        std::make_pair((int)Qt::Key_Game                   , "Game"),
        std::make_pair((int)Qt::Key_Go                     , "Go"),
        std::make_pair((int)Qt::Key_iTouch                 , "iTouch"),
        std::make_pair((int)Qt::Key_LogOff                 , "LogOff"),
        std::make_pair((int)Qt::Key_Market                 , "Market"),
        std::make_pair((int)Qt::Key_Meeting                , "Meeting"),
        std::make_pair((int)Qt::Key_MenuKB                 , "MenuKB"),
        std::make_pair((int)Qt::Key_MenuPB                 , "MenuPB"),
        std::make_pair((int)Qt::Key_MySites                , "MySites"),
        std::make_pair((int)Qt::Key_News                   , "News"),
        std::make_pair((int)Qt::Key_OfficeHome             , "OfficeHome"),
        std::make_pair((int)Qt::Key_Option                 , "Option"),
        std::make_pair((int)Qt::Key_Paste                  , "Paste"),
        std::make_pair((int)Qt::Key_Phone                  , "Phone"),
        std::make_pair((int)Qt::Key_Calendar               , "Calendar"),
        std::make_pair((int)Qt::Key_Reply                  , "Reply"),
        std::make_pair((int)Qt::Key_Reload                 , "Reload"),
        std::make_pair((int)Qt::Key_RotateWindows          , "RotateWindows"),
        std::make_pair((int)Qt::Key_RotationPB             , "RotationPB"),
        std::make_pair((int)Qt::Key_RotationKB             , "RotationKB"),
        std::make_pair((int)Qt::Key_Save                   , "Save"),
        std::make_pair((int)Qt::Key_Send                   , "Send"),
        std::make_pair((int)Qt::Key_Spell                  , "Spell"),
        std::make_pair((int)Qt::Key_SplitScreen            , "SplitScreen"),
        std::make_pair((int)Qt::Key_Support                , "Support"),
        std::make_pair((int)Qt::Key_TaskPane               , "TaskPane"),
        std::make_pair((int)Qt::Key_Terminal               , "Terminal"),
        std::make_pair((int)Qt::Key_Tools                  , "Tools"),
        std::make_pair((int)Qt::Key_Travel                 , "Travel"),
        std::make_pair((int)Qt::Key_Video                  , "Video"),
        std::make_pair((int)Qt::Key_Word                   , "Word"),
        std::make_pair((int)Qt::Key_Xfer                   , "Xfer"),
        std::make_pair((int)Qt::Key_ZoomIn                 , "ZoomIn"),
        std::make_pair((int)Qt::Key_ZoomOut                , "ZoomOut"),
        std::make_pair((int)Qt::Key_Away                   , "Away"),
        std::make_pair((int)Qt::Key_Messenger              , "Messenger"),
        std::make_pair((int)Qt::Key_WebCam                 , "WebCam"),
        std::make_pair((int)Qt::Key_MailForward            , "MailForward"),
        std::make_pair((int)Qt::Key_Pictures               , "Pictures"),
        std::make_pair((int)Qt::Key_Music                  , "Music"),
        std::make_pair((int)Qt::Key_Battery                , "Battery"),
        std::make_pair((int)Qt::Key_Bluetooth              , "Bluetooth"),
        std::make_pair((int)Qt::Key_WLAN                   , "WLAN"),
        std::make_pair((int)Qt::Key_UWB                    , "UWB"),
        std::make_pair((int)Qt::Key_AudioForward           , "AudioForward"),
        std::make_pair((int)Qt::Key_AudioRepeat            , "AudioRepeat"),
        std::make_pair((int)Qt::Key_AudioRandomPlay        , "AudioRandomPlay"),
        std::make_pair((int)Qt::Key_Subtitle               , "Subtitle"),
        std::make_pair((int)Qt::Key_AudioCycleTrack        , "AudioCycleTrack"),
        std::make_pair((int)Qt::Key_Time                   , "Time"),
        std::make_pair((int)Qt::Key_Hibernate              , "Hibernate"),
        std::make_pair((int)Qt::Key_View                   , "View"),
        std::make_pair((int)Qt::Key_TopMenu                , "TopMenu"),
        std::make_pair((int)Qt::Key_PowerDown              , "PowerDown"),
        std::make_pair((int)Qt::Key_Suspend                , "Suspend"),
        std::make_pair((int)Qt::Key_ContrastAdjust         , "ContrastAdjust"),
        std::make_pair((int)Qt::Key_LaunchG                , "LaunchG"),
        std::make_pair((int)Qt::Key_LaunchH                , "LaunchH"),
        std::make_pair((int)Qt::Key_TouchpadToggle         , "TouchpadToggle"),
        std::make_pair((int)Qt::Key_TouchpadOn             , "TouchpadOn"),
        std::make_pair((int)Qt::Key_TouchpadOff            , "TouchpadOff"),
        std::make_pair((int)Qt::Key_MicMute                , "MicMute"),
        std::make_pair((int)Qt::Key_Red                    , "Red"),
        std::make_pair((int)Qt::Key_Green                  , "Green"),
        std::make_pair((int)Qt::Key_Yellow                 , "Yellow"),
        std::make_pair((int)Qt::Key_Blue                   , "Blue"),
        std::make_pair((int)Qt::Key_ChannelUp              , "ChannelUp"),
        std::make_pair((int)Qt::Key_ChannelDown            , "ChannelDown"),
        std::make_pair((int)Qt::Key_Guide                  , "Guide"),
        std::make_pair((int)Qt::Key_Info                   , "Info"),
        std::make_pair((int)Qt::Key_Settings               , "Settings"),
        std::make_pair((int)Qt::Key_MicVolumeUp            , "MicVolumeUp"),
        std::make_pair((int)Qt::Key_MicVolumeDown          , "MicVolumeDown"),
        std::make_pair((int)Qt::Key_New                    , "New"),
        std::make_pair((int)Qt::Key_Open                   , "Open"),
        std::make_pair((int)Qt::Key_Find                   , "Find"),
        std::make_pair((int)Qt::Key_Undo                   , "Undo"),
        std::make_pair((int)Qt::Key_Redo                   , "Redo"),
        std::make_pair((int)Qt::Key_AltGr                  , "AltGr"),
        std::make_pair((int)Qt::Key_Multi_key              , "Multi_key"),
        std::make_pair((int)Qt::Key_Kanji                  , "Kanji"),
        std::make_pair((int)Qt::Key_Muhenkan               , "Muhenkan"),
        std::make_pair((int)Qt::Key_Henkan                 , "Henkan"),
        std::make_pair((int)Qt::Key_Romaji                 , "Romaji"),
        std::make_pair((int)Qt::Key_Hiragana               , "Hiragana"),
        std::make_pair((int)Qt::Key_Katakana               , "Katakana"),
        std::make_pair((int)Qt::Key_Hiragana_Katakana      , "Hiragana_Katakana"),
        std::make_pair((int)Qt::Key_Zenkaku                , "Zenkaku"),
        std::make_pair((int)Qt::Key_Hankaku                , "Hankaku"),
        std::make_pair((int)Qt::Key_Zenkaku_Hankaku        , "Zenkaku_Hankaku"),
        std::make_pair((int)Qt::Key_Touroku                , "Touroku"),
        std::make_pair((int)Qt::Key_Massyo                 , "Massyo"),
        std::make_pair((int)Qt::Key_Kana_Lock              , "Kana_Lock"),
        std::make_pair((int)Qt::Key_Kana_Shift             , "Kana_Shift"),
        std::make_pair((int)Qt::Key_Eisu_Shift             , "Eisu_Shift"),
        std::make_pair((int)Qt::Key_Eisu_toggle            , "Eisu_toggle"),
        std::make_pair((int)Qt::Key_Hangul                 , "Hangul"),
        std::make_pair((int)Qt::Key_Hangul_Start           , "Hangul_Start"),
        std::make_pair((int)Qt::Key_Hangul_End             , "Hangul_End"),
        std::make_pair((int)Qt::Key_Hangul_Hanja           , "Hangul_Hanja"),
        std::make_pair((int)Qt::Key_Hangul_Jamo            , "Hangul_Jamo"),
        std::make_pair((int)Qt::Key_Hangul_Romaja          , "Hangul_Romaja"),
        std::make_pair((int)Qt::Key_Codeinput              , "Codeinput"),
        std::make_pair((int)Qt::Key_Hangul_Jeonja          , "Hangul_Jeonja"),
        std::make_pair((int)Qt::Key_Hangul_Banja           , "Hangul_Banja"),
        std::make_pair((int)Qt::Key_Hangul_PreHanja        , "Hangul_PreHanja"),
        std::make_pair((int)Qt::Key_Hangul_PostHanja       , "Hangul_PostHanja"),
        std::make_pair((int)Qt::Key_SingleCandidate        , "SingleCandidate"),
        std::make_pair((int)Qt::Key_MultipleCandidate      , "MultipleCandidate"),
        std::make_pair((int)Qt::Key_PreviousCandidate      , "PreviousCandidate"),
        std::make_pair((int)Qt::Key_Hangul_Special         , "Hangul_Special"),
        std::make_pair((int)Qt::Key_Mode_switch            , "Mode_switch"),
        std::make_pair((int)Qt::Key_Dead_Grave             , "Dead_Grave"),
        std::make_pair((int)Qt::Key_Dead_Acute             , "Dead_Acute"),
        std::make_pair((int)Qt::Key_Dead_Circumflex        , "Dead_Circumflex"),
        std::make_pair((int)Qt::Key_Dead_Tilde             , "Dead_Tilde"),
        std::make_pair((int)Qt::Key_Dead_Macron            , "Dead_Macron"),
        std::make_pair((int)Qt::Key_Dead_Breve             , "Dead_Breve"),
        std::make_pair((int)Qt::Key_Dead_Abovedot          , "Dead_Abovedot"),
        std::make_pair((int)Qt::Key_Dead_Diaeresis         , "Dead_Diaeresis"),
        std::make_pair((int)Qt::Key_Dead_Abovering         , "Dead_Abovering"),
        std::make_pair((int)Qt::Key_Dead_Doubleacute       , "Dead_Doubleacute"),
        std::make_pair((int)Qt::Key_Dead_Caron             , "Dead_Caron"),
        std::make_pair((int)Qt::Key_Dead_Cedilla           , "Dead_Cedilla"),
        std::make_pair((int)Qt::Key_Dead_Ogonek            , "Dead_Ogonek"),
        std::make_pair((int)Qt::Key_Dead_Iota              , "Dead_Iota"),
        std::make_pair((int)Qt::Key_Dead_Voiced_Sound      , "Dead_Voiced_Sound"),
        std::make_pair((int)Qt::Key_Dead_Semivoiced_Sound  , "Dead_Semivoiced_Sound"),
        std::make_pair((int)Qt::Key_Dead_Belowdot          , "Dead_Belowdot"),
        std::make_pair((int)Qt::Key_Dead_Hook              , "Dead_Hook"),
        std::make_pair((int)Qt::Key_Dead_Horn              , "Dead_Horn"),
        std::make_pair((int)Qt::Key_MediaLast              , "MediaLast"),
        std::make_pair((int)Qt::Key_Select                 , "Select"),
        std::make_pair((int)Qt::Key_Yes                    , "Yes"),
        std::make_pair((int)Qt::Key_No                     , "No"),
        std::make_pair((int)Qt::Key_Cancel                 , "Cancel"),
        std::make_pair((int)Qt::Key_Printer                , "Printer"),
        std::make_pair((int)Qt::Key_Execute                , "Execute"),
        std::make_pair((int)Qt::Key_Sleep                  , "Sleep"),
        std::make_pair((int)Qt::Key_Play                   , "Play"),
        std::make_pair((int)Qt::Key_Zoom                   , "Zoom"),
        std::make_pair((int)Qt::Key_Exit                   , "Exit"),
        std::make_pair((int)Qt::Key_Context1               , "Context1"),
        std::make_pair((int)Qt::Key_Context2               , "Context2"),
        std::make_pair((int)Qt::Key_Context3               , "Context3"),
        std::make_pair((int)Qt::Key_Context4               , "Context4"),
        std::make_pair((int)Qt::Key_Call                   , "Call"),
        std::make_pair((int)Qt::Key_Hangup                 , "Hangup"),
        std::make_pair((int)Qt::Key_Flip                   , "Flip"),
        std::make_pair((int)Qt::Key_ToggleCallHangup       , "ToggleCallHangup"),
        std::make_pair((int)Qt::Key_VoiceDial              , "VoiceDial"),
        std::make_pair((int)Qt::Key_LastNumberRedial       , "LastNumberRedial"),
        std::make_pair((int)Qt::Key_Camera                 , "Camera"),
        std::make_pair((int)Qt::Key_CameraFocus            , "CameraFocus"),
        std::make_pair(0                                   , (const char*) nullptr)
    };

}
