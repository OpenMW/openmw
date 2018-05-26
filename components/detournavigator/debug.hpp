#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_H

#include "tilebounds.hpp"

#include <components/bullethelpers/operators.hpp>
#include <components/osghelpers/operators.hpp>

#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

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

    class Log
    {
    public:
        Log()
            : mEnabled()
        {
            mFile.exceptions(std::ios::failbit | std::ios::badbit);
        }

        void setEnabled(bool value)
        {
            mEnabled = value;
        }

        bool isEnabled() const
        {
            return mEnabled;
        }

        void write(const std::string& text)
        {
            if (mEnabled)
            {
                const std::lock_guard<std::mutex> lock(mMutex);
                if (!mFile.is_open())
                {
                    mFile.open("detournavigator.log");
                }
                mFile << text << std::flush;
            }
        }

        static Log& instance()
        {
            static Log value;
            return value;
        }

    private:
        std::mutex mMutex;
        std::ofstream mFile;
        std::atomic_bool mEnabled;
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
        stream << '[' << std::chrono::steady_clock::now() << "] ";
        write(stream, std::forward<Ts>(values) ...);
        log.write(stream.str());
    }

    void writeToFile(const RecastMesh& recastMesh, const std::string& pathPrefix, const std::string& revision);
    void writeToFile(const dtNavMesh& navMesh, const std::string& pathPrefix, const std::string& revision);
}

#endif
