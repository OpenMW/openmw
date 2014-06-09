#ifndef CSV_DOC_SUBVIEW_H
#define CSV_DOC_SUBVIEW_H

#include "../../model/doc/document.hpp"

#include "../../model/world/universalid.hpp"

#include "subviewfactory.hpp"

#include <QDockWidget>

class QUndoStack;

namespace CSMWorld
{
    class Data;
}

namespace CSVDoc
{
    class SubView : public QDockWidget
    {
            Q_OBJECT

            CSMWorld::UniversalId mUniversalId;

            // not implemented
            SubView (const SubView&);
            SubView& operator= (SubView&);
        protected:
            void setUniversalId(const CSMWorld::UniversalId& id);

        public:

            SubView (const CSMWorld::UniversalId& id);

            CSMWorld::UniversalId getUniversalId() const;

            virtual void setEditLock (bool locked) = 0;

            virtual void setStatusBar (bool show);
            ///< Default implementation: ignored

            virtual void useHint (const std::string& hint);
            ///< Default implementation: ignored

        signals:

            void focusId (const CSMWorld::UniversalId& universalId, const std::string& hint);

        public slots:
            virtual void updateUserSetting
                                    (const QString &, const QStringList &);
    };
}

#endif
