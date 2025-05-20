#ifndef GAME_MWWORLD_CELLSTORE_H
#define GAME_MWWORLD_CELLSTORE_H

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <typeinfo>
#include <vector>

#include "cell.hpp"
#include "cellreflist.hpp"
#include "livecellref.hpp"

#include <components/esm/refid.hpp>
#include <components/esm3/fogstate.hpp>
#include <components/misc/tuplemeta.hpp>

#include "ptr.hpp"
#include "timestamp.hpp"

namespace ESM
{
    class ReadersCache;
    struct Cell;
    struct CellState;
    struct FormId;
    using RefNum = FormId;
    struct Activator;
    struct Potion;
    struct Apparatus;
    struct Armor;
    struct Book;
    struct Clothing;
    struct Container;
    struct Creature;
    struct Door;
    struct Ingredient;
    struct CreatureLevList;
    struct ItemLevList;
    struct Light;
    struct Lockpick;
    struct Miscellaneous;
    struct NPC;
    struct Probe;
    struct Repair;
    struct Static;
    struct Weapon;
    struct BodyPart;
    struct CellCommon;
}

namespace ESM4
{
    class Reader;
    struct Cell;
    struct Reference;
    struct ActorCharacter;
    struct Static;
    struct Light;
    struct Activator;
    struct Potion;
    struct Ammunition;
    struct Armor;
    struct Book;
    struct Clothing;
    struct Container;
    struct Door;
    struct Furniture;
    struct Flora;
    struct Ingredient;
    struct ItemMod;
    struct MiscItem;
    struct MovableStatic;
    struct StaticCollection;
    struct Terminal;
    struct Tree;
    struct Weapon;
    struct Creature;
    struct Npc;
}

namespace MWWorld
{
    class ESMStore;
    struct CellStoreImp;

    using CellStoreTuple = std::tuple<CellRefList<ESM::Activator>, CellRefList<ESM::Potion>,
        CellRefList<ESM::Apparatus>, CellRefList<ESM::Armor>, CellRefList<ESM::Book>, CellRefList<ESM::Clothing>,
        CellRefList<ESM::Container>, CellRefList<ESM::Creature>, CellRefList<ESM::Door>, CellRefList<ESM::Ingredient>,
        CellRefList<ESM::CreatureLevList>, CellRefList<ESM::ItemLevList>, CellRefList<ESM::Light>,
        CellRefList<ESM::Lockpick>, CellRefList<ESM::Miscellaneous>, CellRefList<ESM::NPC>, CellRefList<ESM::Probe>,
        CellRefList<ESM::Repair>, CellRefList<ESM::Static>, CellRefList<ESM::Weapon>, CellRefList<ESM::BodyPart>,

        CellRefList<ESM4::Static>, CellRefList<ESM4::Light>, CellRefList<ESM4::Activator>, CellRefList<ESM4::Potion>,
        CellRefList<ESM4::Ammunition>, CellRefList<ESM4::Armor>, CellRefList<ESM4::Book>, CellRefList<ESM4::Clothing>,
        CellRefList<ESM4::Container>, CellRefList<ESM4::Door>, CellRefList<ESM4::Flora>, CellRefList<ESM4::Ingredient>,
        CellRefList<ESM4::ItemMod>, CellRefList<ESM4::Terminal>, CellRefList<ESM4::Tree>, CellRefList<ESM4::MiscItem>,
        CellRefList<ESM4::MovableStatic>, CellRefList<ESM4::Weapon>, CellRefList<ESM4::Furniture>,
        CellRefList<ESM4::Creature>, CellRefList<ESM4::Npc>, CellRefList<ESM4::StaticCollection>>;

    /// \brief Mutable state of a cell
    class CellStore
    {
    public:
        enum State
        {
            State_Unloaded,
            State_Preloaded,
            State_Loaded
        };

