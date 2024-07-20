#ifndef CSM_WOLRD_DISABLETAG_H
#define CSM_WOLRD_DISABLETAG_H

#include <QVariant>

namespace CSMWorld
{
    class DisableTag
    {
    public:
        static QVariant getVariant() { return QVariant::fromValue(CSMWorld::DisableTag()); }

        static bool isDisableTag(const QVariant& variant)
        {
            return strcmp(variant.typeName(), "CSMWorld::DisableTag") == 0;
        }
    };
}

Q_DECLARE_METATYPE(CSMWorld::DisableTag)

#endif
