#ifndef CSM_WOLRD_ACTORADAPTER_H
#define CSM_WOLRD_ACTORADAPTER_H

#include <functional>
#include <unordered_map>
#include <utility>

#include <QObject>
#include <QModelIndex>

#include <components/esm/loadarmo.hpp>
#include <components/esm/loadbody.hpp>

#include "refidcollection.hpp"
#include "idcollection.hpp"

namespace ESM
{
    struct Race;
    enum PartReferenceType;
}

namespace CSMWorld
{
    class Data;

    /// Quick and dirty hashing functor.
    struct StringBoolPairHash
    {
        size_t operator()(const std::pair<std::string, bool>& value) const noexcept
        {
            auto stringHash = std::hash<std::string>();
            return stringHash(value.first) + value.second;
        }
    };

    class ActorAdapter : public QObject
    {
            Q_OBJECT
        public:

            // Maps body part type to 'body part' id
            using ActorPartMap = std::unordered_map<ESM::PartReferenceType, std::string>;

            ActorAdapter(CSMWorld::Data& data);

            const ActorPartMap* getActorParts(const std::string& refId, bool create=true);

        signals:

            void actorChanged(const std::string& refId);

        public slots:

            void handleReferenceableChanged(const QModelIndex&, const QModelIndex&);
            void handleRaceChanged(const QModelIndex&, const QModelIndex&);
            void handleBodyPartChanged(const QModelIndex&, const QModelIndex&);

        private:
            // Maps mesh part type to 'body part' id
            using RacePartMap = std::unordered_map<ESM::BodyPart::MeshPart, std::string>;
            // Stores ids that are referenced by the actor. Data part is meaningless.
            using DependencyMap = std::unordered_map<std::string, bool>;

            struct ActorData
            {
                ActorPartMap parts;
                DependencyMap dependencies;
            };

            struct RaceData
            {
                RacePartMap femaleParts;
                RacePartMap maleParts;
                DependencyMap dependencies;
            };

            ActorAdapter(const ActorAdapter&) = delete;
            ActorAdapter& operator=(const ActorAdapter&) = delete;

            QModelIndex getHighestIndex(QModelIndex) const;
            bool is1stPersonPart(const std::string& id) const;

            RaceData& getRaceData(const std::string& raceId);

            void updateRace(const std::string& raceId);
            void updateActor(const std::string& refId);
            void updateNpc(const std::string& refId);
            void updateCreature(const std::string& refId);

            void updateActorsWithDependency(const std::string& id);
            void updateRacesWithDependency(const std::string& id);

            RefIdCollection& mReferenceables;
            IdCollection<ESM::Race>& mRaces;
            IdCollection<ESM::BodyPart>& mBodyParts;

            // Key: referenceable id
            std::unordered_map<std::string, ActorData> mCachedActors;
            // Key: race id
            std::unordered_map<std::string, RaceData> mCachedRaces;
    };
}

#endif
