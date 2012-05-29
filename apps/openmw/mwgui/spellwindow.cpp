#include "spellwindow.hpp"

#include "window_manager.hpp"

namespace MWGui
{
    SpellWindow::SpellWindow(WindowManager& parWindowManager)
        : WindowPinnableBase("openmw_spell_window_layout.xml", parWindowManager)
    {
        getWidget(mSpellView, "SpellView");
        getWidget(mEffectBox, "EffectsBox");

        setCoord(498, 300, 302, 300);
    }

    void SpellWindow::onPinToggled()
    {
        mWindowManager.setSpellVisibility(!mPinned);
    }
}
