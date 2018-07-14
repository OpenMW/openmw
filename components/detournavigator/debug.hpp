#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_H

#include "tilebounds.hpp"

#include <components/bullethelpers/operators.hpp>
#include <components/osghelpers/operators.hpp>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

class dtNavMesh;

namespace DetourNavigator
{
    inline std::ostream& operator <<(std::ostream& stream, const TileBounds& value)
    {
        return stream << "TileBounds {" << value.mMin << ", " << value.mMax << "}";
    }

    class RecastMesh;

    inline std::ostream& operator <<(std::ostream& stream, const std::chrono::steady_clock::time_point& value)
    {
        using float_s = std::chrono::duration<float, std::ratio<1>>;
        return stream << std::fixed << std::setprecision(4)
                      << std::chrono::duration_cast<float_s>(value.time_since_epoch()).count();
    }

    struct Sink
    {
        virtual ~Sink() = default;
        virtual void write(const std::string& text) = 0;
    };

    class FileSink final : public Sink
    {
    public:
        FileSink(std::string path)
            : mPath(std::move(path))
        {
            mFile.exceptions(std::ios::failbit | std::ios::badbit);
        }

        void write(const std::string& text) override
        {
            if (!mFile.is_open())
            {
                mFile.open(mPath);
            }
            mFile << text << std::flush;
        }

    private:
        std::string mPath;
        std::ofstream mFile;
    };

    class StdoutSink final : public Sink
    {
    public:
        void write(const std::string& text) override
        {
            std::cout << text << std::flush;
        }
    };

    class Log
    {
    public:
        void setSink(std::unique_ptr<Sink> sink)
        {
            const std::lock_guard<std::mutex> guard(mMutex);
            mSink = std::move(sink);
        }

        bool isEnabled() const
        {
            const std::lock_guard<std::mutex> guard(mMutex);
            return bool(mSink);
        }

        void write(const std::string& text)
        {
            const std::lock_guard<std::mutex> guard(mMutex);
            if (mSink)
                mSink->write(text);
        }

        static Log& instance()
        {
            static Log value;
            return value;
        }

    private:
        mutable std::mutex mMutex;
        std::unique_ptr<Sink> mSink;
    };

    inline void write(std::ostream& stream)
    {
        stream << '\n';
    }

    template <class Head, class ... Tail>
    void write(std::ostream& stream, const Head& head, const Tail& ... tail)
    {
        stream << head;
        write(stream, tail ...);
    }

    template <class ... Ts>
    void log(Ts&& ... values)
    {
        auto& log = Log::instance();
        if (!log.isEnabled())
            return;
        std::ostringstream stream;
        stream << '[' << std::chrono::steady_clock::now() << "] [" << std::this_thread::get_id() << "] ";
        write(stream, std::forward<Ts>(values) ...);
        log.write(stream.str());
    }

    void writeToFile(const RecastMesh& recastMesh, const std::string& pathPrefix, const std::string& revision);
    void writeToFile(const dtNavMesh& navMesh, const std::string& pathPrefix, const std::string& revision);
}

#endif
