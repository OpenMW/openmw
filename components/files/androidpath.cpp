#include "androidpath.hpp"

#if defined(__ANDROID__) 

#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include "androidpath.h"
#include <unistd.h>
#include <boost/filesystem/fstream.hpp>


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

    boost::filesystem::path getUserHome()
    {
        const char* dir = getenv("HOME");
        if (dir == nullptr)
        {
            struct passwd* pwd = getpwuid(getuid());
            if (pwd != nullptr)
            {
                dir = pwd->pw_dir;
            }
        }
        if (dir == nullptr)
            return boost::filesystem::path();
        else
            return boost::filesystem::path(dir);
    }

    boost::filesystem::path getEnv(const std::string& envVariable, const boost::filesystem::path& fallback)
    {
        const char* result = getenv(envVariable.c_str());
        if (!result)
            return fallback;
        boost::filesystem::path dir(result);
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

boost::filesystem::path AndroidPath::getUserConfigPath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/config";
    return getEnv("XDG_CONFIG_HOME", buffer) / mName;
}

boost::filesystem::path AndroidPath::getUserDataPath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/share";
    return getEnv("XDG_DATA_HOME", buffer) / mName;
}

boost::filesystem::path AndroidPath::getCachePath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/cache";
    return getEnv("XDG_CACHE_HOME", buffer) / mName;
}

boost::filesystem::path AndroidPath::getGlobalConfigPath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/";
    boost::filesystem::path globalPath(buffer); 
    return globalPath / mName;
}

boost::filesystem::path AndroidPath::getLocalPath() const
{
    return boost::filesystem::path("./");
}

boost::filesystem::path AndroidPath::getGlobalDataPath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/data";
    boost::filesystem::path globalDataPath(buffer);
    return globalDataPath / mName;
}

boost::filesystem::path AndroidPath::getInstallPath() const
{
    return boost::filesystem::path();
}


} /* namespace Files */

#endif /* defined(__Android__) */
