#ifndef OPENMW_GUI_SPELLMODEL_H
#define OPENMW_GUI_SPELLMODEL_H

#include "../mwworld/ptr.hpp"

namespace MWGui
{

    struct Spell
    {
        enum Type
        {
            Type_Power,
            Type_Spell,
            Type_EnchantedItem
        };

        Type mType;
        std::string mName;
        std::string mCostColumn; // Cost/chance or Cost/charge
        std::string mId; // Item ID or spell ID
        MWWorld::Ptr mItem; // Only for Type_EnchantedItem
        int mCount; // Only for Type_EnchantedItem
        bool mSelected; // Is this the currently selected spell/item (only one can be selected at a time)
        bool mActive; // (Items only) is the item equipped?

        Spell()
            : mType(Type_Spell)
            , mCount(0)
            , mSelected(false)
            , mActive(false)
        {
        }
    };

    ///@brief Model that lists all usable powers, spells and enchanted items for an actor.
    class SpellModel
    {
    public:
        SpellModel(const MWWorld::Ptr& actor, const std::string& filter);
        SpellModel(const MWWorld::Ptr& actor);

        typedef int ModelIndex;

        void update();

        Spell getItem (ModelIndex index) const;
        ///< throws for invalid index

        size_t getItemCount() const;
        ModelIndex getSelectedIndex() const;
        ///< returns -1 if nothing is selected

    private:
        MWWorld::Ptr mActor;

        std::vector<Spell> mSpells;

        std::string mFilter;
    };

}

#endif
