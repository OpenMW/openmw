#ifndef OMWLOADER_HPP
#define OMWLOADER_HPP

#include "contentloader.hpp"

namespace MWWorld
{

/**
 * @brief Placeholder for real OpenMW content loader
 */
struct OmwLoader : public ContentLoader
{
    OmwLoader(Loading::Listener& listener);

    void load(const boost::filesystem::path& filepath, int& index);
};

} /* namespace MWWorld */

#endif /* OMWLOADER_HPP */
