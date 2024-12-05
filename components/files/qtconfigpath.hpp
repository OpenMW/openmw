#ifndef OPENMW_COMPONENTS_FILES_QTCONFIGPATH_H
#define OPENMW_COMPONENTS_FILES_QTCONFIGPATH_H

#include "configurationmanager.hpp"
#include "qtconversion.hpp"

#include <QString>

namespace Files
{
    inline QString getLocalConfigPathQString(const Files::ConfigurationManager& cfgMgr)
    {
        return Files::pathToQString(cfgMgr.getLocalPath() / openmwCfgFile);
    }

    inline QString getUserConfigPathQString(const Files::ConfigurationManager& cfgMgr)
    {
        return Files::pathToQString(cfgMgr.getUserConfigPath() / openmwCfgFile);
    }

    inline QString getGlobalConfigPathQString(const Files::ConfigurationManager& cfgMgr)
    {
        return Files::pathToQString(cfgMgr.getGlobalPath() / openmwCfgFile);
    }

    inline QStringList getActiveConfigPathsQString(const Files::ConfigurationManager& cfgMgr)
    {
        const auto& activePaths = cfgMgr.getActiveConfigPaths();
        QStringList result;
        result.reserve(static_cast<int>(activePaths.size()));
        for (const auto& path : activePaths)
            result.append(Files::pathToQString(path / openmwCfgFile));
        return result;
    }
}

#endif // OPENMW_COMPONENTS_FILES_QTCONFIGPATH_H
