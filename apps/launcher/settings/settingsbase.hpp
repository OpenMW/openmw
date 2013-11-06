#ifndef SETTINGSBASE_HPP
#define SETTINGSBASE_HPP

#include <QTextStream>
#include <QStringList>
#include <QString>
#include <QRegExp>
#include <QMap>

namespace Launcher
{
    template <class Map>
    class SettingsBase
    {

    public:
        SettingsBase() { mMultiValue = false; }
        ~SettingsBase() {}

        inline QString value(const QString &key, const QString &defaultValue = QString())
        {
            return mSettings.value(key).isEmpty() ? defaultValue : mSettings.value(key);
        }

        inline void setValue(const QString &key, const QString &value)
        {
            QStringList values = mSettings.values(key);
            if (!values.contains(value))
                mSettings.insert(key, value);
        }

        inline void setMultiValue(const QString &key, const QString &value)
        {
            QStringList values = mSettings.values(key);
            if (!values.contains(value))
                mSettings.insertMulti(key, value);
        }

        inline void setMultiValueEnabled(bool enable)
        {
            mMultiValue = enable;
        }

        inline void remove(const QString &key)
        {
            mSettings.remove(key);
        }

        Map getSettings() {return mSettings;}

        bool readFile(QTextStream &stream)
        {
            mCache.clear();

            QString sectionPrefix;

            QRegExp sectionRe("^\\[([^]]+)\\]");
            QRegExp keyRe("^([^=]+)\\s*=\\s*(.+)$");

            while (!stream.atEnd()) {
                QString line = stream.readLine();

                if (line.isEmpty() || line.startsWith("#"))
                    continue;

                if (sectionRe.exactMatch(line)) {
                    sectionPrefix = sectionRe.cap(1);
                    sectionPrefix.append("/");
                    continue;
                }

                if (keyRe.indexIn(line) != -1) {

                    QString key = keyRe.cap(1).trimmed();
                    QString value = keyRe.cap(2).trimmed();

                    if (!sectionPrefix.isEmpty())
                        key.prepend(sectionPrefix);

                    mSettings.remove(key);

                    QStringList values = mCache.values(key);

                    if (!values.contains(value)) {
                        if (mMultiValue) {
                            mCache.insertMulti(key, value);
                        } else {
                            mCache.insert(key, value);
                        }
                    }
                }
            }

            if (mSettings.isEmpty()) {
                mSettings = mCache; // This is the first time we read a file
                return true;
            }

            // Merge the changed keys with those which didn't
            mSettings.unite(mCache);
            return true;
        }

    private:
        Map mSettings;
        Map mCache;

        bool mMultiValue;
    };
}
#endif // SETTINGSBASE_HPP
