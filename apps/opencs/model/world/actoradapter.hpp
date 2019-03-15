#ifndef CSM_WOLRD_ACTORADAPTER_H
#define CSM_WOLRD_ACTORADAPTER_H

#include <array>
#include <map>
#include <unordered_set>

#include <QObject>
#include <QModelIndex>

#include <components/esm/loadarmo.hpp>
#include <components/esm/loadbody.hpp>
#include <components/misc/weakcache.hpp>

#include "refidcollection.hpp"
#include "idcollection.hpp"

namespace ESM
{
    struct Race;
}

namespace CSMWorld
{
    class Data;

    /// Adapts multiple collections to provide the data needed to render
    /// an npc or creature.
    class ActorAdapter : public QObject
    {
        Q_OBJECT
    public:

        /// A list indexed by ESM::PartReferenceType
        using ActorPartList = std::map<ESM::PartReferenceType, std::pair<std::string, int>>;
        /// A list indexed by ESM::BodyPart::MeshPart
        using RacePartList = std::array<std::string, ESM::BodyPart::MP_Count>;
        /// Tracks unique strings
        using StringSet = std::unordered_set<std::string>;


        /// Contains base race data shared between actors
        class RaceData
        {
        public:
            RaceData();

            /// Retrieves the id of the race represented
            const std::string& getId() const;
            /// Checks if it's a beast race
            bool isBeast() const;
            /// Checks if a part could exist for the given type
            bool handlesPart(ESM::PartReferenceType type) const;
            /// Retrieves the associated body part
            const std::string& getFemalePart(ESM::PartReferenceType index) const;
            /// Retrieves the associated body part
            const std::string& getMalePart(ESM::PartReferenceType index) const;
            /// Checks if the race has a data dependency
            bool hasDependency(const std::string& id) const;

            /// Sets the associated part if it's empty and marks a dependency
            void setFemalePart(ESM::BodyPart::MeshPart partIndex, const std::string& partId);
            /// Sets the associated part if it's empty and marks a dependency
            void setMalePart(ESM::BodyPart::MeshPart partIndex, const std::string& partId);
            /// Marks an additional dependency
            void addOtherDependency(const std::string& id);
            /// Clears parts and dependencies
            void reset_data(const std::string& raceId, bool isBeast=false);

        private:
            bool handles(ESM::PartReferenceType type) const;
            std::string mId;
            bool mIsBeast;
            RacePartList mFemaleParts;
            RacePartList mMaleParts;
            StringSet mDependencies;
        };
        using RaceDataPtr = std::shared_ptr<RaceData>;

        /// Contains all the data needed to render an actor. Tracks dependencies
        /// so that pertinent data changes can be checked.
        class ActorData
        {
        public:
            ActorData();

            /// Retrieves the id of the actor represented
            const std::string& getId() const;
            /// Checks if the actor is a creature
            bool isCreature() const;
            /// Checks if the actor is female
            bool isFemale() const;
            /// Returns the skeleton the actor should use for attaching parts to
            std::string getSkeleton() const;
            /// Retrieves the associated actor part
            const std::string getPart(ESM::PartReferenceType index) const;
            /// Checks if the actor has a data dependency
            bool hasDependency(const std::string& id) const;

            /// Sets the actor part used and marks a dependency
            void setPart(ESM::PartReferenceType partIndex, const std::string& partId, int priority);
            /// Marks an additional dependency for the actor
            void addOtherDependency(const std::string& id);
            /// Clears race, parts, and dependencies
            void reset_data(const std::string& actorId, const std::string& skeleton="", bool isCreature=false, bool female=true, RaceDataPtr raceData=nullptr);

        private:
            std::string mId;
            bool mCreature;
            bool mFemale;
            std::string mSkeletonOverride;
            RaceDataPtr mRaceData;
            ActorPartList mParts;
            StringSet mDependencies;
        };
        using ActorDataPtr = std::shared_ptr<ActorData>;


        ActorAdapter(Data& data);

        /// Obtains the shared data for a given actor
        ActorDataPtr getActorData(const std::string& refId);

    signals:

        void actorChanged(const std::string& refId);

    public slots:

        void handleReferenceablesInserted(const QModelIndex&, int, int);
        void handleReferenceableChanged(const QModelIndex&, const QModelIndex&);
        void handleReferenceablesAboutToBeRemoved(const QModelIndex&, int, int);
        void handleReferenceablesRemoved(const QModelIndex&, int, int);

        void handleRacesInserted(const QModelIndex&, int, int);
        void handleRaceChanged(const QModelIndex&, const QModelIndex&);
        void handleRacesAboutToBeRemoved(const QModelIndex&, int, int);
        void handleRacesRemoved(const QModelIndex&, int, int);

        void handleBodyPartsInserted(const QModelIndex&, int, int);
        void handleBodyPartChanged(const QModelIndex&, const QModelIndex&);
        void handleBodyPartsAboutToBeRemoved(const QModelIndex&, int, int);
        void handleBodyPartsRemoved(const QModelIndex&, int, int);

    private:

        ActorAdapter(const ActorAdapter&) = delete;
        ActorAdapter& operator=(const ActorAdapter&) = delete;

        QModelIndex getHighestIndex(QModelIndex) const;
        bool is1stPersonPart(const std::string& id) const;

        RaceDataPtr getRaceData(const std::string& raceId);

        void setupActor(const std::string& id, ActorDataPtr data);
        void setupRace(const std::string& id, RaceDataPtr data);

        void setupNpc(const std::string& id, ActorDataPtr data);
        void addNpcItem(const std::string& itemId, ActorDataPtr data);

        void setupCreature(const std::string& id, ActorDataPtr data);

        void markDirtyDependency(const std::string& dependency);
        void updateDirty();

        RefIdCollection& mReferenceables;
        IdCollection<ESM::Race>& mRaces;
        IdCollection<ESM::BodyPart>& mBodyParts;

        Misc::WeakCache<std::string, ActorData> mCachedActors; // Key: referenceable id
        Misc::WeakCache<std::string, RaceData> mCachedRaces; // Key: race id

        StringSet mDirtyActors; // Actors that need updating
        StringSet mDirtyRaces; // Races that need updating
    };
}

#endif
