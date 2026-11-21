#ifndef CSM_WORLD_VARIANTCONVERSION_H
#define CSM_WORLD_VARIANTCONVERSION_H

#include <string>

#include <QByteArray>
#include <QVariant>

#include <components/esm/refid.hpp>

namespace CSMWorld
{
    inline std::string getString(const QVariant& value)
    {
        return value.toString().toUtf8().constData();
    }

    inline ESM::RefId getRefId(const QVariant& value)
    {
        return ESM::RefId::stringRefId(value.toString().toUtf8().constData());
    }
}

#endif
