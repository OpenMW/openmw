#ifndef CSM_PREFS_SHORTCUTMANAGER_H
#define CSM_PREFS_SHORTCUTMANAGER_H

#include <map>

#include <QKeySequence>
#include <QObject>
#include <QString>

namespace CSMPrefs
{
    class Shortcut;
    class ShortcutEventHandler;

    /// Class used to track and update shortcuts/sequences
    class ShortcutManager : public QObject
    {
            Q_OBJECT

        public:

            ShortcutManager();

            /// The shortcut class will do this automatically
            void addShortcut(Shortcut* shortcut);

            /// The shortcut class will do this automatically
            void removeShortcut(Shortcut* shortcut);

            bool getSequence(const std::string& name, QKeySequence& sequence) const;
            void setSequence(const std::string& name, const QKeySequence& sequence);

            bool getModifier(const std::string& name, int& modifier) const;
            void setModifier(const std::string& name, int modifier);

            std::string convertToString(const QKeySequence& sequence) const;
            std::string convertToString(int modifier) const;

            std::string convertToString(const QKeySequence& sequence, int modifier) const;

            void convertFromString(const std::string& data, QKeySequence& sequence) const;
            void convertFromString(const std::string& data, int& modifier) const;

            void convertFromString(const std::string& data, QKeySequence& sequence, int& modifier) const;

            /// Replaces "{sequence-name}" or "{modifier-name}" with the appropriate text
            QString processToolTip(const QString& toolTip) const;

        private:

            // Need a multimap in case multiple shortcuts share the same name
            typedef std::multimap<std::string, Shortcut*> ShortcutMap;
            typedef std::map<std::string, QKeySequence> SequenceMap;
            typedef std::map<std::string, int> ModifierMap;
            typedef std::map<int, std::string> NameMap;
            typedef std::map<std::string, int> KeyMap;

            ShortcutMap mShortcuts;
            SequenceMap mSequences;
            ModifierMap mModifiers;

            NameMap mNames;
            KeyMap mKeys;

            ShortcutEventHandler* mEventHandler;

            void createLookupTables();

            static const std::pair<int, const char*> QtKeys[];
    };
}

#endif
