//
// Created by koncord on 24.01.16.
//

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

#ifdef _WIN32
int setenv(const char *name, const char *value, int overwrite);
#endif

namespace Utils
{
    std::string convertPath(std::string str);

    void timestamp();

    int progress_func(double TotalToDownload, double NowDownloaded);

    bool DoubleCompare(double a, double b, double epsilon);

    std::string str_replace(const std::string &source, const char *find, const char *replace);

    std::string &RemoveExtension(std::string &file);

    long int FileLength(const char *file);

    unsigned int crc32checksum(const std::string &file);
}
#endif //UTILS_HPP
