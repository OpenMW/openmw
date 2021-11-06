#ifndef CONTENTLOADER_HPP
#define CONTENTLOADER_HPP

#include <boost/filesystem/path.hpp>

#include <components/debug/debuglog.hpp>
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

    virtual void load(const boost::filesystem::path& filepath)
    {
        Log(Debug::Info) << "Loading content file " << filepath.string();
        mListener.setLabel(filepath.string());
    }

    protected:
        Loading::Listener& mListener;
};

} /* namespace MWWorld */

#endif /* CONTENTLOADER_HPP */
