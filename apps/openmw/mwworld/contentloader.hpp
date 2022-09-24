#ifndef CONTENTLOADER_HPP
#define CONTENTLOADER_HPP

#include <filesystem>

namespace Loading
{
    class Listener;
}

namespace MWWorld
{

    struct ContentLoader
    {
        virtual ~ContentLoader() = default;

        virtual void load(const std::filesystem::path& filepath, int& index, Loading::Listener* listener) = 0;
    };

} /* namespace MWWorld */

#endif /* CONTENTLOADER_HPP */
