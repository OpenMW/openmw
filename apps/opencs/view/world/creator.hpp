#ifndef CSV_WORLD_CREATOR_H
#define CSV_WORLD_CREATOR_H

#include <memory>

#include <QWidget>

#ifndef Q_MOC_RUN
#include "../../model/doc/document.hpp"

#include "../../model/world/scope.hpp"
#include "../../model/world/universalid.hpp"
#endif

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    /// \brief Record creator UI base class
    class Creator : public QWidget
    {
            Q_OBJECT

        public:

            virtual ~Creator();

            virtual void reset() = 0;

            virtual void cloneMode(const std::string& originId,
                                   const CSMWorld::UniversalId::Type type) = 0;

            /// Touches a record, if the creator supports it.
            virtual void touch(const std::vector<CSMWorld::UniversalId>& ids) = 0;

            virtual void setEditLock (bool locked) = 0;

            virtual void toggleWidgets(bool active = true) = 0;

            /// Default implementation: Throw an exception if scope!=Scope_Content.
            virtual void setScope (unsigned int scope);

            /// Focus main input widget
            virtual void focus() = 0;

        signals:

            void done();

            void requestFocus (const std::string& id);
            ///< Request owner of this creator to focus the just created \a id. The owner may
            /// ignore this request.
    };

    /// \brief Base class for Creator factory
    class CreatorFactoryBase
    {
        public:

            virtual ~CreatorFactoryBase();

            virtual Creator *makeCreator (CSMDoc::Document& document, const CSMWorld::UniversalId& id) const = 0;
            ///< The ownership of the returned Creator is transferred to the caller.
            ///
            /// \note The function can return a 0-pointer, which means no UI for creating/deleting
            /// records should be provided.
    };

    /// \brief Creator factory that does not produces any creator
    class NullCreatorFactory : public CreatorFactoryBase
    {
        public:

            virtual Creator *makeCreator (CSMDoc::Document& document, const CSMWorld::UniversalId& id) const;
            ///< The ownership of the returned Creator is transferred to the caller.
            ///
            /// \note The function always returns 0.
    };

    template<class CreatorT, unsigned int scope = CSMWorld::Scope_Content>
    class CreatorFactory : public CreatorFactoryBase
    {
        public:

            virtual Creator *makeCreator (CSMDoc::Document& document, const CSMWorld::UniversalId& id) const;
            ///< The ownership of the returned Creator is transferred to the caller.
            ///
            /// \note The function can return a 0-pointer, which means no UI for creating/deleting
            /// records should be provided.
    };

    template<class CreatorT, unsigned int scope>
    Creator *CreatorFactory<CreatorT, scope>::makeCreator (CSMDoc::Document& document,
                                                           const CSMWorld::UniversalId& id) const
    {
        std::unique_ptr<CreatorT> creator (new CreatorT (document.getData(), document.getUndoStack(), id));

        creator->setScope (scope);

        return creator.release();
    }
}

#endif
