#ifndef OPENMW_COMPONENTS_ESM_PATH_H
#define OPENMW_COMPONENTS_ESM_PATH_H

#include <components/vfs/pathutil.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace ESM
{
    class Path
    {
    public:
        const std::string& getOriginal() const { return mOriginal; }

        const VFS::Path::Normalized& getNormalized() const { return mNormalized; }

        bool empty() const { return mOriginal.empty(); }

        void set(std::string&& value)
        {
            mOriginal = std::move(value);
            mNormalized = VFS::Path::Normalized(mOriginal);
        }

        void set(std::string_view value)
        {
            mOriginal = value;
            mNormalized = VFS::Path::Normalized(mOriginal);
        }

        void clear()
        {
            mOriginal.clear();
            mNormalized.clear();
        }

        Path& operator=(const std::string& value)
        {
            set(std::string_view(value));
            return *this;
        }

        Path& operator=(std::string&& value)
        {
            set(std::move(value));
            return *this;
        }

        Path& operator=(const char* value)
        {
            set(std::string_view(value));
            return *this;
        }

        Path& operator=(std::string_view value)
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
