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

        inline void setValue(const QString &key, const QVariant &value)
        {
            mSettings.insert(key, value);
        }

        inline void remove(const QString &key)
        {
            mSettings.remove(key);
        }

        bool readFile(QTextStream &stream);
        bool writeFile(QTextStream &stream);

    private:

        SettingsMap mSettings;
    };

}

#endif // INISETTINGS_HPP
