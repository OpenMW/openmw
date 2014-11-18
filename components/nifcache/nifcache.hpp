#ifndef OPENMW_COMPONENTS_NIFCACHE_H
#define OPENMW_COMPONENTS_NIFCACHE_H

#include <components/nif/niffile.hpp>

#include <boost/shared_ptr.hpp>

#include <map>

namespace Nif
{

    typedef boost::shared_ptr<Nif::NIFFile> NIFFilePtr;

    /// @brief A basic resource manager for NIF files
    class Cache
    {
    public:
        Cache();

        /// Queue this file for background loading. A worker thread will start loading the file.
        /// To get the loaded NIFFilePtr, use the load method, which will wait until the worker thread is finished
        /// and then return the loaded file.
        //void loadInBackground (const std::string& file);

        /// Read and parse the given file. May retrieve from cache if this file has been used previously.
        /// @note If the file is currently loading in the background, this function will block until
        ///       the background loading finishes, then return the background loaded file.
        /// @note Returns a SharedPtr to the file and the file will stay loaded as long as the user holds on to this pointer.
        ///       When all external SharedPtrs to a file are released, the cache may decide to unload the file.
        NIFFilePtr load (const std::string& filename);

        /// Return instance of this class.
        static Cache& getInstance();
        static Cache* getInstancePtr();

    private:
        static Cache* sThis;

        Cache(const Cache&);
        Cache& operator =(const Cache&);

        typedef std::map<std::string, NIFFilePtr> LoadedMap;

        LoadedMap mLoadedMap;
    };

}

#endif
