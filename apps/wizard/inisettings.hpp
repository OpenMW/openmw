#ifndef INISETTINGS_HPP
#define INISETTINGS_HPP

#include <QHash>
#include <QVariant>

class QTextStream;

namespace Wizard
{

    typedef QHash<QString, QVariant> SettingsMap;

    class IniSettings
    {
    public:
        explicit IniSettings();
        ~IniSettings();

        inline QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const
        {
            return mSettings.value(key, defaultValue);
        }

        inline QList<QVariant> values() const
        {
            return mSettings.values();
        }

        inline void setValue(const QString &key, const QVariant &value)
        {
            mSettings.insert(key, value);
        }

        inline void remove(const QString &key)
        {
            mSettings.remove(key);
        }

        QStringList findKeys(const QString &text);

        bool readFile(QTextStream &stream);
        bool writeFile(const QString &path, QTextStream &stream);

        bool parseInx(const QString &path);

    private:

        int getLastNewline(const QString &buffer, int from);

        SettingsMap mSettings;
    };

}

#endif // INISETTINGS_HPP
