#ifndef CSM_PREFS_SHORTCUTMANAGER_H
#define CSM_PREFS_SHORTCUTMANAGER_H

#include <map>

#include <QKeySequence>
#include <QObject>

namespace CSMPrefs
{
    class Shortcut;

    /// Class used to track and update shortcuts/sequences
    class ShortcutManager : public QObject
    {
            Q_OBJECT

        public:

            /// The shortcut class will do this automatically
            void addShortcut(Shortcut* shortcut);

            /// The shortcut class will do this automatically
            void removeShortcut(Shortcut* shortcut);

            QKeySequence getSequence(const std::string& name) const;
            void setSequence(const std::string& name, const QKeySequence& sequence);

            std::string sequenceToString(const QKeySequence& sequence);
            QKeySequence stringToSequence(const std::string& str);

        private:

            // Need a multimap in case multiple shortcuts share the same name
            typedef std::multimap<std::string, Shortcut*> ShortcutMap;
            typedef std::map<std::string, QKeySequence> SequenceMap;

            ShortcutMap mShortcuts;
            SequenceMap mSequences;
    };
}

#endif
