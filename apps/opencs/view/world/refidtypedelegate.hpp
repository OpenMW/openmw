#ifndef REFIDTYPEDELEGATE_HPP
#define REFIDTYPEDELEGATE_HPP

#include "enumdelegate.hpp"
#include "util.hpp"
#include "../../model/world/universalid.hpp"
#include "datadisplaydelegate.hpp"

namespace CSVWorld
{
    class RefIdTypeDelegate : public DataDisplayDelegate
    {
        public:
            RefIdTypeDelegate (const ValueList &mValues, const IconList &icons, QUndoStack& undoStack, QObject *parent);

            void updateEditorSetting (const QString &settingName, const QString &settingValue);

    };

    class RefIdTypeDelegateFactory : public DataDisplayDelegateFactory
    {

        typedef std::vector < std::pair <CSMWorld::UniversalId::Type, QString> > UidTypeList;

        public:
            RefIdTypeDelegateFactory();

            virtual CommandDelegate *makeDelegate (QUndoStack& undoStack, QObject *parent) const;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.

    private:
            UidTypeList buildUidTypeList () const;

    };
}

#endif // REFIDTYPEDELEGATE_HPP