        /// Should this reference be accessible to the outside world (i.e. to scripts / game logic)?
        /// Determined based on the deletion flags. By default, objects deleted by content files are never accessible;
        /// objects deleted by setCount(0) are still accessible *if* they came from a content file (needed for vanilla
        /// scripting compatibility, and the fact that objects may be "un-deleted" in the original game).
        static bool isAccessible(const MWWorld::RefData& refdata, const MWWorld::CellRef& cref)
        {
            return !refdata.isDeletedByContentFile() && (cref.hasContentFile() || cref.getCount() > 0);
        }

        /// Moves object from this cell to the given cell.
        /// @note automatically updates given cell by calling cellToMoveTo->moveFrom(...)
        /// @note throws exception if cellToMoveTo == this
        /// @return updated MWWorld::Ptr with the new CellStore pointer set.
        MWWorld::Ptr moveTo(const MWWorld::Ptr& object, MWWorld::CellStore* cellToMoveTo);

        void rest(double hours);
        void recharge(float duration);

        /// Make a copy of the given object and insert it into this cell.
        /// @note If you get a linker error here, this means the given type can not be inserted into a cell.
        /// The supported types are defined at the bottom of this file.
        template <typename T>
        LiveCellRefBase* insert(const LiveCellRef<T>* ref)
        {
            mHasState = true;
            CellRefList<T>& list = get<T>();
            LiveCellRefBase* ret = &list.insert(*ref);
            requestMergedRefsUpdate();
            return ret;
        }

        /// @param readerList The readers to use for loading of the cell on-demand.
        CellStore(MWWorld::Cell&& cell, const MWWorld::ESMStore& store, ESM::ReadersCache& readers);

        CellStore(const CellStore&) = delete;

        CellStore(CellStore&&) = delete;

        CellStore& operator=(const CellStore&) = delete;

        CellStore& operator=(CellStore&&) = delete;

        ~CellStore();

        const MWWorld::Cell* getCell() const;

        State getState() const;

        const std::vector<ESM::RefId>& getPreloadedIds() const;
        ///< Get Ids of objects in this cell, only valid in State_Preloaded

        bool hasState() const;
        ///< Does this cell have state that needs to be stored in a saved game file?

        bool hasId(const ESM::RefId& id) const;
        ///< May return true for deleted IDs when in preload state. Will return false, if cell is
        /// unloaded.
        /// @note Will not account for moved references which may exist in Loaded state. Use search() instead if the
        /// cell is loaded.

        Ptr search(const ESM::RefId& id);
        ///< Will return an empty Ptr if cell is not loaded. Does not check references in
        /// containers.
        /// @note Triggers CellStore hasState flag.

        ConstPtr searchConst(const ESM::RefId& id) const;
        ///< Will return an empty Ptr if cell is not loaded. Does not check references in
        /// containers.
        /// @note Does not trigger CellStore hasState flag.

        Ptr searchViaActorId(int id);
        ///< Will return an empty Ptr if cell is not loaded.

        float getWaterLevel() const;

        bool movedHere(const MWWorld::Ptr& ptr) const;

        void setWaterLevel(float level);

        void setFog(std::unique_ptr<ESM::FogState>&& fog);
        ///< \note Takes ownership of the pointer

        ESM::FogState* getFog() const;

        std::size_t count() const;
        ///< Return total number of references, including deleted ones.

        void load();
        ///< Load references from content file.

        void preload();
        ///< Build ID list from content file.

        /// Call visitor (MWWorld::Ptr) for each reference. visitor must return a bool. Returning
        /// false will abort the iteration.
        /// \note Prefer using forEachConst when possible.
        /// \note Do not modify this cell (i.e. remove/add objects) during the forEach, doing this may result in
        /// unintended behaviour. \attention This function also lists deleted (count 0) objects!
        /// \return Iteration completed?
        template <class Visitor>
        bool forEach(Visitor&& visitor, bool includeDeleted = false)
        {
            if (mState != State_Loaded)
                return false;

            if (mMergedRefsNeedsUpdate)
                updateMergedRefs(includeDeleted);
            if (mMergedRefs.empty())
                return true;

            mHasState = true;

            for (LiveCellRefBase* mergedRef : mMergedRefs)
            {
                if (!includeDeleted && !isAccessible(mergedRef->mData, mergedRef->mRef))
                    continue;

                if (!visitor(MWWorld::Ptr(mergedRef, this)))
                    return false;
            }
            return true;
        }

