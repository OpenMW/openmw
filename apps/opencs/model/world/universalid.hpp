#ifndef CSM_WOLRD_UNIVERSALID_H
#define CSM_WOLRD_UNIVERSALID_H

#include <string>
#include <iosfwd>
#include <vector>

#include <QMetaType>

namespace CSMWorld
{
    class UniversalId
    {
        public:

            enum Class
            {
                Class_None = 0,
                Class_Record,
                Class_RefRecord, // referenceable record
                Class_SubRecord,
                Class_RecordList,
                Class_Collection, // multiple types of records combined
                Class_Transient, // not part of the world data or the project data
                Class_NonRecord // record like data that is not part of the world
            };

            enum ArgumentType
            {
                ArgumentType_None,
                ArgumentType_Id,
                ArgumentType_Index
            };

            enum Type
            {
                Type_None = 0,
                Type_Globals,
                Type_Global,
                Type_VerificationResults,
                Type_Gmsts,
                Type_Gmst,
                Type_Skills,
                Type_Skill,
                Type_Classes,
                Type_Class,
                Type_Factions,
                Type_Faction,
                Type_Races,
                Type_Race,
                Type_Sounds,
                Type_Sound,
                Type_Scripts,
                Type_Script,
                Type_Regions,
                Type_Region,
                Type_Birthsigns,
                Type_Birthsign,
                Type_Spells,
                Type_Spell,
                Type_Cells,
                Type_Cell,
                Type_Cell_Missing, //For cells that does not exist yet.
                Type_Referenceables,
                Type_Referenceable,
                Type_Activator,
                Type_Potion,
                Type_Apparatus,
                Type_Armor,
                Type_Book,
                Type_Clothing,
                Type_Container,
                Type_Creature,
                Type_Door,
                Type_Ingredient,
                Type_CreatureLevelledList,
                Type_ItemLevelledList,
                Type_Light,
                Type_Lockpick,
                Type_Miscellaneous,
                Type_Npc,
                Type_Probe,
                Type_Repair,
                Type_Static,
                Type_Weapon,
                Type_References,
                Type_Reference,
                Type_RegionMap,
                Type_Filter,
                Type_Filters,
                Type_Topics,
                Type_Topic,
                Type_Journals,
                Type_Journal,
                Type_TopicInfos,
                Type_TopicInfo,
                Type_JournalInfos,
                Type_JournalInfo,
                Type_Scene,
                Type_Preview,
                Type_LoadErrorLog
            };

            enum { NumberOfTypes = Type_LoadErrorLog+1 };

        private:

            Class mClass;
            ArgumentType mArgumentType;
            Type mType;
            std::string mId;
            int mIndex;

        public:

            UniversalId (const std::string& universalId);

            UniversalId (Type type = Type_None);

            UniversalId (Type type, const std::string& id);
            ///< Using a type for a non-ID-argument UniversalId will throw an exception.

            UniversalId (Type type, int index);
            ///< Using a type for a non-index-argument UniversalId will throw an exception.

            Class getClass() const;

            ArgumentType getArgumentType() const;

            Type getType() const;

            const std::string& getId() const;
            ///< Calling this function for a non-ID type will throw an exception.

            int getIndex() const;
            ///< Calling this function for a non-index type will throw an exception.

            bool isEqual (const UniversalId& universalId) const;

            bool isLess (const UniversalId& universalId) const;

            std::string getTypeName() const;

            std::string toString() const;

            std::string getIcon() const;
            ///< Will return an empty string, if no icon is available.

            static std::vector<Type> listReferenceableTypes();
    };

    bool operator== (const UniversalId& left, const UniversalId& right);
    bool operator!= (const UniversalId& left, const UniversalId& right);

    bool operator< (const UniversalId& left, const UniversalId& right);

    std::ostream& operator< (std::ostream& stream, const UniversalId& universalId);
}

Q_DECLARE_METATYPE (CSMWorld::UniversalId)

#endif
