#include "switchpath.hpp"

#if defined(__SWITCH__)

#include <switch.h>

#include <unistd.h>
#include <boost/filesystem/fstream.hpp>

#include <components/misc/stringops.hpp>

/**
 * \namespace Files
 */
namespace Files
{

SwitchPath::SwitchPath(const std::string& application_name)
    : mName(application_name)
{
}

boost::filesystem::path SwitchPath::getUserConfigPath() const
{
    return boost::filesystem::path("./config");
}

boost::filesystem::path SwitchPath::getUserDataPath() const
{
    // get the name of the current user
    accountIntialize();
    
    u128 userId = 0;
    bool accountSelected = 0;
    
    char username[0x21] = "Global";
    
    accountGetActiveUser(&userId, &accountSelected);
    
    if (!accountSelected) {
        username = "Global"; // if there's no account, just assume that it's a global profile
    } else {
        AccountProfile profile;
        accountGetProfile(&profile, userId);
        
        AccountProfileBase profilebase;
        accountProfileGet(&profile, nullptr, &profilebase);
        
        username = profilebase.username;
        
        accountProfileClose(&profile);
    }
    accountExit();
    
    return boost::filesystem::path("./data/" + std::string(username));
}

boost::filesystem::path SwitchPath::getCachePath() const
{
    return boost::filesystem::path("./cache");
}

boost::filesystem::path SwitchPath::getGlobalConfigPath() const
{
    return boost::filesystem::path("./default");
}

boost::filesystem::path SwitchPath::getLocalPath() const
{
    return boost::filesystem::path("./");
}

boost::filesystem::path SwitchPath::getGlobalDataPath() const
{
    return boost::filesystem::path("./data");
}

boost::filesystem::path SwitchPath::getInstallPath() const
{
    return boost::filesystem::path("./mw");
}

} /* namespace Files */

#endif /* defined(__SWITCH__) */