        /// Call visitor (MWWorld::ConstPtr) for each reference. visitor must return a bool. Returning
        /// false will abort the iteration.
        /// \note Do not modify this cell (i.e. remove/add objects) during the forEach, doing this may result in
        /// unintended behaviour. \attention This function also lists deleted (count 0) objects!
        /// \return Iteration completed?
        template <class Visitor>
        bool forEachConst(Visitor&& visitor, bool includeDeleted = false) const
        {
            if (mState != State_Loaded)
                return false;

            if (mMergedRefsNeedsUpdate)
                updateMergedRefs(includeDeleted);

            for (const LiveCellRefBase* mergedRef : mMergedRefs)
            {
                if (!includeDeleted && !isAccessible(mergedRef->mData, mergedRef->mRef))
                    continue;

                if (!visitor(MWWorld::ConstPtr(mergedRef, this)))
                    return false;
            }
            return true;
        }

        /// Call visitor (ref) for each reference of given type. visitor must return a bool. Returning
        /// false will abort the iteration.
        /// \note Do not modify this cell (i.e. remove/add objects) during the forEach, doing this may result in
        /// unintended behaviour. \attention This function also lists deleted (count 0) objects!
        /// \return Iteration completed?
        template <class T, class Visitor>
        bool forEachType(Visitor&& visitor, bool includeDeleted = false)
        {
            if (mState != State_Loaded)
                return false;

            if (mMergedRefsNeedsUpdate)
                updateMergedRefs(includeDeleted);
            if (mMergedRefs.empty())
                return true;

            mHasState = true;

            for (LiveCellRefBase& base : get<T>().mList)
            {
                if (mMovedToAnotherCell.contains(&base))
                    continue;
                if (!includeDeleted && !isAccessible(base.mData, base.mRef))
                    continue;
                if (!visitor(MWWorld::Ptr(&base, this)))
                    return false;
            }

            for (MovedRefTracker::const_iterator it = mMovedHere.begin(); it != mMovedHere.end(); ++it)
            {
                LiveCellRefBase* base = it->first;
                if (dynamic_cast<LiveCellRef<T>*>(base))
                    if (!visitor(MWWorld::Ptr(base, this)))
                        return false;
            }
            return true;
        }

        // NOTE: does not account for moved references
        // Should be phased out when we have const version of forEach
        inline const CellRefList<ESM::Door>& getReadOnlyDoors() const { return get<ESM::Door>(); }
        inline const CellRefList<ESM4::Door>& getReadOnlyEsm4Doors() const { return get<ESM4::Door>(); }
        inline const CellRefList<ESM::Static>& getReadOnlyStatics() const { return get<ESM::Static>(); }
        inline const CellRefList<ESM4::Static>& getReadOnlyEsm4Statics() const { return get<ESM4::Static>(); }

        bool isExterior() const;

        bool isQuasiExterior() const;

        Ptr searchInContainer(const ESM::RefId& id);

        void loadState(const ESM::CellState& state);

        void saveState(ESM::CellState& state) const;

        void writeFog(ESM::ESMWriter& writer) const;

        void readFog(ESM::ESMReader& reader);

        void writeReferences(ESM::ESMWriter& writer) const;

        struct GetCellStoreCallback
        {
            ///@note must return nullptr if the cell is not found
            virtual CellStore* getCellStore(const ESM::RefId& cellId) = 0;
            virtual ~GetCellStoreCallback() = default;
        };

        /// @param callback to use for retrieving of additional CellStore objects by ID (required for resolving moved
        /// references)
        void readReferences(ESM::ESMReader& reader, GetCellStoreCallback* callback);

