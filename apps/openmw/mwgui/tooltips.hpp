#ifndef MWGUI_TOOLTIPS_H
#define MWGUI_TOOLTIPS_H

#include "layout.hpp"
#include "../mwworld/ptr.hpp"

#include "widgets.hpp"

namespace ESM
{
    struct Class;
    struct Race;
}

namespace MWGui
{
    // Info about tooltip that is supplied by the MWWorld::Class object
    struct ToolTipInfo
    {
    public:
        ToolTipInfo()
            : imageSize(32)
            , remainingEnchantCharge(-1)
            , isPotion(false)
            , isIngredient(false)
            , wordWrap(true)
        {}

        std::string caption;
        std::string text;
        std::string icon;
        int imageSize;

        // enchantment (for cloth, armor, weapons)
        std::string enchant;
        int remainingEnchantCharge;

        // effects (for potions, ingredients)
        Widgets::SpellEffectList effects;

        // local map notes
        std::vector<std::string> notes;

        bool isPotion; // potions do not show target in the tooltip
        bool isIngredient; // ingredients have no effect magnitude
        bool wordWrap;
    };

    class ToolTips : public Layout
    {
    public:
        ToolTips();

        void onFrame(float frameDuration);
        void update(float frameDuration);

        void setEnabled(bool enabled);

        bool toggleFullHelp(); ///< show extra info in item tooltips (owner, script)
        bool getFullHelp() const;

        void setDelay(float delay);

        void clear();

        void setFocusObject(const MWWorld::Ptr& focus);
        void setFocusObjectScreenCoords(float min_x, float min_y, float max_x, float max_y);
        ///< set the screen-space position of the tooltip for focused object

        static std::string getWeightString(const float weight, const std::string& prefix);
        static std::string getPercentString(const float value, const std::string& prefix);
        static std::string getValueString(const int value, const std::string& prefix);
        ///< @return "prefix: value" or "" if value is 0

        static std::string getMiscString(const std::string& text, const std::string& prefix);
        ///< @return "prefix: text" or "" if text is empty

        static std::string toString(const float value);
        static std::string toString(const int value);

        static std::string getCountString(const int value);
        ///< @return blank string if count is 1, or else " (value)"

        static std::string getCellRefString(const MWWorld::CellRef& cellref);
        ///< Returns a string containing debug tooltip information about the given cellref.

        static std::string getDurationString (float duration, const std::string& prefix);
        ///< Returns duration as two largest time units, rounded down. Note: not localized; no line break.

        // these do not create an actual tooltip, but they fill in the data that is required so the tooltip
        // system knows what to show in case this widget is hovered
        static void createSkillToolTip(MyGUI::Widget* widget, int skillId);
        static void createAttributeToolTip(MyGUI::Widget* widget, int attributeId);
        static void createSpecializationToolTip(MyGUI::Widget* widget, const std::string& name, int specId);
        static void createBirthsignToolTip(MyGUI::Widget* widget, const std::string& birthsignId);
        static void createRaceToolTip(MyGUI::Widget* widget, const ESM::Race* playerRace);
        static void createClassToolTip(MyGUI::Widget* widget, const ESM::Class& playerClass);
        static void createMagicEffectToolTip(MyGUI::Widget* widget, short id);
        
        bool checkOwned();
        /// Returns True if taking mFocusObject would be crime
 
    private:
        MyGUI::Widget* mDynamicToolTipBox;

        MWWorld::Ptr mFocusObject;

        MyGUI::IntSize getToolTipViaPtr (int count, bool image = true, bool isOwned = false);
        ///< @return requested tooltip size

        MyGUI::IntSize createToolTip(const ToolTipInfo& info, bool isOwned = false);
        ///< @return requested tooltip size
        /// @param isFocusObject Is the object this tooltips originates from mFocusObject?

        float mFocusToolTipX;
        float mFocusToolTipY;

        /// Adjust position for a tooltip so that it doesn't leave the screen and does not obscure the mouse cursor
        void position(MyGUI::IntPoint& position, MyGUI::IntSize size, MyGUI::IntSize viewportSize);

        static std::string sSchoolNames[6];

        int mHorizontalScrollIndex;


        float mDelay;
        float mRemainingDelay; // remaining time until tooltip will show

        int mLastMouseX;
        int mLastMouseY;

        bool mEnabled;

        bool mFullHelp;
        
        int mShowOwned;

        float mFrameDuration;
    };
}
#endif
