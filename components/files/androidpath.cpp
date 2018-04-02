#include "androidpath.hpp"

#if defined(__ANDROID__) 

#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include "androidpath.h"
#include <unistd.h>

namespace sfs = std::experimental::filesystem;

class Buffer {
    public:
      static void setData(char const *data);
      static char const * getData();
};
static char const *path;

void Buffer::setData(char const *data)
{
    path=data;
}
char const * Buffer::getData()
{
    return path;
}


JNIEXPORT void JNICALL Java_ui_activity_GameActivity_getPathToJni(JNIEnv *env, jobject obj, jstring prompt)
{
    jboolean iscopy;
    Buffer::setData((env)->GetStringUTFChars(prompt, &iscopy));
    (env)->DeleteLocalRef(prompt);
}

namespace
{

    sfs::path getUserHome()
    {
        const char* dir = getenv("HOME");
        if (dir == NULL)
        {
            struct passwd* pwd = getpwuid(getuid());
            if (pwd != NULL)
            {
                dir = pwd->pw_dir;
            }
        }
        if (dir == NULL)
            return sfs::path();
        else
            return sfs::path(dir);
    }

    sfs::path getEnv(const std::string& envVariable, const sfs::path& fallback)
    {
        const char* result = getenv(envVariable.c_str());
        if (!result)
            return fallback;
        sfs::path dir(result);
        if (dir.empty())
            return fallback;
        else
            return dir;
    }
}

/**
 * \namespace Files
 */
namespace Files
{

AndroidPath::AndroidPath(const std::string& application_name)
    : mName(application_name)
{
}

sfs::path AndroidPath::getUserConfigPath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/config";
    return getEnv("XDG_CONFIG_HOME", buffer) / mName;
}

sfs::path AndroidPath::getUserDataPath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/share";
    return getEnv("XDG_DATA_HOME", buffer) / mName;
}

sfs::path AndroidPath::getCachePath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/cache";
    return getEnv("XDG_CACHE_HOME", buffer) / mName;
}

sfs::path AndroidPath::getGlobalConfigPath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/";
    sfs::path globalPath(buffer);
    return globalPath / mName;
}

sfs::path AndroidPath::getLocalPath() const
{
    return sfs::path("./");
}

sfs::path AndroidPath::getGlobalDataPath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/data";
    sfs::path globalDataPath(buffer);
    return globalDataPath / mName;
}

sfs::path AndroidPath::getInstallPath() const
{
    return sfs::path();
}


} /* namespace Files */

#endif /* defined(__Android__) */
