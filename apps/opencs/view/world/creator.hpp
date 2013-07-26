#ifndef CSV_WORLD_CREATOR_H
#define CSV_WORLD_CREATOR_H

namespace CSVWorld
{
    /// \brief Record creator UI base class
    class Creator
    {
        public:

            virtual ~Creator();
    };

    /// \brief Base class for Creator factory
    class CreatorFactoryBase
    {
        public:

            virtual ~CreatorFactoryBase();

            virtual Creator *makeCreator() const = 0;
            ///< The ownership of the returned Creator is transferred to the caller.
            ///
            /// \note The function can return a 0-pointer, which means no UI for creating/deleting
            /// records should be provided.
    };

    /// \brief Creator factory that does not produces any creator
    class NullCreatorFactory : public CreatorFactoryBase
    {
        public:

            virtual Creator *makeCreator() const;
            ///< The ownership of the returned Creator is transferred to the caller.
            ///
            /// \note The function always returns 0.
    };

    template<class CreatorT>
    class CreatorFactory : public CreatorFactoryBase
    {
        public:

            virtual Creator *makeCreator() const;
            ///< The ownership of the returned Creator is transferred to the caller.
            ///
            /// \note The function can return a 0-pointer, which means no UI for creating/deleting
            /// records should be provided.
    };

    template<class CreatorT>
    Creator *CreatorFactory<CreatorT>::makeCreator() const
    {
        return new CreatorT;
    }
}

#endif