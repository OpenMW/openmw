
#ifndef MWGUI_TOOLTIPS_H
#define MWGUI_TOOLTIPS_H

#include <openengine/gui/layout.hpp>
#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class WindowManager;

    // Info about tooltip that is supplied by the MWWorld::Class object
    struct ToolTipInfo
    {
    public:
        ToolTipInfo() :
            effects(0)
        {
        };

        std::string caption;
        std::string text;
        std::string icon;

        // enchantment (for cloth, armor, weapons)
        std::string enchant;

        // effects (for potions, ingredients)
        const ESM::EffectList* effects;
    };

    class ToolTips : public OEngine::GUI::Layout
    {
    public:
        ToolTips(WindowManager* windowManager);

        void onFrame(float frameDuration);

        void enterGameMode();
        void enterGuiMode();

        void toggleFullHelp(); ///< show extra info in item tooltips (owner, script)
        bool getFullHelp() const;

        void setFocusObject(const MWWorld::Ptr& focus);

        static std::string getValueString(const int value, const std::string& prefix);
        ///< @return "prefix: value" or "" if value is 0

        static std::string getMiscString(const std::string& text, const std::string& prefix);
        ///< @return "prefix: text" or "" if text is empty

        static std::string toString(const float value);
        static std::string toString(const int value);

    private:
        MyGUI::Widget* mDynamicToolTipBox;

        WindowManager* mWindowManager;

        MWWorld::Ptr mFocusObject;

        void findImageExtension(std::string& image);

        MyGUI::IntSize getToolTipViaPtr ();
        ///< @return requested tooltip size

        MyGUI::IntSize createToolTip(const ToolTipInfo& info);
        ///< @return requested tooltip size

        bool mGameMode;

        bool mFullHelp;
    };
}
#endif
