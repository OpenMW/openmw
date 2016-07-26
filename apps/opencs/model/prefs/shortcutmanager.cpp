#include "shortcutmanager.hpp"

#include <sstream>
#include <iostream>

#include <QApplication>
#include <QMetaEnum>
#include <QRegExp>
#include <QStringList>

#include "shortcut.hpp"
#include "shortcuteventhandler.hpp"

namespace CSMPrefs
{
    ShortcutManager::ShortcutManager()
    {
        mEventHandler = new ShortcutEventHandler(this);
    }

    void ShortcutManager::addShortcut(Shortcut* shortcut)
    {
        mShortcuts.insert(std::make_pair(shortcut->getName(), shortcut));
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

    ShortcutManager::SequenceData ShortcutManager::getSequence(const std::string& name) const
    {
        SequenceData data;
        SequenceMap::const_iterator item = mSequences.find(name);

        if (item != mSequences.end())
        {
            data = item->second;
        }

        return data;
    }

    void ShortcutManager::setSequence(const std::string& name, const SequenceData& data)
    {
        // Add to map/modify
        SequenceMap::iterator item = mSequences.find(name);

        if (item != mSequences.end())
        {
            item->second = data;
        }
        else
        {
            mSequences.insert(std::make_pair(name, data));
        }

        // Change active shortcuts
        std::pair<ShortcutMap::iterator, ShortcutMap::iterator> rangeS = mShortcuts.equal_range(name);

        for (ShortcutMap::iterator it = rangeS.first; it != rangeS.second; ++it)
        {
            it->second->setSequence(data.first);
            it->second->setModifier(data.second);
        }
    }

    std::string ShortcutManager::sequenceToString(const SequenceData& data)
    {
        const int MouseMask = 0x0000001F; // Conflicts with key
        const int KeyMask = 0x01FFFFFF;
        const int ModMask = 0x7E000000;

        const int KeyEnumIndex = staticQtMetaObject.indexOfEnumerator("Key");
        const int ModEnumIndex = staticQtMetaObject.indexOfEnumerator("KeyboardModifiers");

        std::string output;

        // KeySequence
        for (int i = 0; i < (int)data.first.count(); ++i)
        {
            if (data.first[i] & ModMask)
            {
                // TODO separate out modifiers to allow more than 1
                output.append(staticQtMetaObject.enumerator(ModEnumIndex).valueToKey(data.first[i] & ModMask));
                output.append("+");
            }

            if (data.first[i] & KeyMask & ~MouseMask)
            {
                // Is a key
                output.append(staticQtMetaObject.enumerator(KeyEnumIndex).valueToKey(data.first[i] & KeyMask));
                output.append(",");
            }
            else if (data.first[i] & MouseMask)
            {
                std::stringstream ss;
                std::string num;

                unsigned int value = (unsigned int)(data.first[i] & MouseMask);

                // value will never be 0
                int exponent = 1; // Offset by 1
                while (value >>= 1)
                    ++exponent;

                ss << exponent;
                ss >> num;

                // Is a mouse button
                output.append("Mouse");
                output.append(num);
                output.append(",");
            }
        }

        // Remove last comma
        if (output.size() > 0)
        {
            output.resize(output.size() - 1);
        }

        // Add modifier if needed
        if (data.second & ModMask)
        {
            output.append(";");
            output.append(staticQtMetaObject.enumerator(ModEnumIndex).valueToKey(data.second & ModMask));
        }
        else if (data.second & KeyMask & ~MouseMask)
        {
            output.append(";");
            output.append(staticQtMetaObject.enumerator(KeyEnumIndex).valueToKey(data.second & KeyMask));
        }
        else if (data.second & MouseMask)
        {
            std::stringstream ss;
            std::string num;

            unsigned int value = (unsigned int)(data.second & MouseMask);

            // value will never be 0
            int exponent = 1; // Offset by 1
            while (value >>= 1)
                ++exponent;

            ss << exponent;
            ss >> num;

            // Is a mouse button
            output.append(";Mouse");
            output.append(num);
        }

        return output;
    }

    ShortcutManager::SequenceData ShortcutManager::stringToSequence(const std::string& input)
    {
        // TODO clean and standardize

        const int KeyEnumIndex = staticQtMetaObject.indexOfEnumerator("Key");
        const int ModEnumIndex = staticQtMetaObject.indexOfEnumerator("KeyboardModifiers");

        int keys[4] = { 0, 0, 0, 0 };
        int modifier = 0;

        size_t middle = input.find(';');
        std::string sequenceStr = input.substr(0, middle);
        std::string modifierStr = input.substr((middle < input.size())? middle + 1 : input.size());

        QRegExp splitRX("[, ]");
        QStringList keyStrs = QString(sequenceStr.c_str()).split(splitRX, QString::SkipEmptyParts);

        for (int i = 0; i < keyStrs.size(); ++i)
        {
            QRegExp modSeparator("[+]");

            QStringList separatedList = keyStrs[i].split(modSeparator, QString::SkipEmptyParts);
            for (int j = 0; j < separatedList.size(); ++j)
            {
                if (separatedList[j].startsWith("Mouse"))
                {
                    QString num = separatedList[j].mid(5);
                    if (num > 0)
                    {
                        keys[i] |= 1 << (num.toInt() - 1); // offset by 1
                    }
                }
                else if (staticQtMetaObject.enumerator(ModEnumIndex).keyToValue(separatedList[j].toUtf8().data()) != -1)
                {
                    keys[i] |= staticQtMetaObject.enumerator(ModEnumIndex).keyToValue(separatedList[j].toUtf8().data());
                }
                else if (staticQtMetaObject.enumerator(KeyEnumIndex).keyToValue(separatedList[j].toUtf8().data()) != -1)
                {
                    keys[i] |= staticQtMetaObject.enumerator(KeyEnumIndex).keyToValue(separatedList[j].toUtf8().data());
                }
            }
        }

        if (!modifierStr.empty())
        {
            if (modifierStr.find("Mouse") != std::string::npos)
            {
                QString num = QString::fromUtf8(modifierStr.substr(5).data());
                if (num > 0)
                {
                    modifier = 1 << (num.toInt() - 1); // offset by 1
                }
            }
            else if (staticQtMetaObject.enumerator(ModEnumIndex).keyToValue(modifierStr.data()) != -1)
            {
                modifier = staticQtMetaObject.enumerator(ModEnumIndex).keyToValue(modifierStr.data());
            }
            else if (staticQtMetaObject.enumerator(KeyEnumIndex).keyToValue(modifierStr.data()) != -1)
            {
                modifier = staticQtMetaObject.enumerator(KeyEnumIndex).keyToValue(modifierStr.data());
            }
        }

        // TODO remove
        std::cout << input << '.' << keys[0] << '.'<< keys[1] << '.'<< keys[2] << '.'<< keys[3] << '.' << modifier << std::endl;

        return std::make_pair(QKeySequence(keys[0], keys[1], keys[2], keys[3]), modifier);
    }
}
