#ifndef SWITCH_STARTUP_H
#define SWITCH_STARTUP_H

#include <components/files/configurationmanager.hpp>

namespace Switch
{
    void startup();
    void shutdown();
    void importIni(Files::ConfigurationManager& cfgMgr);
    std::string getUsername();
    void fatal(const char *fmt, ...);
}

#endif /* SWITCH_STARTUP_H */
