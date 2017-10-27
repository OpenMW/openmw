#include "androidpath.hpp"

#if defined(__ANDROID__) 

#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include "androidpath.h"
#include <unistd.h>



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

    std::experimental::filesystem::path getUserHome()
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
            return std::experimental::filesystem::path();
        else
            return std::experimental::filesystem::path(dir);
    }

    std::experimental::filesystem::path getEnv(const std::string& envVariable, const std::experimental::filesystem::path& fallback)
    {
        const char* result = getenv(envVariable.c_str());
        if (!result)
            return fallback;
        std::experimental::filesystem::path dir(result);
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

std::experimental::filesystem::path AndroidPath::getUserConfigPath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/config";
    return getEnv("XDG_CONFIG_HOME", buffer) / mName;
}

std::experimental::filesystem::path AndroidPath::getUserDataPath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/share";
    return getEnv("XDG_DATA_HOME", buffer) / mName;
}

std::experimental::filesystem::path AndroidPath::getCachePath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/cache";
    return getEnv("XDG_CACHE_HOME", buffer) / mName;
}

std::experimental::filesystem::path AndroidPath::getGlobalConfigPath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/";
    std::experimental::filesystem::path globalPath(buffer);
    return globalPath / mName;
}

std::experimental::filesystem::path AndroidPath::getLocalPath() const
{
    return std::experimental::filesystem::path("./");
}

std::experimental::filesystem::path AndroidPath::getGlobalDataPath() const
{
    std::string buffer = ""; 
    buffer = buffer + Buffer::getData() +"/data";
    std::experimental::filesystem::path globalDataPath(buffer);
    return globalDataPath / mName;
}

std::experimental::filesystem::path AndroidPath::getInstallPath() const
{
    return std::experimental::filesystem::path();
}


} /* namespace Files */

#endif /* defined(__Android__) */
