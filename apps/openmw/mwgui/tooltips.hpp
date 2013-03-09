
#ifndef MWGUI_TOOLTIPS_H
#define MWGUI_TOOLTIPS_H

#include <openengine/gui/layout.hpp>
#include "../mwworld/ptr.hpp"

#include "widgets.hpp"

namespace MWGui
{
    // Info about tooltip that is supplied by the MWWorld::Class object
    struct ToolTipInfo
    {
    public:
        ToolTipInfo()
            : isPotion(false)
            , imageSize(32)
            , wordWrap(true)
        {}

        std::string caption;
        std::string text;
        std::string icon;
        int imageSize;

        // enchantment (for cloth, armor, weapons)
        std::string enchant;

        // effects (for potions, ingredients)
        Widgets::SpellEffectList effects;

        bool isPotion; // potions do not show target in the tooltip
        bool wordWrap;
    };

    class ToolTips : public OEngine::GUI::Layout
    {
    public:
        ToolTips(MWBase::WindowManager* windowManager);

        void onFrame(float frameDuration);

        void enterGameMode();
        void enterGuiMode();

        void setEnabled(bool enabled);

        void toggleFullHelp(); ///< show extra info in item tooltips (owner, script)
        bool getFullHelp() const;

        void setDelay(float delay);

        void setFocusObject(const MWWorld::Ptr& focus);
        void setFocusObjectScreenCoords(float min_x, float min_y, float max_x, float max_y);
        ///< set the screen-space position of the tooltip for focused object

        static std::string getValueString(const int value, const std::string& prefix);
        ///< @return "prefix: value" or "" if value is 0

        static std::string getMiscString(const std::string& text, const std::string& prefix);
        ///< @return "prefix: text" or "" if text is empty

        static std::string toString(const float value);
        static std::string toString(const int value);

        static std::string getCountString(const int value);
        ///< @return blank string if count is 1, or else " (value)"

        // these do not create an actual tooltip, but they fill in the data that is required so the tooltip
        // system knows what to show in case this widget is hovered
        static void createSkillToolTip(MyGUI::Widget* widget, int skillId);
        static void createAttributeToolTip(MyGUI::Widget* widget, int attributeId);
        static void createSpecializationToolTip(MyGUI::Widget* widget, const std::string& name, int specId);
        static void createBirthsignToolTip(MyGUI::Widget* widget, const std::string& birthsignId);
        static void createRaceToolTip(MyGUI::Widget* widget, const ESM::Race* playerRace);
        static void createClassToolTip(MyGUI::Widget* widget, const ESM::Class& playerClass);
        static void createMagicEffectToolTip(MyGUI::Widget* widget, short id);

    private:
        MyGUI::Widget* mDynamicToolTipBox;

        MWBase::WindowManager* mWindowManager;

        MWWorld::Ptr mFocusObject;

        void findImageExtension(std::string& image);

        MyGUI::IntSize getToolTipViaPtr (bool image=true);
        ///< @return requested tooltip size

        MyGUI::IntSize createToolTip(const ToolTipInfo& info);
        ///< @return requested tooltip size

        float mFocusToolTipX;
        float mFocusToolTipY;
	
	int mHorizontalScrollIndex;
	

        float mDelay;
        float mRemainingDelay; // remaining time until tooltip will show

        int mLastMouseX;
        int mLastMouseY;

        bool mGameMode;

        bool mEnabled;

        bool mFullHelp;
    };
}
#endif
