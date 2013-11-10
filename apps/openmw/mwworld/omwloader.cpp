#include "omwloader.hpp"

namespace MWWorld
{

OmwLoader::OmwLoader(Loading::Listener& listener)
  : ContentLoader(listener)
{
}

void OmwLoader::load(const boost::filesystem::path& filepath, int& index)
{
    ContentLoader::load(filepath.filename(), index);
}

} /* namespace MWWorld */

