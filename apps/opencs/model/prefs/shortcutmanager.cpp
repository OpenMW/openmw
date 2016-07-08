#include "shortcutmanager.hpp"

#include <sstream>
#include <iostream>

#include <QMetaEnum>

#include "shortcut.hpp"

namespace CSMPrefs
{
    void ShortcutManager::addShortcut(Shortcut* shortcut)
    {
        mShortcuts.insert(std::make_pair(shortcut->getName(), shortcut));
    }

    void ShortcutManager::addShortcut(QShortcutWrapper* wrapper)
    {
        mShortcutWrappers.insert(std::make_pair(wrapper->getName(), wrapper));
    }

    void ShortcutManager::removeShortcut(Shortcut* shortcut)
    {
        std::pair<ShortcutMap::iterator, ShortcutMap::iterator> range = mShortcuts.equal_range(shortcut->getName());

        for (ShortcutMap::iterator it = range.first; it != range.second;)
        {
            if (it->second == shortcut)
            {
                it = mShortcuts.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void ShortcutManager::removeShortcut(QShortcutWrapper* wrapper)
    {
        std::pair<ShortcutWrapperMap::iterator, ShortcutWrapperMap::iterator> range = mShortcutWrappers.equal_range(
            wrapper->getName());

        for (ShortcutWrapperMap::iterator it = range.first; it != range.second;)
        {
            if (it->second == wrapper)
            {
                it = mShortcutWrappers.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    QKeySequence ShortcutManager::getSequence(const std::string& name) const
    {
        QKeySequence sequence;
        SequenceMap::const_iterator item = mSequences.find(name);

        if (item != mSequences.end())
        {
            sequence = item->second;
        }

        return sequence;
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
        std::pair<ShortcutWrapperMap::iterator, ShortcutWrapperMap::iterator> rangeW = mShortcutWrappers.equal_range(name);

        for (ShortcutMap::iterator it = rangeS.first; it != rangeS.second; ++it)
        {
            it->second->setSequence(sequence);
        }

        for (ShortcutWrapperMap::iterator it = rangeW.first; it != rangeW.second; ++it)
        {
            it->second->setSequence(sequence);
        }
    }

    std::string ShortcutManager::sequenceToString(const QKeySequence& seq)
    {
        const int MouseMask = 0x0000001F; // Conflicts with key
        const int KeyMask = 0x01FFFFFF;
        const int ModMask = 0x7E000000;

        const int KeyEnumIndex = staticQtMetaObject.indexOfEnumerator("Key");
        const int ModEnumIndex = staticQtMetaObject.indexOfEnumerator("KeyboardModifiers");

        std::string output;

        for (int i = 0; i < seq.count(); ++i)
        {
            if (seq[i] & ModMask)
            {
                // TODO separate out modifiers to allow more than 1
                output.append(staticQtMetaObject.enumerator(ModEnumIndex).valueToKey(seq[i] & ModMask));
                output.append("+");
            }

            if (seq[i] & KeyMask & ~MouseMask)
            {
                // Is a key
                output.append(staticQtMetaObject.enumerator(KeyEnumIndex).valueToKey(seq[i] & KeyMask));
                output.append(",");
            }
            else if (seq[i] & MouseMask)
            {
                std::stringstream ss;
                std::string num;

                unsigned int value = (unsigned int)(seq[i] & MouseMask);

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

        return output;
    }

    QKeySequence ShortcutManager::stringToSequence(const std::string& input)
    {
        const int KeyEnumIndex = staticQtMetaObject.indexOfEnumerator("Key");
        const int ModEnumIndex = staticQtMetaObject.indexOfEnumerator("KeyboardModifiers");

        int keys[4] = { 0, 0, 0, 0 };

        QRegExp splitRX("[, ]");
        QStringList keyStrs = QString(input.c_str()).split(splitRX, QString::SkipEmptyParts);

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

        // TODO remove
        std::cout << input << '.' << keys[0] << '.'<< keys[1] << '.'<< keys[2] << '.'<< keys[3] << std::endl;

        return QKeySequence(keys[0], keys[1], keys[2], keys[3]);
    }
}
