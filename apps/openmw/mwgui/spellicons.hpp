#ifndef MWGUI_SPELLICONS_H
#define MWGUI_SPELLICONS_H

#include <string>

#include "../mwmechanics/magiceffects.hpp"

namespace MyGUI
{
    class Widget;
    class ImageBox;
}
namespace ESM
{
    struct ENAMstruct;
    struct EffectList;
}

namespace MWGui
{

    // information about a single magic effect source as required for display in the tooltip
    struct MagicEffectInfo
    {
        MagicEffectInfo() : mPermanent(false) {}
        std::string mSource; // display name for effect source (e.g. potion name)
        MWMechanics::EffectKey mKey;
        int mMagnitude;
        float mRemainingTime;
        bool mPermanent; // the effect is permanent
    };

    class SpellIcons
    {
    public:
        void updateWidgets(MyGUI::Widget* parent, bool adjustSize);

    private:
        std::string getSpellDisplayName (const std::string& id);
        ESM::EffectList getSpellEffectList (const std::string& id);

        std::map<int, MyGUI::ImageBox*> mWidgetMap;
    };

}

#endif
