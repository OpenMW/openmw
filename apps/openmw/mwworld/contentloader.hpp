#ifndef CONTENTLOADER_HPP
#define CONTENTLOADER_HPP

#include <iosfwd>
#include <iostream>
#include <boost/filesystem/path.hpp>

#include "components/loadinglistener/loadinglistener.hpp"

namespace MWWorld
{

struct ContentLoader
{
    ContentLoader(Loading::Listener& listener)
      : mListener(listener)
    {
    }

    virtual ~ContentLoader()
    {
    }

    virtual void load(const boost::filesystem::path& filepath, int& index)
    {
      std::cout << "Loading content file " << filepath.string() << std::endl;
      mListener.setLabel(filepath.string());
    }

    protected:
        Loading::Listener& mListener;
};

} /* namespace MWWorld */

#endif /* CONTENTLOADER_HPP */
