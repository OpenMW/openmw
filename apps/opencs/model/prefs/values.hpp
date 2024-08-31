#ifndef OPENMW_APPS_OPENCS_MODEL_PREFS_VALUES_H
#define OPENMW_APPS_OPENCS_MODEL_PREFS_VALUES_H

#include "enumvalueview.hpp"

#include <components/settings/sanitizer.hpp>
#include <components/settings/settingvalue.hpp>

#include <Qt>
#include <QtGlobal>

#include <array>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace CSMPrefs
{
    class EnumSanitizer final : public Settings::Sanitizer<std::string>
    {
    public:
        explicit EnumSanitizer(std::span<const EnumValueView> values)
            : mValues(values)
        {
        }

        std::string apply(const std::string& value) const override
        {
            const auto hasValue = [&](const EnumValueView& v) { return v.mValue == value; };
            if (std::find_if(mValues.begin(), mValues.end(), hasValue) == mValues.end())
            {
                std::ostringstream message;
                message << "Invalid enum value: " << value;
                throw std::runtime_error(message.str());
            }
            return value;
        }

    private:
        std::span<const EnumValueView> mValues;
    };

    inline std::unique_ptr<Settings::Sanitizer<std::string>> makeEnumSanitizerString(
        std::span<const EnumValueView> values)
    {
        return std::make_unique<EnumSanitizer>(values);
    }

    class EnumSettingValue
    {
    public:
        explicit EnumSettingValue(Settings::Index& index, std::string_view category, std::string_view name,
            std::span<const EnumValueView> values, std::size_t defaultValueIndex)
            : mValue(
                index, category, name, std::string(values[defaultValueIndex].mValue), makeEnumSanitizerString(values))
            , mEnumValues(values)
        {
        }

        Settings::SettingValue<std::string>& getValue() { return mValue; }

        std::span<const EnumValueView> getEnumValues() const { return mEnumValues; }

    private:
        Settings::SettingValue<std::string> mValue;
        std::span<const EnumValueView> mEnumValues;
    };

    struct WindowsCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "Windows";

        static constexpr std::array<EnumValueView, 3> sMainwindowScrollbarValues{
            EnumValueView{
                "Scrollbar Only", "Simple addition of scrollbars, the view window does not grow automatically." },
            EnumValueView{ "Grow Only", "The view window grows as subviews are added. No scrollbars." },
            EnumValueView{
                "Grow then Scroll", "The view window grows. The scrollbar appears once it cannot grow any further." },
        };

        Settings::SettingValue<int> mDefaultWidth{ mIndex, sName, "default-width", 800 };
        Settings::SettingValue<int> mDefaultHeight{ mIndex, sName, "default-height", 600 };
        Settings::SettingValue<bool> mShowStatusbar{ mIndex, sName, "show-statusbar", true };
        Settings::SettingValue<bool> mReuse{ mIndex, sName, "reuse", true };
        Settings::SettingValue<int> mMaxSubviews{ mIndex, sName, "max-subviews", 256 };
        Settings::SettingValue<bool> mHideSubview{ mIndex, sName, "hide-subview", false };
        Settings::SettingValue<int> mMinimumWidth{ mIndex, sName, "minimum-width", 325 };
        EnumSettingValue mMainwindowScrollbar{ mIndex, sName, "mainwindow-scrollbar", sMainwindowScrollbarValues, 0 };
        Settings::SettingValue<bool> mGrowLimit{ mIndex, sName, "grow-limit", false };
    };

    struct RecordsCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "Records";

        static constexpr std::array<EnumValueView, 3> sRecordValues{
            EnumValueView{ "Icon and Text", "" },
            EnumValueView{ "Icon Only", "" },
            EnumValueView{ "Text Only", "" },
        };

        EnumSettingValue mStatusFormat{ mIndex, sName, "status-format", sRecordValues, 0 };
        EnumSettingValue mTypeFormat{ mIndex, sName, "type-format", sRecordValues, 0 };
    };

    struct IdTablesCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "ID Tables";

        static constexpr std::array<EnumValueView, 7> sDoubleClickValues{
            EnumValueView{ "Edit in Place", "Edit the clicked cell" },
            EnumValueView{ "Edit Record", "Open a dialogue subview for the clicked record" },
            EnumValueView{ "View", "Open a scene subview for the clicked record (not available everywhere)" },
            EnumValueView{ "Revert", "" },
            EnumValueView{ "Delete", "" },
            EnumValueView{ "Edit Record and Close", "" },
            EnumValueView{
                "View and Close", "Open a scene subview for the clicked record and close the table subview" },
        };

        static constexpr std::array<EnumValueView, 3> sJumpAndSelectValues{
            EnumValueView{ "Jump and Select", "Scroll new record into view and make it the selection" },
            EnumValueView{ "Jump Only", "Scroll new record into view" },
            EnumValueView{ "No Jump", "No special action" },
        };

        EnumSettingValue mDouble{ mIndex, sName, "double", sDoubleClickValues, 0 };
        EnumSettingValue mDoubleS{ mIndex, sName, "double-s", sDoubleClickValues, 1 };
        EnumSettingValue mDoubleC{ mIndex, sName, "double-c", sDoubleClickValues, 2 };
        EnumSettingValue mDoubleSc{ mIndex, sName, "double-sc", sDoubleClickValues, 5 };
        EnumSettingValue mJumpToAdded{ mIndex, sName, "jump-to-added", sJumpAndSelectValues, 0 };
        Settings::SettingValue<bool> mExtendedConfig{ mIndex, sName, "extended-config", false };
        Settings::SettingValue<bool> mSubviewNewWindow{ mIndex, sName, "subview-new-window", false };
        Settings::SettingValue<int> mFilterDelay{ mIndex, sName, "filter-delay", 500 };
    };

    struct IdDialoguesCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "ID Dialogues";

        Settings::SettingValue<bool> mToolbar{ mIndex, sName, "toolbar", true };
    };

    struct ReportsCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "Reports";

        static constexpr std::array<EnumValueView, 4> sReportValues{
            EnumValueView{ "None", "" },
            EnumValueView{ "Edit", "Open a table or dialogue suitable for addressing the listed report" },
            EnumValueView{ "Remove", "Remove the report from the report table" },
            EnumValueView{ "Edit And Remove",
                "Open a table or dialogue suitable for addressing the listed report, then remove the report from the "
                "report table" },
        };

        EnumSettingValue mDouble{ mIndex, sName, "double", sReportValues, 1 };
        EnumSettingValue mDoubleS{ mIndex, sName, "double-s", sReportValues, 2 };
        EnumSettingValue mDoubleC{ mIndex, sName, "double-c", sReportValues, 3 };
        EnumSettingValue mDoubleSc{ mIndex, sName, "double-sc", sReportValues, 0 };
        Settings::SettingValue<bool> mIgnoreBaseRecords{ mIndex, sName, "ignore-base-records", false };
    };

    struct SearchAndReplaceCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "Search & Replace";

        Settings::SettingValue<int> mCharBefore{ mIndex, sName, "char-before", 10 };
        Settings::SettingValue<int> mCharAfter{ mIndex, sName, "char-after", 10 };
        Settings::SettingValue<bool> mAutoDelete{ mIndex, sName, "auto-delete", true };
    };

    struct ScriptsCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "Scripts";

        static constexpr std::array<EnumValueView, 3> sWarningValues{
            EnumValueView{ "Ignore", "Do not report warning" },
            EnumValueView{ "Normal", "Report warnings as warning" },
            EnumValueView{ "Strict", "Promote warning to an error" },
        };

        Settings::SettingValue<bool> mShowLinenum{ mIndex, sName, "show-linenum", true };
        Settings::SettingValue<bool> mWrapLines{ mIndex, sName, "wrap-lines", false };
        Settings::SettingValue<bool> mMonoFont{ mIndex, sName, "mono-font", true };
        Settings::SettingValue<int> mTabWidth{ mIndex, sName, "tab-width", 4 };
        EnumSettingValue mWarnings{ mIndex, sName, "warnings", sWarningValues, 1 };
        Settings::SettingValue<bool> mToolbar{ mIndex, sName, "toolbar", true };
        Settings::SettingValue<int> mCompileDelay{ mIndex, sName, "compile-delay", 100 };
        Settings::SettingValue<int> mErrorHeight{ mIndex, sName, "error-height", 100 };
        Settings::SettingValue<bool> mHighlightOccurrences{ mIndex, sName, "highlight-occurrences", true };
        Settings::SettingValue<std::string> mColourHighlight{ mIndex, sName, "colour-highlight", "lightcyan" };
        Settings::SettingValue<std::string> mColourInt{ mIndex, sName, "colour-int", "#aa55ff" };
        Settings::SettingValue<std::string> mColourFloat{ mIndex, sName, "colour-float", "magenta" };
        Settings::SettingValue<std::string> mColourName{ mIndex, sName, "colour-name", "grey" };
        Settings::SettingValue<std::string> mColourKeyword{ mIndex, sName, "colour-keyword", "red" };
        Settings::SettingValue<std::string> mColourSpecial{ mIndex, sName, "colour-special", "darkorange" };
        Settings::SettingValue<std::string> mColourComment{ mIndex, sName, "colour-comment", "green" };
        Settings::SettingValue<std::string> mColourId{ mIndex, sName, "colour-id", "#0055ff" };
    };

    struct GeneralInputCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "General Input";

        Settings::SettingValue<bool> mCycle{ mIndex, sName, "cycle", false };
    };

    struct SceneInputCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "3D Scene Input";

        Settings::SettingValue<double> mNaviWheelFactor{ mIndex, sName, "navi-wheel-factor", 8 };
        Settings::SettingValue<double> mSNaviSensitivity{ mIndex, sName, "s-navi-sensitivity", 50 };
        Settings::SettingValue<double> mPNaviFreeSensitivity{ mIndex, sName, "p-navi-free-sensitivity", 1 / 650.0 };
        Settings::SettingValue<bool> mPNaviFreeInvert{ mIndex, sName, "p-navi-free-invert", false };
        Settings::SettingValue<double> mNaviFreeLinSpeed{ mIndex, sName, "navi-free-lin-speed", 1000 };
        Settings::SettingValue<double> mNaviFreeRotSpeed{ mIndex, sName, "navi-free-rot-speed", 3.14 / 2 };
        Settings::SettingValue<double> mNaviFreeSpeedMult{ mIndex, sName, "navi-free-speed-mult", 8 };
        Settings::SettingValue<double> mPNaviOrbitSensitivity{ mIndex, sName, "p-navi-orbit-sensitivity", 1 / 650.0 };
        Settings::SettingValue<bool> mPNaviOrbitInvert{ mIndex, sName, "p-navi-orbit-invert", false };
        Settings::SettingValue<double> mNaviOrbitRotSpeed{ mIndex, sName, "navi-orbit-rot-speed", 3.14 / 4 };
        Settings::SettingValue<double> mNaviOrbitSpeedMult{ mIndex, sName, "navi-orbit-speed-mult", 4 };
        Settings::SettingValue<bool> mNaviOrbitConstRoll{ mIndex, sName, "navi-orbit-const-roll", true };
        Settings::SettingValue<bool> mContextSelect{ mIndex, sName, "context-select", false };
        Settings::SettingValue<double> mDragFactor{ mIndex, sName, "drag-factor", 1 };
        Settings::SettingValue<double> mDragWheelFactor{ mIndex, sName, "drag-wheel-factor", 1 };
        Settings::SettingValue<double> mDragShiftFactor{ mIndex, sName, "drag-shift-factor", 4 };
        Settings::SettingValue<double> mRotateFactor{ mIndex, sName, "rotate-factor", 0.007 };
    };

    struct RenderingCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "Rendering";

        Settings::SettingValue<int> mFramerateLimit{ mIndex, sName, "framerate-limit", 60 };
        Settings::SettingValue<int> mCameraFov{ mIndex, sName, "camera-fov", 90 };
        Settings::SettingValue<bool> mCameraOrtho{ mIndex, sName, "camera-ortho", false };
        Settings::SettingValue<int> mCameraOrthoSize{ mIndex, sName, "camera-ortho-size", 100 };
        Settings::SettingValue<double> mObjectMarkerAlpha{ mIndex, sName, "object-marker-alpha", 0.5 };
        Settings::SettingValue<bool> mSceneUseGradient{ mIndex, sName, "scene-use-gradient", true };
        Settings::SettingValue<std::string> mSceneDayBackgroundColour{ mIndex, sName, "scene-day-background-colour",
            "#6e7880" };
        Settings::SettingValue<std::string> mSceneDayGradientColour{ mIndex, sName, "scene-day-gradient-colour",
            "#2f3333" };
        Settings::SettingValue<std::string> mSceneBrightBackgroundColour{ mIndex, sName,
            "scene-bright-background-colour", "#4f575c" };
        Settings::SettingValue<std::string> mSceneBrightGradientColour{ mIndex, sName, "scene-bright-gradient-colour",
            "#2f3333" };
        Settings::SettingValue<std::string> mSceneNightBackgroundColour{ mIndex, sName, "scene-night-background-colour",
            "#404d4f" };
        Settings::SettingValue<std::string> mSceneNightGradientColour{ mIndex, sName, "scene-night-gradient-colour",
            "#2f3333" };
        Settings::SettingValue<bool> mSceneDayNightSwitchNodes{ mIndex, sName, "scene-day-night-switch-nodes", true };
    };

    struct TooltipsCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "Tooltips";

        Settings::SettingValue<bool> mScene{ mIndex, sName, "scene", true };
        Settings::SettingValue<bool> mSceneHideBasic{ mIndex, sName, "scene-hide-basic", false };
        Settings::SettingValue<int> mSceneDelay{ mIndex, sName, "scene-delay", 500 };
    };

    struct SceneEditingCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "3D Scene Editing";

        static constexpr std::array<EnumValueView, 3> sInsertOutsideCellValues{
            EnumValueView{ "Create cell and insert", "" },
            EnumValueView{ "Discard", "" },
            EnumValueView{ "Insert anyway", "" },
        };

        static constexpr std::array<EnumValueView, 3> sInsertOutsideVisibleCellValues{
            EnumValueView{ "Show cell and insert", "" },
            EnumValueView{ "Discard", "" },
            EnumValueView{ "Insert anyway", "" },
        };

        static constexpr std::array<EnumValueView, 2> sLandEditOutsideCellValues{
            EnumValueView{ "Create cell and land, then edit", "" },
            EnumValueView{ "Discard", "" },
        };

        static constexpr std::array<EnumValueView, 2> sLandEditOutsideVisibleCellValues{
            EnumValueView{ "Show cell and edit", "" },
            EnumValueView{ "Discard", "" },
        };

        static constexpr std::array<EnumValueView, 4> sSelectAction{
            EnumValueView{ "Select only", "" },
            EnumValueView{ "Add to selection", "" },
            EnumValueView{ "Remove from selection", "" },
            EnumValueView{ "Invert selection", "" },
        };

        Settings::SettingValue<double> mGridsnapMovement{ mIndex, sName, "gridsnap-movement", 16 };
        Settings::SettingValue<double> mGridsnapRotation{ mIndex, sName, "gridsnap-rotation", 15 };
        Settings::SettingValue<double> mGridsnapScale{ mIndex, sName, "gridsnap-scale", 0.25 };
        Settings::SettingValue<int> mDistance{ mIndex, sName, "distance", 50 };
        EnumSettingValue mOutsideDrop{ mIndex, sName, "outside-drop", sInsertOutsideCellValues, 0 };
        EnumSettingValue mOutsideVisibleDrop{ mIndex, sName, "outside-visible-drop", sInsertOutsideVisibleCellValues,
            0 };
        EnumSettingValue mOutsideLandedit{ mIndex, sName, "outside-landedit", sLandEditOutsideCellValues, 0 };
        EnumSettingValue mOutsideVisibleLandedit{ mIndex, sName, "outside-visible-landedit",
            sLandEditOutsideVisibleCellValues, 0 };
        Settings::SettingValue<int> mTexturebrushMaximumsize{ mIndex, sName, "texturebrush-maximumsize", 50 };
        Settings::SettingValue<int> mShapebrushMaximumsize{ mIndex, sName, "shapebrush-maximumsize", 100 };
        Settings::SettingValue<bool> mLandeditPostSmoothpainting{ mIndex, sName, "landedit-post-smoothpainting",
            false };
        Settings::SettingValue<double> mLandeditPostSmoothstrength{ mIndex, sName, "landedit-post-smoothstrength",
            0.25 };
        Settings::SettingValue<bool> mOpenListView{ mIndex, sName, "open-list-view", false };
        EnumSettingValue mPrimarySelectAction{ mIndex, sName, "primary-select-action", sSelectAction, 0 };
        EnumSettingValue mSecondarySelectAction{ mIndex, sName, "secondary-select-action", sSelectAction, 1 };
    };

    struct KeyBindingsCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "Key Bindings";

        Settings::SettingValue<std::string> mDocumentFileNewgame{ mIndex, sName, "document-file-newgame", "Ctrl+N" };
        Settings::SettingValue<std::string> mDocumentFileNewaddon{ mIndex, sName, "document-file-newaddon", "" };
        Settings::SettingValue<std::string> mDocumentFileOpen{ mIndex, sName, "document-file-open", "Ctrl+O" };
        Settings::SettingValue<std::string> mDocumentFileSave{ mIndex, sName, "document-file-save", "Ctrl+S" };
        Settings::SettingValue<std::string> mDocumentHelpHelp{ mIndex, sName, "document-help-help", "F1" };
        Settings::SettingValue<std::string> mDocumentHelpTutorial{ mIndex, sName, "document-help-tutorial", "" };
        Settings::SettingValue<std::string> mDocumentFileVerify{ mIndex, sName, "document-file-verify", "" };
        Settings::SettingValue<std::string> mDocumentFileMerge{ mIndex, sName, "document-file-merge", "" };
        Settings::SettingValue<std::string> mDocumentFileErrorlog{ mIndex, sName, "document-file-errorlog", "" };
        Settings::SettingValue<std::string> mDocumentFileMetadata{ mIndex, sName, "document-file-metadata", "" };
        Settings::SettingValue<std::string> mDocumentFileClose{ mIndex, sName, "document-file-close", "Ctrl+W" };
        Settings::SettingValue<std::string> mDocumentFileExit{ mIndex, sName, "document-file-exit", "Ctrl+Q" };
        Settings::SettingValue<std::string> mDocumentEditUndo{ mIndex, sName, "document-edit-undo", "Ctrl+Z" };
        Settings::SettingValue<std::string> mDocumentEditRedo{ mIndex, sName, "document-edit-redo", "Ctrl+Shift+Z" };
        Settings::SettingValue<std::string> mDocumentEditPreferences{ mIndex, sName, "document-edit-preferences", "" };
        Settings::SettingValue<std::string> mDocumentEditSearch{ mIndex, sName, "document-edit-search", "Ctrl+F" };
        Settings::SettingValue<std::string> mDocumentViewNewview{ mIndex, sName, "document-view-newview", "" };
        Settings::SettingValue<std::string> mDocumentViewStatusbar{ mIndex, sName, "document-view-statusbar", "" };
        Settings::SettingValue<std::string> mDocumentViewFilters{ mIndex, sName, "document-view-filters", "" };
        Settings::SettingValue<std::string> mDocumentWorldRegions{ mIndex, sName, "document-world-regions", "" };
        Settings::SettingValue<std::string> mDocumentWorldCells{ mIndex, sName, "document-world-cells", "" };
        Settings::SettingValue<std::string> mDocumentWorldReferencables{ mIndex, sName, "document-world-referencables",
            "" };
        Settings::SettingValue<std::string> mDocumentWorldReferences{ mIndex, sName, "document-world-references", "" };
        Settings::SettingValue<std::string> mDocumentWorldLands{ mIndex, sName, "document-world-lands", "" };
        Settings::SettingValue<std::string> mDocumentWorldLandtextures{ mIndex, sName, "document-world-landtextures",
            "" };
        Settings::SettingValue<std::string> mDocumentWorldPathgrid{ mIndex, sName, "document-world-pathgrid", "" };
        Settings::SettingValue<std::string> mDocumentWorldRegionmap{ mIndex, sName, "document-world-regionmap", "" };
        Settings::SettingValue<std::string> mDocumentMechanicsGlobals{ mIndex, sName, "document-mechanics-globals",
            "" };
        Settings::SettingValue<std::string> mDocumentMechanicsGamesettings{ mIndex, sName,
            "document-mechanics-gamesettings", "" };
        Settings::SettingValue<std::string> mDocumentMechanicsScripts{ mIndex, sName, "document-mechanics-scripts",
            "" };
        Settings::SettingValue<std::string> mDocumentMechanicsSpells{ mIndex, sName, "document-mechanics-spells", "" };
        Settings::SettingValue<std::string> mDocumentMechanicsEnchantments{ mIndex, sName,
            "document-mechanics-enchantments", "" };
        Settings::SettingValue<std::string> mDocumentMechanicsMagiceffects{ mIndex, sName,
            "document-mechanics-magiceffects", "" };
        Settings::SettingValue<std::string> mDocumentMechanicsStartscripts{ mIndex, sName,
            "document-mechanics-startscripts", "" };
        Settings::SettingValue<std::string> mDocumentCharacterSkills{ mIndex, sName, "document-character-skills", "" };
        Settings::SettingValue<std::string> mDocumentCharacterClasses{ mIndex, sName, "document-character-classes",
            "" };
        Settings::SettingValue<std::string> mDocumentCharacterFactions{ mIndex, sName, "document-character-factions",
            "" };
        Settings::SettingValue<std::string> mDocumentCharacterRaces{ mIndex, sName, "document-character-races", "" };
        Settings::SettingValue<std::string> mDocumentCharacterBirthsigns{ mIndex, sName,
            "document-character-birthsigns", "" };
        Settings::SettingValue<std::string> mDocumentCharacterTopics{ mIndex, sName, "document-character-topics", "" };
        Settings::SettingValue<std::string> mDocumentCharacterJournals{ mIndex, sName, "document-character-journals",
            "" };
        Settings::SettingValue<std::string> mDocumentCharacterTopicinfos{ mIndex, sName,
            "document-character-topicinfos", "" };
        Settings::SettingValue<std::string> mDocumentCharacterJournalinfos{ mIndex, sName,
            "document-character-journalinfos", "" };
        Settings::SettingValue<std::string> mDocumentCharacterBodyparts{ mIndex, sName, "document-character-bodyparts",
            "" };
        Settings::SettingValue<std::string> mDocumentAssetsReload{ mIndex, sName, "document-assets-reload", "F5" };
        Settings::SettingValue<std::string> mDocumentAssetsSounds{ mIndex, sName, "document-assets-sounds", "" };
        Settings::SettingValue<std::string> mDocumentAssetsSoundgens{ mIndex, sName, "document-assets-soundgens", "" };
        Settings::SettingValue<std::string> mDocumentAssetsMeshes{ mIndex, sName, "document-assets-meshes", "" };
        Settings::SettingValue<std::string> mDocumentAssetsIcons{ mIndex, sName, "document-assets-icons", "" };
        Settings::SettingValue<std::string> mDocumentAssetsMusic{ mIndex, sName, "document-assets-music", "" };
        Settings::SettingValue<std::string> mDocumentAssetsSoundres{ mIndex, sName, "document-assets-soundres", "" };
        Settings::SettingValue<std::string> mDocumentAssetsTextures{ mIndex, sName, "document-assets-textures", "" };
        Settings::SettingValue<std::string> mDocumentAssetsVideos{ mIndex, sName, "document-assets-videos", "" };
        Settings::SettingValue<std::string> mDocumentDebugRun{ mIndex, sName, "document-debug-run", "" };
        Settings::SettingValue<std::string> mDocumentDebugShutdown{ mIndex, sName, "document-debug-shutdown", "" };
        Settings::SettingValue<std::string> mDocumentDebugProfiles{ mIndex, sName, "document-debug-profiles", "" };
        Settings::SettingValue<std::string> mDocumentDebugRunlog{ mIndex, sName, "document-debug-runlog", "" };
        Settings::SettingValue<std::string> mTableEdit{ mIndex, sName, "table-edit", "" };
        Settings::SettingValue<std::string> mTableAdd{ mIndex, sName, "table-add", "Shift+A" };
        Settings::SettingValue<std::string> mTableClone{ mIndex, sName, "table-clone", "Shift+D" };
        Settings::SettingValue<std::string> mTouchRecord{ mIndex, sName, "touch-record", "" };
        Settings::SettingValue<std::string> mTableRevert{ mIndex, sName, "table-revert", "" };
        Settings::SettingValue<std::string> mTableRemove{ mIndex, sName, "table-remove", "Delete" };
        Settings::SettingValue<std::string> mTableMoveup{ mIndex, sName, "table-moveup", "" };
        Settings::SettingValue<std::string> mTableMovedown{ mIndex, sName, "table-movedown", "" };
        Settings::SettingValue<std::string> mTableView{ mIndex, sName, "table-view", "Shift+C" };
        Settings::SettingValue<std::string> mTablePreview{ mIndex, sName, "table-preview", "Shift+V" };
        Settings::SettingValue<std::string> mTableExtendeddelete{ mIndex, sName, "table-extendeddelete", "" };
        Settings::SettingValue<std::string> mTableExtendedrevert{ mIndex, sName, "table-extendedrevert", "" };
        Settings::SettingValue<std::string> mReporttableShow{ mIndex, sName, "reporttable-show", "" };
        Settings::SettingValue<std::string> mReporttableRemove{ mIndex, sName, "reporttable-remove", "Delete" };
        Settings::SettingValue<std::string> mReporttableReplace{ mIndex, sName, "reporttable-replace", "" };
        Settings::SettingValue<std::string> mReporttableRefresh{ mIndex, sName, "reporttable-refresh", "" };
        Settings::SettingValue<std::string> mSceneNaviPrimary{ mIndex, sName, "scene-navi-primary", "MMB" };
        Settings::SettingValue<std::string> mSceneNaviSecondary{ mIndex, sName, "scene-navi-secondary", "Ctrl+MMB" };
        Settings::SettingValue<std::string> mSceneOpenPrimary{ mIndex, sName, "scene-open-primary", "Shift+RMB" };
        Settings::SettingValue<std::string> mSceneEditPrimary{ mIndex, sName, "scene-edit-primary", "RMB" };
        Settings::SettingValue<std::string> mSceneEditSecondary{ mIndex, sName, "scene-edit-secondary", "Ctrl+RMB" };
        Settings::SettingValue<std::string> mSceneSelectPrimary{ mIndex, sName, "scene-select-primary", "LMB" };
        Settings::SettingValue<std::string> mSceneSelectSecondary{ mIndex, sName, "scene-select-secondary",
            "Ctrl+LMB" };
        Settings::SettingValue<std::string> mSceneSelectTertiary{ mIndex, sName, "scene-select-tertiary", "Shift+LMB" };
        Settings::SettingValue<std::string> mSceneSpeedModifier{ mIndex, sName, "scene-speed-modifier", "Shift" };
        Settings::SettingValue<std::string> mSceneDelete{ mIndex, sName, "scene-delete", "Delete" };
        Settings::SettingValue<std::string> mSceneInstanceDrop{ mIndex, sName, "scene-instance-drop", "F" };
        Settings::SettingValue<std::string> mSceneDuplicate{ mIndex, sName, "scene-duplicate", "Shift+C" };
        Settings::SettingValue<std::string> mSceneLoadCamCell{ mIndex, sName, "scene-load-cam-cell", "Keypad+5" };
        Settings::SettingValue<std::string> mSceneLoadCamEastcell{ mIndex, sName, "scene-load-cam-eastcell",
            "Keypad+6" };
        Settings::SettingValue<std::string> mSceneLoadCamNorthcell{ mIndex, sName, "scene-load-cam-northcell",
            "Keypad+8" };
        Settings::SettingValue<std::string> mSceneLoadCamWestcell{ mIndex, sName, "scene-load-cam-westcell",
            "Keypad+4" };
        Settings::SettingValue<std::string> mSceneLoadCamSouthcell{ mIndex, sName, "scene-load-cam-southcell",
            "Keypad+2" };
        Settings::SettingValue<std::string> mSceneEditAbort{ mIndex, sName, "scene-edit-abort", "Escape" };
        Settings::SettingValue<std::string> mSceneFocusToolbar{ mIndex, sName, "scene-focus-toolbar", "T" };
        Settings::SettingValue<std::string> mSceneRenderStats{ mIndex, sName, "scene-render-stats", "F3" };
        Settings::SettingValue<std::string> mSceneClearSelection{ mIndex, sName, "scene-clear-selection", "Space" };
        Settings::SettingValue<std::string> mSceneUnhideAll{ mIndex, sName, "scene-unhide-all", "Alt+H" };
        Settings::SettingValue<std::string> mSceneToggleVisibility{ mIndex, sName, "scene-toggle-visibility", "H" };
        Settings::SettingValue<std::string> mSceneGroup0{ mIndex, sName, "scene-group-0", "0" };
        Settings::SettingValue<std::string> mSceneSave0{ mIndex, sName, "scene-save-0", "Ctrl+0" };
        Settings::SettingValue<std::string> mSceneGroup1{ mIndex, sName, "scene-group-1", "1" };
        Settings::SettingValue<std::string> mSceneSave1{ mIndex, sName, "scene-save-1", "Ctrl+1" };
        Settings::SettingValue<std::string> mSceneGroup2{ mIndex, sName, "scene-group-2", "2" };
        Settings::SettingValue<std::string> mSceneSave2{ mIndex, sName, "scene-save-2", "Ctrl+2" };
        Settings::SettingValue<std::string> mSceneGroup3{ mIndex, sName, "scene-group-3", "3" };
        Settings::SettingValue<std::string> mSceneSave3{ mIndex, sName, "scene-save-3", "Ctrl+3" };
        Settings::SettingValue<std::string> mSceneGroup4{ mIndex, sName, "scene-group-4", "4" };
        Settings::SettingValue<std::string> mSceneSave4{ mIndex, sName, "scene-save-4", "Ctrl+4" };
        Settings::SettingValue<std::string> mSceneGroup5{ mIndex, sName, "scene-group-5", "5" };
        Settings::SettingValue<std::string> mSceneSave5{ mIndex, sName, "scene-save-5", "Ctrl+5" };
        Settings::SettingValue<std::string> mSceneGroup6{ mIndex, sName, "scene-group-6", "6" };
        Settings::SettingValue<std::string> mSceneSave6{ mIndex, sName, "scene-save-6", "Ctrl+6" };
        Settings::SettingValue<std::string> mSceneGroup7{ mIndex, sName, "scene-group-7", "7" };
        Settings::SettingValue<std::string> mSceneSave7{ mIndex, sName, "scene-save-7", "Ctrl+7" };
        Settings::SettingValue<std::string> mSceneGroup8{ mIndex, sName, "scene-group-8", "8" };
        Settings::SettingValue<std::string> mSceneSave8{ mIndex, sName, "scene-save-8", "Ctrl+8" };
        Settings::SettingValue<std::string> mSceneGroup9{ mIndex, sName, "scene-group-9", "9" };
        Settings::SettingValue<std::string> mSceneSave9{ mIndex, sName, "scene-save-9", "Ctrl+9" };
        Settings::SettingValue<std::string> mSceneAxisX{ mIndex, sName, "scene-axis-x", "X" };
        Settings::SettingValue<std::string> mSceneAxisY{ mIndex, sName, "scene-axis-y", "Y" };
        Settings::SettingValue<std::string> mSceneAxisZ{ mIndex, sName, "scene-axis-z", "Z" };
        Settings::SettingValue<std::string> mSceneMoveSubmode{ mIndex, sName, "scene-submode-move", "G" };
        Settings::SettingValue<std::string> mSceneScaleSubmode{ mIndex, sName, "scene-submode-scale", "V" };
        Settings::SettingValue<std::string> mSceneRotateSubmode{ mIndex, sName, "scene-submode-rotate", "R" };
        Settings::SettingValue<std::string> mSceneCameraCycle{ mIndex, sName, "scene-cam-cycle", "Tab" };
        Settings::SettingValue<std::string> mSceneToggleMarkers{ mIndex, sName, "scene-toggle-markers", "F4" };
        Settings::SettingValue<std::string> mFreeForward{ mIndex, sName, "free-forward", "W" };
        Settings::SettingValue<std::string> mFreeBackward{ mIndex, sName, "free-backward", "S" };
        Settings::SettingValue<std::string> mFreeLeft{ mIndex, sName, "free-left", "A" };
        Settings::SettingValue<std::string> mFreeRight{ mIndex, sName, "free-right", "D" };
        Settings::SettingValue<std::string> mFreeRollLeft{ mIndex, sName, "free-roll-left", "Q" };
        Settings::SettingValue<std::string> mFreeRollRight{ mIndex, sName, "free-roll-right", "E" };
        Settings::SettingValue<std::string> mFreeSpeedMode{ mIndex, sName, "free-speed-mode", "" };
        Settings::SettingValue<std::string> mOrbitUp{ mIndex, sName, "orbit-up", "W" };
        Settings::SettingValue<std::string> mOrbitDown{ mIndex, sName, "orbit-down", "S" };
        Settings::SettingValue<std::string> mOrbitLeft{ mIndex, sName, "orbit-left", "A" };
        Settings::SettingValue<std::string> mOrbitRight{ mIndex, sName, "orbit-right", "D" };
        Settings::SettingValue<std::string> mOrbitRollLeft{ mIndex, sName, "orbit-roll-left", "Q" };
        Settings::SettingValue<std::string> mOrbitRollRight{ mIndex, sName, "orbit-roll-right", "E" };
        Settings::SettingValue<std::string> mOrbitSpeedMode{ mIndex, sName, "orbit-speed-mode", "" };
        Settings::SettingValue<std::string> mOrbitCenterSelection{ mIndex, sName, "orbit-center-selection", "C" };
        Settings::SettingValue<std::string> mScriptEditorComment{ mIndex, sName, "script-editor-comment", "" };
        Settings::SettingValue<std::string> mScriptEditorUncomment{ mIndex, sName, "script-editor-uncomment", "" };
    };

    struct ModelsCategory : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        static constexpr std::string_view sName = "Models";

        Settings::SettingValue<std::string> mBaseanim{ mIndex, sName, "baseanim", "meshes/base_anim.nif" };
        Settings::SettingValue<std::string> mBaseanimkna{ mIndex, sName, "baseanimkna", "meshes/base_animkna.nif" };
        Settings::SettingValue<std::string> mBaseanimfemale{ mIndex, sName, "baseanimfemale",
            "meshes/base_anim_female.nif" };
        Settings::SettingValue<std::string> mWolfskin{ mIndex, sName, "wolfskin", "meshes/wolf/skin.nif" };
    };

    struct Values : Settings::WithIndex
    {
        using Settings::WithIndex::WithIndex;

        WindowsCategory mWindows{ mIndex };
        RecordsCategory mRecords{ mIndex };
        IdTablesCategory mIdTables{ mIndex };
        IdDialoguesCategory mIdDialogues{ mIndex };
        ReportsCategory mReports{ mIndex };
        SearchAndReplaceCategory mSearchAndReplace{ mIndex };
        ScriptsCategory mScripts{ mIndex };
        GeneralInputCategory mGeneralInput{ mIndex };
        SceneInputCategory mSceneInput{ mIndex };
        RenderingCategory mRendering{ mIndex };
        TooltipsCategory mTooltips{ mIndex };
        SceneEditingCategory mSceneEditing{ mIndex };
        KeyBindingsCategory mKeyBindings{ mIndex };
        ModelsCategory mModels{ mIndex };
    };
}

#endif
