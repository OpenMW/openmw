#ifndef GAME_MWWORLD_LIVECELLREF_H
#define GAME_MWWORLD_LIVECELLREF_H

#include "cellref.hpp"

#include "refdata.hpp"

#include <stdexcept>

namespace ESM
{
    struct ObjectState;
}

namespace MWWorld
{
    class Ptr;
    class ESMStore;
    class Class;

    template <typename X>
    struct LiveCellRef;

    /// Used to create pointers to hold any type of LiveCellRef<> object.
    struct LiveCellRefBase
    {
        const Class* mClass;

        /** Information about this instance, such as 3D location and rotation
         * and individual type-dependent data.
         */
        CellRef mRef;

        /** runtime-data */
        RefData mData;

        LiveCellRefBase(unsigned int type, const ESM::CellRef& cref = ESM::CellRef());
        LiveCellRefBase(unsigned int type, const ESM4::Reference& cref);
        LiveCellRefBase(unsigned int type, const ESM4::ActorCharacter& cref);
        /* Need this for the class to be recognized as polymorphic */
        virtual ~LiveCellRefBase();

        virtual void load(const ESM::ObjectState& state) = 0;
        ///< Load state into a LiveCellRef, that has already been initialised with base and class.
        ///
        /// \attention Must not be called with an invalid \a state.

        virtual void save(ESM::ObjectState& state) const = 0;
        ///< Save LiveCellRef state into \a state.

        virtual std::string_view getTypeDescription() const = 0;

        unsigned int getType() const;
        ///< @see MWWorld::Class::getType

        template <class T>
        static const LiveCellRef<T>* dynamicCast(const LiveCellRefBase* value);

        template <class T>
        static LiveCellRef<T>* dynamicCast(LiveCellRefBase* value);

        /// Returns true if the object was either deleted by the content file or by gameplay.
        bool isDeleted() const;

    protected:
        void loadImp(const ESM::ObjectState& state);
        ///< Load state into a LiveCellRef, that has already been initialised with base and
        /// class.
        ///
        /// \attention Must not be called with an invalid \a state.

        void saveImp(ESM::ObjectState& state) const;
        ///< Save LiveCellRef state into \a state.

        static bool checkStateImp(const ESM::ObjectState& state);
        ///< Check if state is valid and report errors.
        ///
        /// \return Valid?
        ///
        /// \note Does not check if the RefId exists.
    };

    inline bool operator==(const LiveCellRefBase& cellRef, const ESM::RefNum refNum)
    {
        return cellRef.mRef.getRefNum() == refNum;
    }

    std::string makeDynamicCastErrorMessage(const LiveCellRefBase* value, std::string_view recordType);

    template <class T>
    const LiveCellRef<T>* LiveCellRefBase::dynamicCast(const LiveCellRefBase* value)
    {
        if (const LiveCellRef<T>* ref = dynamic_cast<const LiveCellRef<T>*>(value))
            return ref;
        throw std::runtime_error(
            makeDynamicCastErrorMessage(value, ESM::getRecNameString(T::sRecordId).toStringView()));
    }

    template <class T>
    LiveCellRef<T>* LiveCellRefBase::dynamicCast(LiveCellRefBase* value)
    {
        if (LiveCellRef<T>* ref = dynamic_cast<LiveCellRef<T>*>(value))
            return ref;
        throw std::runtime_error(
            makeDynamicCastErrorMessage(value, ESM::getRecNameString(T::sRecordId).toStringView()));
    }

    /// A reference to one object (of any type) in a cell.
    ///
    /// Constructing this with a CellRef instance in the constructor means that
    /// in practice (where D is RefData) the possibly mutable data is copied
    /// across to mData. If later adding data (such as position) to CellRef
    /// this would have to be manually copied across.
    template <typename X>
    struct LiveCellRef : public LiveCellRefBase
    {
        LiveCellRef(const ESM::CellRef& cref, const X* b = nullptr)
            : LiveCellRefBase(X::sRecordId, cref)
            , mBase(b)
        {
        }

        LiveCellRef(const ESM4::Reference& cref, const X* b = nullptr)
            : LiveCellRefBase(X::sRecordId, cref)
            , mBase(b)
        {
        }

        LiveCellRef(const ESM4::ActorCharacter& cref, const X* b = nullptr)
            : LiveCellRefBase(X::sRecordId, cref)
            , mBase(b)
        {
        }

        LiveCellRef(const X* b = nullptr)
            : LiveCellRefBase(X::sRecordId)
            , mBase(b)
        {
        }

        // The object that this instance is based on.
        const X* mBase;

        void load(const ESM::ObjectState& state) override;
        ///< Load state into a LiveCellRef, that has already been initialised with base and class.
        ///
        /// \attention Must not be called with an invalid \a state.

        void save(ESM::ObjectState& state) const override;
        ///< Save LiveCellRef state into \a state.

        std::string_view getTypeDescription() const override
        {
            if constexpr (ESM::isESM4Rec(X::sRecordId))
            {
                static constexpr ESM::FixedString<6> name = ESM::getRecNameString(X::sRecordId);
                return name.toStringView();
            }
            else
                return X::getRecordType();
        }

        static bool checkState(const ESM::ObjectState& state);
        ///< Check if state is valid and report errors.
        ///
        /// \return Valid?
        ///
        /// \note Does not check if the RefId exists.
    };

    template <typename X>
    void LiveCellRef<X>::load(const ESM::ObjectState& state)
    {
        loadImp(state);
    }

    template <typename X>
    void LiveCellRef<X>::save(ESM::ObjectState& state) const
    {
        saveImp(state);
    }

    template <typename X>
    bool LiveCellRef<X>::checkState(const ESM::ObjectState& state)
    {
        return checkStateImp(state);
    }
}

#endif
