#ifndef CONTENTLOADER_HPP
#define CONTENTLOADER_HPP

#include <iosfwd>
#include <iostream>
#include <experimental/filesystem>
#include <MyGUI_TextIterator.h>

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

    virtual void load(const std::experimental::filesystem::path& filepath, int& index)
    {
      std::cout << "Loading content file " << filepath.string() << std::endl;
      mListener.setLabel(MyGUI::TextIterator::toTagsString(filepath.string()));
    }

    protected:
        Loading::Listener& mListener;
};

} /* namespace MWWorld */

#endif /* CONTENTLOADER_HPP */
