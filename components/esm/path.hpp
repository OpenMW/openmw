#ifndef OPENMW_COMPONENTS_ESM_PATH_H
#define OPENMW_COMPONENTS_ESM_PATH_H

#include <components/vfs/pathutil.hpp>

#include <string>

namespace ESM
{
    class Path
    {
    public:
        const std::string& getOriginal() const { return mOriginal; }

        const VFS::Path::Normalized& getNormalized() const { return mNormalized; }

        void set(std::string&& value)
        {
            mOriginal = std::move(value);
            mNormalized = VFS::Path::Normalized(mOriginal);
        }

        void set(const char* value)
        {
            mOriginal = value;
            mNormalized = VFS::Path::Normalized(mOriginal);
        }

        void clear()
        {
            mOriginal.clear();
            mNormalized.clear();
        }

        Path& operator=(std::string&& value)
        {
            set(std::move(value));
            return *this;
        }

        Path& operator=(const char* value)
        {
            set(value);
            return *this;
        }

    private:
        std::string mOriginal;
        VFS::Path::Normalized mNormalized;
    };
}

#endif
