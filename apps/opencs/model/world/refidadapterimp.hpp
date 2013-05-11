#ifndef CSM_WOLRD_REFIDADAPTERIMP_H
#define CSM_WOLRD_REFIDADAPTERIMP_H

#include "refidadapter.hpp"

namespace CSMWorld
{
    class StaticRefIdAdapter : public RefIdAdapter
    {
            const RefIdColumn *mId;
            const RefIdColumn *mModified;

        public:

            StaticRefIdAdapter (const RefIdColumn *id, const RefIdColumn *modified);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int idnex)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.

            virtual std::string getId (const RecordBase& record) const;
    };
}

#endif
