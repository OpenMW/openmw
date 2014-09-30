#include "nifcache.hpp"

namespace Nif
{

Cache* Cache::sThis = 0;

Cache& Cache::getInstance()
{
    assert (sThis);
    return *sThis;
}

Cache* Cache::getInstancePtr()
{
    return sThis;
}

Cache::Cache()
{
    assert (!sThis);
    sThis = this;
}

NIFFilePtr Cache::load(const std::string &filename)
{
    // TODO: normalize file path to make sure we're not loading the same file twice

    LoadedMap::iterator it = mLoadedMap.find(filename);
    if (it != mLoadedMap.end())
        return it->second;
    else
    {
        NIFFilePtr file(new Nif::NIFFile(filename));
        mLoadedMap[filename] = file;
        return file;
    }
}

}
