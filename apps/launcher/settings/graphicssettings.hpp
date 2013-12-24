#ifndef GRAPHICSSETTINGS_HPP
#define GRAPHICSSETTINGS_HPP

#include <components/config/settingsbase.hpp>

namespace Launcher
{
    class GraphicsSettings : public Config::SettingsBase<QMap<QString, QString> >
    {
    public:
        GraphicsSettings();
        ~GraphicsSettings();

        bool writeFile(QTextStream &stream);

    };
}
#endif // GRAPHICSSETTINGS_HPP
