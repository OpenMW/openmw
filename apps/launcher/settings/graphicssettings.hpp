#ifndef GRAPHICSSETTINGS_HPP
#define GRAPHICSSETTINGS_HPP

#include "settingsbase.hpp"

namespace Launcher
{
    class GraphicsSettings : public SettingsBase<QMap<QString, QString> >
    {
    public:
        GraphicsSettings();
        ~GraphicsSettings();

        bool writeFile(QTextStream &stream);

    };
}
#endif // GRAPHICSSETTINGS_HPP
