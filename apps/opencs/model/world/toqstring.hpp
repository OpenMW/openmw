#ifndef OPENMW_APPS_OPENCS_MODEL_WORLD_TOQSTRING_H
#define OPENMW_APPS_OPENCS_MODEL_WORLD_TOQSTRING_H

#include <components/esm/path.hpp>

#include <QString>

#include <string>

namespace CSMWorld
{
    inline QString toQString(const std::string& value)
    {
        return QString::fromStdString(value);
    }

    inline QString toQString(const ESM::Path& value)
    {
        return QString::fromStdString(value.getOriginal());
    }
}

#endif
