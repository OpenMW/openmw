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
            // Maps mesh part type to 'body part' id
            using RacePartMap = std::unordered_map<ESM::BodyPart::MeshPart, std::string>;

            ActorAdapter(CSMWorld::Data& data);

            const ActorPartMap* getActorPartMap(const std::string& refId);

        signals:

            void actorChanged(const std::string& refId);

        public slots:

            void handleReferenceableChanged(const QModelIndex&, const QModelIndex&);
            void handleRaceChanged(const QModelIndex&, const QModelIndex&);
            void handleBodyPartChanged(const QModelIndex&, const QModelIndex&);

        private:

            RacePartMap& getOrCreateRacePartMap(const std::string& raceId, bool isFemale);

            void updateRaceParts(const std::string& raceId);
            void updateActor(const std::string& refId);
            void updateNpc(const std::string& refId);
            void updateCreature(const std::string& refId);

            RefIdCollection& mReferenceables;
            IdCollection<ESM::Race>& mRaces;
            IdCollection<ESM::BodyPart>& mBodyParts;

            // Key: referenceable id
            std::unordered_map<std::string, ActorPartMap> mActorPartMaps;
            // Key: race id, is female
            std::unordered_map<std::pair<std::string, bool>, RacePartMap, StringBoolPairHash> mRacePartMaps;
    };
}

#endif
