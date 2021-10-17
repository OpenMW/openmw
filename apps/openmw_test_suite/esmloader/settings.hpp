#ifndef OPENMW_TEST_SUITE_LOAD_SETTINGS_H
#define OPENMW_TEST_SUITE_LOAD_SETTINGS_H

#include <boost/filesystem/path.hpp>

namespace EsmLoaderTests
{
    struct Settings
    {
        boost::filesystem::path mBasePath;

        static Settings& impl()
        {
            static Settings value;
            return value;
        }
    };
}

#endif
