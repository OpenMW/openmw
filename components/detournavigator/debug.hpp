#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_H

#include "tilebounds.hpp"

#include <components/bullethelpers/operators.hpp>
#include <components/osghelpers/operators.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#ifdef OPENMW_WRITE_OBJ
#include <vector>
#endif

#ifdef OPENMW_WRITE_TO_FILE
class dtNavMesh;
#endif

namespace DetourNavigator
{
    inline std::ostream& operator <<(std::ostream& stream, const TileBounds& value)
    {
        return stream << "TileBounds {" << value.mMin << ", " << value.mMax << "}";
    }

// Use to dump scene to load from recastnavigation demo tool
#ifdef OPENMW_WRITE_OBJ
    void writeObj(const std::vector<float>& vertices, const std::vector<int>& indices);
#endif

#ifdef OPENMW_WRITE_TO_FILE
    class RecastMesh;

    void writeToFile(const RecastMesh& recastMesh, const std::string& revision);

    void writeToFile(const dtNavMesh& navMesh, const std::string& revision);
#endif

    class Log
    {
    public:
        Log() : mEnabled(false) {}

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
                std::cout << text;
        }

        static Log& instance()
        {
            static Log value;
            return value;
        }

    private:
        bool mEnabled;
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
        write(stream, std::forward<Ts>(values) ...);
        log.write(stream.str());
    }
}

#endif
