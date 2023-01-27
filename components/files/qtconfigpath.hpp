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
}

#endif // OPENMW_COMPONENTS_FILES_QTCONFIGPATH_H