        void respawn();
        ///< Check mLastRespawn and respawn references if necessary. This is a no-op if the cell is not loaded.

        Ptr getMovedActor(int actorId) const;

        Ptr getPtr(ESM::RefId id);

    private:
        friend struct CellStoreImp;

        const MWWorld::ESMStore& mStore;
        ESM::ReadersCache& mReaders;

        // Even though fog actually belongs to the player and not cells,
        // it makes sense to store it here since we need it once for each cell.
        // Note this is nullptr until the cell is explored to save some memory
        std::unique_ptr<ESM::FogState> mFogState;

        MWWorld::Cell mCellVariant;
        State mState;
        bool mHasState;
        std::vector<ESM::RefId> mIds;
        float mWaterLevel;

        MWWorld::TimeStamp mLastRespawn;

        template <typename T>
        static constexpr std::size_t getTypeIndex()
        {
            static_assert(Misc::TupleHasType<CellRefList<T>, CellStoreTuple>::value);
            return Misc::TupleTypeIndex<CellRefList<T>, CellStoreTuple>::value;
        }

        std::unique_ptr<CellStoreImp> mCellStoreImp;
        std::vector<CellRefListBase*> mCellRefLists;

        template <class T>
        CellRefList<T>& get()
        {
            mHasState = true;
            return static_cast<CellRefList<T>&>(*mCellRefLists[getTypeIndex<T>()]);
        }

        template <class T>
        const CellRefList<T>& get() const
        {
            return static_cast<const CellRefList<T>&>(*mCellRefLists[getTypeIndex<T>()]);
        }

        typedef std::map<LiveCellRefBase*, MWWorld::CellStore*> MovedRefTracker;
        // References owned by a different cell that have been moved here.
        // <reference, cell the reference originally came from>
        MovedRefTracker mMovedHere;
        // References owned by this cell that have been moved to another cell.
        // <reference, cell the reference was moved to>
        MovedRefTracker mMovedToAnotherCell;

        // Merged list of ref's currently in this cell - i.e. with added refs from mMovedHere, removed refs from
        // mMovedToAnotherCell
        mutable std::vector<LiveCellRefBase*> mMergedRefs;
        mutable bool mMergedRefsNeedsUpdate = false;

        // Get the Ptr for the given ref which originated from this cell (possibly moved to another cell at this point).
        Ptr getCurrentPtr(MWWorld::LiveCellRefBase* ref);

        /// Moves object from the given cell to this cell.
        void moveFrom(const MWWorld::Ptr& object, MWWorld::CellStore* from);

        /// Repopulate mMergedRefs.
        void requestMergedRefsUpdate();
        void updateMergedRefs(bool includeDeleted = false) const;

        // (item, max charge)
        typedef std::vector<std::pair<LiveCellRefBase*, float>> TRechargingItems;
        TRechargingItems mRechargingItems;

        bool mRechargingItemsUpToDate;

        void updateRechargingItems();
        void rechargeItems(float duration);
        void checkItem(const Ptr& ptr);

        /// Run through references and store IDs
        void listRefs(const ESM::Cell& cell);
        void listRefs(const ESM4::Cell& cell);
        void listRefs();

        void loadRefs(const ESM::Cell& cell, std::map<ESM::RefNum, ESM::RefId>& refNumToID);
        void loadRefs(const ESM4::Cell& cell, std::map<ESM::RefNum, ESM::RefId>& refNumToID);

        void loadRefs();

        void loadRef(const ESM4::Reference& ref);
        void loadRef(const ESM4::ActorCharacter& ref);
        void loadRef(ESM::CellRef& ref, bool deleted, std::map<ESM::RefNum, ESM::RefId>& refNumToID);
        ///< Make case-adjustments to \a ref and insert it into the respective container.
        ///
        /// Invalid \a ref objects are silently dropped.
        ///
    };

}

#endif
