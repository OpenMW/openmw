
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
        std::string caption;
        std::string text;
        std::string icon;

        /// \todo enchantments (armor, cloth, weapons), magic effects (potions, ingredients)
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

        MyGUI::IntSize createToolTip(const std::string& caption, const std::string& image, const int imageSize, const std::string& text);
        ///< @return requested tooltip size

        bool mGameMode;

        bool mFullHelp;
    };
}
#endif
