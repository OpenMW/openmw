#ifndef CSM_WOLRD_UNIVERSALID_H
#define CSM_WOLRD_UNIVERSALID_H

#include <string>
#include <iosfwd>

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
                Type_None,
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
                Type_Reference
            };

        private:

            Class mClass;
            ArgumentType mArgumentType;
            Type mType;
            std::string mId;
            int mIndex;

        public:

            UniversalId (const std::string& universalId);

            UniversalId (Type type = Type_None);
            ///< Using a type for a non-argument-less UniversalId will throw an exception.

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

            static std::pair<int, const char *> getIdArgPair (unsigned int index);
            static unsigned int getIdArgSize ();
    };

    bool operator== (const UniversalId& left, const UniversalId& right);
    bool operator!= (const UniversalId& left, const UniversalId& right);

    bool operator< (const UniversalId& left, const UniversalId& right);

    std::ostream& operator< (std::ostream& stream, const UniversalId& universalId);
}

Q_DECLARE_METATYPE (CSMWorld::UniversalId)

#endif
