#ifndef CSM_WORLD_NPCAUTOCALC_H
#define CSM_WORLD_NPCAUTOCALC_H

#include <string>
#include <map>

#include <QObject>
#include <QModelIndex>

namespace ESM
{
    struct NPC;
}

namespace CSMWorld
{
    class Data;
    class NpcStats;
    class IdTable;
    class IdTree;

    class NpcAutoCalc : public QObject
    {
            Q_OBJECT

            const Data&    mData;
            const IdTable *mSkillModel;
            const IdTable *mClassModel;
            const IdTree  *mRaceModel;
            mutable std::map<const std::string, NpcStats*> mNpcStatCache;

        public:

            NpcAutoCalc (const Data& data, const IdTable *gmsts, const IdTable *skills, const IdTable *classes,
                    const IdTree *races, const IdTree *objects);

            ~NpcAutoCalc ();

            NpcStats* npcAutoCalculate (const ESM::NPC& npc) const;

        private:

            // not implemented
            NpcAutoCalc (const NpcAutoCalc&);
            NpcAutoCalc& operator= (const NpcAutoCalc&);

            NpcStats* getCachedNpcData (const std::string& id) const;

            void clearNpcStatsCache ();

        signals:

            // refresh NPC dialogue subviews via object table model
            void updateNpcAutocalc (int type, const std::string& id);

            //void cacheNpcStats (const std::string& id, NpcStats *stats) const;

        private slots:

            // for autocalc updates when gmst/race/class/skils tables change
            void gmstDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void raceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void classDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void skillDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void npcDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            //void cacheNpcStatsEvent (const std::string& id, NpcStats *stats);
    };
}

#endif // CSM_WORLD_NPCAUTOCALC_H
