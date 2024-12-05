#ifndef CSM_WOLRD_ACTORADAPTER_H
#define CSM_WOLRD_ACTORADAPTER_H

#include <array>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include <QModelIndex>
#include <QObject>

#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbody.hpp>
#include <components/misc/weakcache.hpp>

#include "idcollection.hpp"

namespace ESM
{
    struct Race;
}

namespace CSMWorld
{
    class Data;
    class RefIdCollection;

    /// Adapts multiple collections to provide the data needed to render
    /// an npc or creature.
    class ActorAdapter : public QObject
    {
        Q_OBJECT
    public:
        /// A list indexed by ESM::PartReferenceType
        using ActorPartList = std::map<ESM::PartReferenceType, std::pair<ESM::RefId, int>>;
        /// A list indexed by ESM::BodyPart::MeshPart
        using RacePartList = std::array<ESM::RefId, ESM::BodyPart::MP_Count>;
        /// Tracks unique strings
        using RefIdSet = std::unordered_set<ESM::RefId>;

        struct WeightsHeights
        {
            osg::Vec2f mMaleWeightHeight;
            osg::Vec2f mFemaleWeightHeight;
        };

        /// Contains base race data shared between actors
        class RaceData
        {
        public:
            RaceData();

            /// Retrieves the id of the race represented
            const ESM::RefId& getId() const;
            /// Checks if it's a beast race
            bool isBeast() const;
            /// Checks if a part could exist for the given type
            bool handlesPart(ESM::PartReferenceType type) const;
            /// Retrieves the associated body part
            const ESM::RefId& getFemalePart(ESM::PartReferenceType index) const;
            /// Retrieves the associated body part
            const ESM::RefId& getMalePart(ESM::PartReferenceType index) const;

            const osg::Vec2f& getGenderWeightHeight(bool isFemale);
            /// Checks if the race has a data dependency
            bool hasDependency(const ESM::RefId& id) const;

            /// Sets the associated part if it's empty and marks a dependency
            void setFemalePart(ESM::BodyPart::MeshPart partIndex, const ESM::RefId& partId);
            /// Sets the associated part if it's empty and marks a dependency
            void setMalePart(ESM::BodyPart::MeshPart partIndex, const ESM::RefId& partId);
            /// Marks an additional dependency
            void addOtherDependency(const ESM::RefId& id);
            /// Clears parts and dependencies
            void reset_data(const ESM::RefId& raceId, const WeightsHeights& raceStats = { { 1.f, 1.f }, { 1.f, 1.f } },
                bool isBeast = false);

        private:
            bool handles(ESM::PartReferenceType type) const;
            ESM::RefId mId;
            bool mIsBeast;
            RacePartList mFemaleParts;
            RacePartList mMaleParts;
            WeightsHeights mWeightsHeights;
            RefIdSet mDependencies;
        };
        using RaceDataPtr = std::shared_ptr<RaceData>;

        /// Contains all the data needed to render an actor. Tracks dependencies
        /// so that pertinent data changes can be checked.
        class ActorData
        {
        public:
            ActorData();

            /// Retrieves the id of the actor represented
            const ESM::RefId& getId() const;
            /// Checks if the actor is a creature
            bool isCreature() const;
            /// Checks if the actor is female
            bool isFemale() const;
            /// Returns the skeleton the actor should use for attaching parts to
            std::string getSkeleton() const;
            /// Retrieves the associated actor part
            ESM::RefId getPart(ESM::PartReferenceType index) const;

            const osg::Vec2f& getRaceWeightHeight() const;
            /// Checks if the actor has a data dependency
            bool hasDependency(const ESM::RefId& id) const;

            /// Sets the actor part used and marks a dependency
            void setPart(ESM::PartReferenceType partIndex, const ESM::RefId& partId, int priority);
            /// Marks an additional dependency for the actor
            void addOtherDependency(const ESM::RefId& id);
            /// Clears race, parts, and dependencies
            void reset_data(const ESM::RefId& actorId, const std::string& skeleton = "", bool isCreature = false,
                bool female = true, RaceDataPtr raceData = nullptr);

        private:
            ESM::RefId mId;
            bool mCreature;
            bool mFemale;
            std::string mSkeletonOverride;
            RaceDataPtr mRaceData;
            ActorPartList mParts;
            RefIdSet mDependencies;
        };
        using ActorDataPtr = std::shared_ptr<ActorData>;

        ActorAdapter(Data& data);

        /// Obtains the shared data for a given actor
        ActorDataPtr getActorData(const ESM::RefId& refId);

    signals:

        void actorChanged(const ESM::RefId& refId);

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

        RaceDataPtr getRaceData(const ESM::RefId& raceId);

        void setupActor(const ESM::RefId& id, ActorDataPtr data);
        void setupRace(const ESM::RefId& id, RaceDataPtr data);

        void setupNpc(const ESM::RefId& id, ActorDataPtr data);
        void addNpcItem(const ESM::RefId& itemId, ActorDataPtr data);

        void setupCreature(const ESM::RefId& id, ActorDataPtr data);

        void markDirtyDependency(const ESM::RefId& dependency);
        void updateDirty();

        RefIdCollection& mReferenceables;
        IdCollection<ESM::Race>& mRaces;
        IdCollection<ESM::BodyPart>& mBodyParts;

        Misc::WeakCache<ESM::RefId, ActorData> mCachedActors; // Key: referenceable id
        Misc::WeakCache<ESM::RefId, RaceData> mCachedRaces; // Key: race id

        RefIdSet mDirtyActors; // Actors that need updating
        RefIdSet mDirtyRaces; // Races that need updating
    };
}

#endif
