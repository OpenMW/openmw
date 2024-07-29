#include "state.hpp"

#include <QColor>
#include <QKeySequence>

#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/enumsetting.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/prefs/shortcutmanager.hpp>
#include <apps/opencs/model/prefs/subcategory.hpp>

#include <components/settings/categories.hpp>
#include <components/settings/settings.hpp>

#include "boolsetting.hpp"
#include "coloursetting.hpp"
#include "doublesetting.hpp"
#include "intsetting.hpp"
#include "modifiersetting.hpp"
#include "shortcutsetting.hpp"
#include "stringsetting.hpp"
#include "values.hpp"

CSMPrefs::State* CSMPrefs::State::sThis = nullptr;

void CSMPrefs::State::declare()
{
    declareCategory("Windows");
    declareInt(mValues->mWindows.mDefaultWidth, "Default window width")
        .setTooltip("Newly opened top-level windows will open with this width.")
        .setMin(80);
    declareInt(mValues->mWindows.mDefaultHeight, "Default window height")
        .setTooltip("Newly opened top-level windows will open with this height.")
        .setMin(80);
    declareBool(mValues->mWindows.mShowStatusbar, "Show Status Bar")
        .setTooltip(
            "If a newly open top level window is showing status bars or not. "
            " Note that this does not affect existing windows.");
    declareBool(mValues->mWindows.mReuse, "Reuse Subviews")
        .setTooltip(
            "When a new subview is requested and a matching subview already "
            " exist, do not open a new subview and use the existing one instead.");
    declareInt(mValues->mWindows.mMaxSubviews, "Maximum number of subviews per top-level window")
        .setTooltip(
            "If the maximum number is reached and a new subview is opened "
            "it will be placed into a new top-level window.")
        .setRange(1, 256);
    declareBool(mValues->mWindows.mHideSubview, "Hide single subview")
        .setTooltip(
            "When a view contains only a single subview, hide the subview title "
            "bar and if this subview is closed also close the view (unless it is the last "
            "view for this document)");
    declareInt(mValues->mWindows.mMinimumWidth, "Minimum subview width")
        .setTooltip("Minimum width of subviews.")
        .setRange(50, 10000);
    declareEnum(mValues->mWindows.mMainwindowScrollbar, "Horizontal scrollbar mode for main window.");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    declareBool(mValues->mWindows.mGrowLimit, "Grow Limit Screen")
        .setTooltip(
            "When \"Grow then Scroll\" option is selected, the window size grows to"
            " the width of the virtual desktop. \nIf this option is selected the the window growth"
            "is limited to the current screen.");
#endif

    declareCategory("Records");
    declareEnum(mValues->mRecords.mStatusFormat, "Modification status display format");
    declareEnum(mValues->mRecords.mTypeFormat, "ID type display format");

    declareCategory("ID Tables");
    declareEnum(mValues->mIdTables.mDouble, "Double Click");
    declareEnum(mValues->mIdTables.mDoubleS, "Shift Double Click");
    declareEnum(mValues->mIdTables.mDoubleC, "Control Double Click");
    declareEnum(mValues->mIdTables.mDoubleSc, "Shift Control Double Click");
    declareEnum(mValues->mIdTables.mJumpToAdded, "Action on adding or cloning a record");
    declareBool(
        mValues->mIdTables.mExtendedConfig, "Manually specify affected record types for an extended delete/revert")
        .setTooltip(
            "Delete and revert commands have an extended form that also affects "
            "associated records.\n\n"
            "If this option is enabled, types of affected records are selected "
            "manually before a command execution.\nOtherwise, all associated "
            "records are deleted/reverted immediately.");
    declareBool(mValues->mIdTables.mSubviewNewWindow, "Open Record in new window")
        .setTooltip(
            "When editing a record, open the view in a new window,"
            " rather than docked in the main view.");
    declareInt(mValues->mIdTables.mFilterDelay, "Delay before applying a filter (in miliseconds)");

    declareCategory("ID Dialogues");
    declareBool(mValues->mIdDialogues.mToolbar, "Show toolbar");

    declareCategory("Reports");
    declareEnum(mValues->mReports.mDouble, "Double Click");
    declareEnum(mValues->mReports.mDoubleS, "Shift Double Click");
    declareEnum(mValues->mReports.mDoubleC, "Control Double Click");
    declareEnum(mValues->mReports.mDoubleSc, "Shift Control Double Click");
    declareBool(mValues->mReports.mIgnoreBaseRecords, "Ignore base records in verifier");

    declareCategory("Search & Replace");
    declareInt(mValues->mSearchAndReplace.mCharBefore, "Characters before search string")
        .setTooltip("Maximum number of character to display in search result before the searched text");
    declareInt(mValues->mSearchAndReplace.mCharAfter, "Characters after search string")
        .setTooltip("Maximum number of character to display in search result after the searched text");
    declareBool(mValues->mSearchAndReplace.mAutoDelete, "Delete row from result table after a successful replace");

    declareCategory("Scripts");
    declareBool(mValues->mScripts.mShowLinenum, "Show Line Numbers")
        .setTooltip(
            "Show line numbers to the left of the script editor window."
            "The current row and column numbers of the text cursor are shown at the bottom.");
    declareBool(mValues->mScripts.mWrapLines, "Wrap Lines")
        .setTooltip("Wrap lines longer than width of script editor.");
    declareBool(mValues->mScripts.mMonoFont, "Use monospace font");
    declareInt(mValues->mScripts.mTabWidth, "Tab Width")
        .setTooltip("Number of characters for tab width")
        .setRange(1, 10);
    declareEnum(mValues->mScripts.mWarnings, "Warning Mode");
    declareBool(mValues->mScripts.mToolbar, "Show toolbar");
    declareInt(mValues->mScripts.mCompileDelay, "Delay between updating of source errors")
        .setTooltip("Delay in milliseconds")
        .setRange(0, 10000);
    declareInt(mValues->mScripts.mErrorHeight, "Initial height of the error panel").setRange(100, 10000);
    declareBool(mValues->mScripts.mHighlightOccurrences, "Highlight other occurrences of selected names");
    declareColour(mValues->mScripts.mColourHighlight, "Colour of highlighted occurrences");
    declareColour(mValues->mScripts.mColourInt, "Highlight Colour: Integer Literals");
    declareColour(mValues->mScripts.mColourFloat, "Highlight Colour: Float Literals");
    declareColour(mValues->mScripts.mColourName, "Highlight Colour: Names");
    declareColour(mValues->mScripts.mColourKeyword, "Highlight Colour: Keywords");
    declareColour(mValues->mScripts.mColourSpecial, "Highlight Colour: Special Characters");
    declareColour(mValues->mScripts.mColourComment, "Highlight Colour: Comments");
    declareColour(mValues->mScripts.mColourId, "Highlight Colour: IDs");

    declareCategory("General Input");
    declareBool(mValues->mGeneralInput.mCycle, "Cyclic next/previous")
        .setTooltip(
            "When using next/previous functions at the last/first item of a "
            "list go to the first/last item");

    declareCategory("3D Scene Input");

    declareDouble(mValues->mSceneInput.mNaviWheelFactor, "Camera Zoom Sensitivity").setRange(-100.0, 100.0);
    declareDouble(mValues->mSceneInput.mSNaviSensitivity, "Secondary Camera Movement Sensitivity")
        .setRange(-1000.0, 1000.0);

    declareDouble(mValues->mSceneInput.mPNaviFreeSensitivity, "Free Camera Sensitivity")
        .setPrecision(5)
        .setRange(0.0, 1.0);
    declareBool(mValues->mSceneInput.mPNaviFreeInvert, "Invert Free Camera Mouse Input");
    declareDouble(mValues->mSceneInput.mNaviFreeLinSpeed, "Free Camera Linear Speed").setRange(1.0, 10000.0);
    declareDouble(mValues->mSceneInput.mNaviFreeRotSpeed, "Free Camera Rotational Speed").setRange(0.001, 6.28);
    declareDouble(mValues->mSceneInput.mNaviFreeSpeedMult, "Free Camera Speed Multiplier (from Modifier)")
        .setRange(0.001, 1000.0);

    declareDouble(mValues->mSceneInput.mPNaviOrbitSensitivity, "Orbit Camera Sensitivity")
        .setPrecision(5)
        .setRange(0.0, 1.0);
    declareBool(mValues->mSceneInput.mPNaviOrbitInvert, "Invert Orbit Camera Mouse Input");
    declareDouble(mValues->mSceneInput.mNaviOrbitRotSpeed, "Orbital Camera Rotational Speed").setRange(0.001, 6.28);
    declareDouble(mValues->mSceneInput.mNaviOrbitSpeedMult, "Orbital Camera Speed Multiplier (from Modifier)")
        .setRange(0.001, 1000.0);
    declareBool(mValues->mSceneInput.mNaviOrbitConstRoll, "Keep camera roll constant for orbital camera");

    declareBool(mValues->mSceneInput.mContextSelect, "Context Sensitive Selection");
    declareDouble(mValues->mSceneInput.mDragFactor, "Mouse sensitivity during drag operations").setRange(0.001, 100.0);
    declareDouble(mValues->mSceneInput.mDragWheelFactor, "Mouse wheel sensitivity during drag operations")
        .setRange(0.001, 100.0);
    declareDouble(mValues->mSceneInput.mDragShiftFactor, "Shift-acceleration factor during drag operations")
        .setTooltip("Acceleration factor during drag operations while holding down shift")
        .setRange(0.001, 100.0);
    declareDouble(mValues->mSceneInput.mRotateFactor, "Free rotation factor").setPrecision(4).setRange(0.0001, 0.1);

    declareCategory("Rendering");
    declareInt(mValues->mRendering.mFramerateLimit, "FPS limit")
        .setTooltip("Framerate limit in 3D preview windows. Zero value means \"unlimited\".")
        .setRange(0, 10000);
    declareInt(mValues->mRendering.mCameraFov, "Camera FOV").setRange(10, 170);
    declareBool(mValues->mRendering.mCameraOrtho, "Orthographic projection for camera");
    declareInt(mValues->mRendering.mCameraOrthoSize, "Orthographic projection size parameter")
        .setTooltip("Size of the orthographic frustum, greater value will allow the camera to see more of the world.")
        .setRange(10, 10000);
    declareDouble(mValues->mRendering.mObjectMarkerAlpha, "Object Marker Transparency").setPrecision(2).setRange(0, 1);
    declareBool(mValues->mRendering.mSceneUseGradient, "Use Gradient Background");
    declareColour(mValues->mRendering.mSceneDayBackgroundColour, "Day Background Colour");
    declareColour(mValues->mRendering.mSceneDayGradientColour, "Day Gradient  Colour")
        .setTooltip(
            "Sets the gradient color to use in conjunction with the day background color. Ignored if "
            "the gradient option is disabled.");
    declareColour(mValues->mRendering.mSceneBrightBackgroundColour, "Scene Bright Background Colour");
    declareColour(mValues->mRendering.mSceneBrightGradientColour, "Scene Bright Gradient Colour")
        .setTooltip(
            "Sets the gradient color to use in conjunction with the bright background color. Ignored if "
            "the gradient option is disabled.");
    declareColour(mValues->mRendering.mSceneNightBackgroundColour, "Scene Night Background Colour");
    declareColour(mValues->mRendering.mSceneNightGradientColour, "Scene Night Gradient Colour")
        .setTooltip(
            "Sets the gradient color to use in conjunction with the night background color. Ignored if "
            "the gradient option is disabled.");
    declareBool(mValues->mRendering.mSceneDayNightSwitchNodes, "Use Day/Night Switch Nodes");

    declareCategory("Tooltips");
    declareBool(mValues->mTooltips.mScene, "Show Tooltips in 3D scenes");
    declareBool(mValues->mTooltips.mSceneHideBasic, "Hide basic  3D scenes tooltips");
    declareInt(mValues->mTooltips.mSceneDelay, "Tooltip delay in milliseconds").setMin(1);

    declareCategory("3D Scene Editing");
    declareDouble(mValues->mSceneEditing.mGridsnapMovement, "Grid snap size");
    declareDouble(mValues->mSceneEditing.mGridsnapRotation, "Angle snap size");
    declareDouble(mValues->mSceneEditing.mGridsnapScale, "Scale snap size");
    declareInt(mValues->mSceneEditing.mDistance, "Drop Distance")
        .setTooltip(
            "If an instance drop can not be placed against another object at the "
            "insert point, it will be placed by this distance from the insert point instead");
    declareEnum(mValues->mSceneEditing.mOutsideDrop, "Handling drops outside of cells");
    declareEnum(mValues->mSceneEditing.mOutsideVisibleDrop, "Handling drops outside of visible cells");
    declareEnum(mValues->mSceneEditing.mOutsideLandedit, "Handling terrain edit outside of cells")
        .setTooltip("Behavior of terrain editing, if land editing brush reaches an area without cell record.");
    declareEnum(mValues->mSceneEditing.mOutsideVisibleLandedit, "Handling terrain edit outside of visible cells")
        .setTooltip(
            "Behavior of terrain editing, if land editing brush reaches an area that is not currently visible.");
    declareInt(mValues->mSceneEditing.mTexturebrushMaximumsize, "Maximum texture brush size").setMin(1);
    declareInt(mValues->mSceneEditing.mShapebrushMaximumsize, "Maximum height edit brush size")
        .setTooltip("Setting for the slider range of brush size in terrain height editing.")
        .setMin(1);
    declareBool(mValues->mSceneEditing.mLandeditPostSmoothpainting, "Smooth land after painting height")
        .setTooltip("Raise and lower tools will leave bumpy finish without this option");
    declareDouble(mValues->mSceneEditing.mLandeditPostSmoothstrength, "Smoothing strength (post-edit)")
        .setTooltip(
            "If smoothing land after painting height is used, this is the percentage of smooth applied afterwards. "
            "Negative values may be used to roughen instead of smooth.")
        .setMin(-1)
        .setMax(1);
    declareBool(mValues->mSceneEditing.mOpenListView, "Open displays list view")
        .setTooltip(
            "When opening a reference from the scene view, it will open the"
            " instance list view instead of the individual instance record view.");
    declareEnum(mValues->mSceneEditing.mPrimarySelectAction, "Action for primary select")
        .setTooltip(
            "Selection can be chosen between select only, add to selection, remove from selection and invert "
            "selection.");
    declareEnum(mValues->mSceneEditing.mSecondarySelectAction, "Action for secondary select")
        .setTooltip(
            "Selection can be chosen between select only, add to selection, remove from selection and invert "
            "selection.");

    declareCategory("Key Bindings");

    declareSubcategory("Document");
    declareShortcut(mValues->mKeyBindings.mDocumentFileNewgame, "New Game");
    declareShortcut(mValues->mKeyBindings.mDocumentFileNewaddon, "New Addon");
    declareShortcut(mValues->mKeyBindings.mDocumentFileOpen, "Open");
    declareShortcut(mValues->mKeyBindings.mDocumentFileSave, "Save");
    declareShortcut(mValues->mKeyBindings.mDocumentHelpHelp, "Help");
    declareShortcut(mValues->mKeyBindings.mDocumentHelpTutorial, "Tutorial");
    declareShortcut(mValues->mKeyBindings.mDocumentFileVerify, "Verify");
    declareShortcut(mValues->mKeyBindings.mDocumentFileMerge, "Merge");
    declareShortcut(mValues->mKeyBindings.mDocumentFileErrorlog, "Open Load Error Log");
    declareShortcut(mValues->mKeyBindings.mDocumentFileMetadata, "Meta Data");
    declareShortcut(mValues->mKeyBindings.mDocumentFileClose, "Close Document");
    declareShortcut(mValues->mKeyBindings.mDocumentFileExit, "Exit Application");
    declareShortcut(mValues->mKeyBindings.mDocumentEditUndo, "Undo");
    declareShortcut(mValues->mKeyBindings.mDocumentEditRedo, "Redo");
    declareShortcut(mValues->mKeyBindings.mDocumentEditPreferences, "Open Preferences");
    declareShortcut(mValues->mKeyBindings.mDocumentEditSearch, "Search");
    declareShortcut(mValues->mKeyBindings.mDocumentViewNewview, "New View");
    declareShortcut(mValues->mKeyBindings.mDocumentViewStatusbar, "Toggle Status Bar");
    declareShortcut(mValues->mKeyBindings.mDocumentViewFilters, "Open Filter List");
    declareShortcut(mValues->mKeyBindings.mDocumentWorldRegions, "Open Region List");
    declareShortcut(mValues->mKeyBindings.mDocumentWorldCells, "Open Cell List");
    declareShortcut(mValues->mKeyBindings.mDocumentWorldReferencables, "Open Object List");
    declareShortcut(mValues->mKeyBindings.mDocumentWorldReferences, "Open Instance List");
    declareShortcut(mValues->mKeyBindings.mDocumentWorldLands, "Open Lands List");
    declareShortcut(mValues->mKeyBindings.mDocumentWorldLandtextures, "Open Land Textures List");
    declareShortcut(mValues->mKeyBindings.mDocumentWorldPathgrid, "Open Pathgrid List");
    declareShortcut(mValues->mKeyBindings.mDocumentWorldRegionmap, "Open Region Map");
    declareShortcut(mValues->mKeyBindings.mDocumentMechanicsGlobals, "Open Global List");
    declareShortcut(mValues->mKeyBindings.mDocumentMechanicsGamesettings, "Open Game Settings");
    declareShortcut(mValues->mKeyBindings.mDocumentMechanicsScripts, "Open Script List");
    declareShortcut(mValues->mKeyBindings.mDocumentMechanicsSpells, "Open Spell List");
    declareShortcut(mValues->mKeyBindings.mDocumentMechanicsEnchantments, "Open Enchantment List");
    declareShortcut(mValues->mKeyBindings.mDocumentMechanicsMagiceffects, "Open Magic Effect List");
    declareShortcut(mValues->mKeyBindings.mDocumentMechanicsStartscripts, "Open Start Script List");
    declareShortcut(mValues->mKeyBindings.mDocumentCharacterSkills, "Open Skill List");
    declareShortcut(mValues->mKeyBindings.mDocumentCharacterClasses, "Open Class List");
    declareShortcut(mValues->mKeyBindings.mDocumentCharacterFactions, "Open Faction List");
    declareShortcut(mValues->mKeyBindings.mDocumentCharacterRaces, "Open Race List");
    declareShortcut(mValues->mKeyBindings.mDocumentCharacterBirthsigns, "Open Birthsign List");
    declareShortcut(mValues->mKeyBindings.mDocumentCharacterTopics, "Open Topic List");
    declareShortcut(mValues->mKeyBindings.mDocumentCharacterJournals, "Open Journal List");
    declareShortcut(mValues->mKeyBindings.mDocumentCharacterTopicinfos, "Open Topic Info List");
    declareShortcut(mValues->mKeyBindings.mDocumentCharacterJournalinfos, "Open Journal Info List");
    declareShortcut(mValues->mKeyBindings.mDocumentCharacterBodyparts, "Open Body Part List");
    declareShortcut(mValues->mKeyBindings.mDocumentAssetsReload, "Reload Assets");
    declareShortcut(mValues->mKeyBindings.mDocumentAssetsSounds, "Open Sound Asset List");
    declareShortcut(mValues->mKeyBindings.mDocumentAssetsSoundgens, "Open Sound Generator List");
    declareShortcut(mValues->mKeyBindings.mDocumentAssetsMeshes, "Open Mesh Asset List");
    declareShortcut(mValues->mKeyBindings.mDocumentAssetsIcons, "Open Icon Asset List");
    declareShortcut(mValues->mKeyBindings.mDocumentAssetsMusic, "Open Music Asset List");
    declareShortcut(mValues->mKeyBindings.mDocumentAssetsSoundres, "Open Sound File List");
    declareShortcut(mValues->mKeyBindings.mDocumentAssetsTextures, "Open Texture Asset List");
    declareShortcut(mValues->mKeyBindings.mDocumentAssetsVideos, "Open Video Asset List");
    declareShortcut(mValues->mKeyBindings.mDocumentDebugRun, "Run Debug");
    declareShortcut(mValues->mKeyBindings.mDocumentDebugShutdown, "Stop Debug");
    declareShortcut(mValues->mKeyBindings.mDocumentDebugProfiles, "Debug Profiles");
    declareShortcut(mValues->mKeyBindings.mDocumentDebugRunlog, "Open Run Log");

    declareSubcategory("Table");
    declareShortcut(mValues->mKeyBindings.mTableEdit, "Edit Record");
    declareShortcut(mValues->mKeyBindings.mTableAdd, "Add Row/Record");
    declareShortcut(mValues->mKeyBindings.mTableClone, "Clone Record");
    declareShortcut(mValues->mKeyBindings.mTouchRecord, "Touch Record");
    declareShortcut(mValues->mKeyBindings.mTableRevert, "Revert Record");
    declareShortcut(mValues->mKeyBindings.mTableRemove, "Remove Row/Record");
    declareShortcut(mValues->mKeyBindings.mTableMoveup, "Move Record Up");
    declareShortcut(mValues->mKeyBindings.mTableMovedown, "Move Record Down");
    declareShortcut(mValues->mKeyBindings.mTableView, "View Record");
    declareShortcut(mValues->mKeyBindings.mTablePreview, "Preview Record");
    declareShortcut(mValues->mKeyBindings.mTableExtendeddelete, "Extended Record Deletion");
    declareShortcut(mValues->mKeyBindings.mTableExtendedrevert, "Extended Record Revertion");

    declareSubcategory("Report Table");
    declareShortcut(mValues->mKeyBindings.mReporttableShow, "Show Report");
    declareShortcut(mValues->mKeyBindings.mReporttableRemove, "Remove Report");
    declareShortcut(mValues->mKeyBindings.mReporttableReplace, "Replace Report");
    declareShortcut(mValues->mKeyBindings.mReporttableRefresh, "Refresh Report");

    declareSubcategory("Scene");
    declareShortcut(mValues->mKeyBindings.mSceneNaviPrimary, "Camera Rotation From Mouse Movement");
    declareShortcut(mValues->mKeyBindings.mSceneNaviSecondary, "Camera Translation From Mouse Movement");
    declareShortcut(mValues->mKeyBindings.mSceneOpenPrimary, "Primary Open");
    declareShortcut(mValues->mKeyBindings.mSceneEditPrimary, "Primary Edit");
    declareShortcut(mValues->mKeyBindings.mSceneEditSecondary, "Secondary Edit");
    declareShortcut(mValues->mKeyBindings.mSceneSelectPrimary, "Primary Select");
    declareShortcut(mValues->mKeyBindings.mSceneSelectSecondary, "Secondary Select");
    declareShortcut(mValues->mKeyBindings.mSceneSelectTertiary, "Tertiary Select");
    declareModifier(mValues->mKeyBindings.mSceneSpeedModifier, "Speed Modifier");
    declareShortcut(mValues->mKeyBindings.mSceneDelete, "Delete Instance");
    declareShortcut(mValues->mKeyBindings.mSceneInstanceDropTerrain, "Drop to terrain level");
    declareShortcut(mValues->mKeyBindings.mSceneInstanceDropCollision, "Drop to collision");
    declareShortcut(mValues->mKeyBindings.mSceneInstanceDropTerrainSeparately, "Drop to terrain level separately");
    declareShortcut(mValues->mKeyBindings.mSceneInstanceDropCollisionSeparately, "Drop to collision separately");
    declareShortcut(mValues->mKeyBindings.mSceneLoadCamCell, "Load Camera Cell");
    declareShortcut(mValues->mKeyBindings.mSceneLoadCamEastcell, "Load East Cell");
    declareShortcut(mValues->mKeyBindings.mSceneLoadCamNorthcell, "Load North Cell");
    declareShortcut(mValues->mKeyBindings.mSceneLoadCamWestcell, "Load West Cell");
    declareShortcut(mValues->mKeyBindings.mSceneLoadCamSouthcell, "Load South Cell");
    declareShortcut(mValues->mKeyBindings.mSceneEditAbort, "Abort");
    declareShortcut(mValues->mKeyBindings.mSceneFocusToolbar, "Toggle Toolbar Focus");
    declareShortcut(mValues->mKeyBindings.mSceneRenderStats, "Debug Rendering Stats");
    declareShortcut(mValues->mKeyBindings.mSceneDuplicate, "Duplicate Instance");
    declareShortcut(mValues->mKeyBindings.mSceneClearSelection, "Clear Selection");
    declareShortcut(mValues->mKeyBindings.mSceneUnhideAll, "Unhide All Objects");
    declareShortcut(mValues->mKeyBindings.mSceneToggleVisibility, "Toggle Selection Visibility");
    declareShortcut(mValues->mKeyBindings.mSceneGroup0, "Selection Group 0");
    declareShortcut(mValues->mKeyBindings.mSceneSave0, "Save Group 0");
    declareShortcut(mValues->mKeyBindings.mSceneGroup1, "Select Group 1");
    declareShortcut(mValues->mKeyBindings.mSceneSave1, "Save Group 1");
    declareShortcut(mValues->mKeyBindings.mSceneGroup2, "Select Group 2");
    declareShortcut(mValues->mKeyBindings.mSceneSave2, "Save Group 2");
    declareShortcut(mValues->mKeyBindings.mSceneGroup3, "Select Group 3");
    declareShortcut(mValues->mKeyBindings.mSceneSave3, "Save Group 3");
    declareShortcut(mValues->mKeyBindings.mSceneGroup4, "Select Group 4");
    declareShortcut(mValues->mKeyBindings.mSceneSave4, "Save Group 4");
    declareShortcut(mValues->mKeyBindings.mSceneGroup5, "Selection Group 5");
    declareShortcut(mValues->mKeyBindings.mSceneSave5, "Save Group 5");
    declareShortcut(mValues->mKeyBindings.mSceneGroup6, "Selection Group 6");
    declareShortcut(mValues->mKeyBindings.mSceneSave6, "Save Group 6");
    declareShortcut(mValues->mKeyBindings.mSceneGroup7, "Selection Group 7");
    declareShortcut(mValues->mKeyBindings.mSceneSave7, "Save Group 7");
    declareShortcut(mValues->mKeyBindings.mSceneGroup8, "Selection Group 8");
    declareShortcut(mValues->mKeyBindings.mSceneSave8, "Save Group 8");
    declareShortcut(mValues->mKeyBindings.mSceneGroup9, "Selection Group 9");
    declareShortcut(mValues->mKeyBindings.mSceneSave9, "Save Group 9");
    declareShortcut(mValues->mKeyBindings.mSceneAxisX, "X Axis Movement Lock");
    declareShortcut(mValues->mKeyBindings.mSceneAxisY, "Y Axis Movement Lock");
    declareShortcut(mValues->mKeyBindings.mSceneAxisZ, "Z Axis Movement Lock");
    declareShortcut(mValues->mKeyBindings.mSceneMoveSubmode, "Move Object Submode");
    declareShortcut(mValues->mKeyBindings.mSceneScaleSubmode, "Scale Object Submode");
    declareShortcut(mValues->mKeyBindings.mSceneRotateSubmode, "Rotate Object Submode");
    declareShortcut(mValues->mKeyBindings.mSceneCameraCycle, "Cycle Camera Mode");

    declareSubcategory("1st/Free Camera");
    declareShortcut(mValues->mKeyBindings.mFreeForward, "Forward");
    declareShortcut(mValues->mKeyBindings.mFreeBackward, "Backward");
    declareShortcut(mValues->mKeyBindings.mFreeLeft, "Left");
    declareShortcut(mValues->mKeyBindings.mFreeRight, "Right");
    declareShortcut(mValues->mKeyBindings.mFreeRollLeft, "Roll Left");
    declareShortcut(mValues->mKeyBindings.mFreeRollRight, "Roll Right");
    declareShortcut(mValues->mKeyBindings.mFreeSpeedMode, "Toggle Speed Mode");

    declareSubcategory("Orbit Camera");
    declareShortcut(mValues->mKeyBindings.mOrbitUp, "Up");
    declareShortcut(mValues->mKeyBindings.mOrbitDown, "Down");
    declareShortcut(mValues->mKeyBindings.mOrbitLeft, "Left");
    declareShortcut(mValues->mKeyBindings.mOrbitRight, "Right");
    declareShortcut(mValues->mKeyBindings.mOrbitRollLeft, "Roll Left");
    declareShortcut(mValues->mKeyBindings.mOrbitRollRight, "Roll Right");
    declareShortcut(mValues->mKeyBindings.mOrbitSpeedMode, "Toggle Speed Mode");
    declareShortcut(mValues->mKeyBindings.mOrbitCenterSelection, "Center On Selected");

    declareSubcategory("Script Editor");
    declareShortcut(mValues->mKeyBindings.mScriptEditorComment, "Comment Selection");
    declareShortcut(mValues->mKeyBindings.mScriptEditorUncomment, "Uncomment Selection");

    declareCategory("Models");
    declareString(mValues->mModels.mBaseanim, "base animations").setTooltip("3rd person base model with textkeys-data");
    declareString(mValues->mModels.mBaseanimkna, "base animations, kna")
        .setTooltip("3rd person beast race base model with textkeys-data");
    declareString(mValues->mModels.mBaseanimfemale, "base animations, female")
        .setTooltip("3rd person female base model with textkeys-data");
    declareString(mValues->mModels.mWolfskin, "base animations, wolf").setTooltip("3rd person werewolf skin");
}

void CSMPrefs::State::declareCategory(const std::string& key)
{
    std::map<std::string, Category>::iterator iter = mCategories.find(key);

    if (iter != mCategories.end())
    {
        mCurrentCategory = iter;
    }
    else
    {
        mCurrentCategory = mCategories.insert(std::make_pair(key, Category(this, key))).first;
    }
}

CSMPrefs::IntSetting& CSMPrefs::State::declareInt(Settings::SettingValue<int>& value, const QString& label)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    CSMPrefs::IntSetting* setting
        = new CSMPrefs::IntSetting(&mCurrentCategory->second, &mMutex, value.mName, label, *mIndex);

    mCurrentCategory->second.addSetting(setting);

    return *setting;
}

CSMPrefs::DoubleSetting& CSMPrefs::State::declareDouble(Settings::SettingValue<double>& value, const QString& label)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    CSMPrefs::DoubleSetting* setting
        = new CSMPrefs::DoubleSetting(&mCurrentCategory->second, &mMutex, value.mName, label, *mIndex);

    mCurrentCategory->second.addSetting(setting);

    return *setting;
}

CSMPrefs::BoolSetting& CSMPrefs::State::declareBool(Settings::SettingValue<bool>& value, const QString& label)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    CSMPrefs::BoolSetting* setting
        = new CSMPrefs::BoolSetting(&mCurrentCategory->second, &mMutex, value.mName, label, *mIndex);

    mCurrentCategory->second.addSetting(setting);

    return *setting;
}

CSMPrefs::EnumSetting& CSMPrefs::State::declareEnum(EnumSettingValue& value, const QString& label)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    CSMPrefs::EnumSetting* setting = new CSMPrefs::EnumSetting(
        &mCurrentCategory->second, &mMutex, value.getValue().mName, label, value.getEnumValues(), *mIndex);

    mCurrentCategory->second.addSetting(setting);

    return *setting;
}

CSMPrefs::ColourSetting& CSMPrefs::State::declareColour(
    Settings::SettingValue<std::string>& value, const QString& label)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    CSMPrefs::ColourSetting* setting
        = new CSMPrefs::ColourSetting(&mCurrentCategory->second, &mMutex, value.mName, label, *mIndex);

    mCurrentCategory->second.addSetting(setting);

    return *setting;
}

CSMPrefs::ShortcutSetting& CSMPrefs::State::declareShortcut(
    Settings::SettingValue<std::string>& value, const QString& label)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    // Setup with actual data
    QKeySequence sequence;

    getShortcutManager().convertFromString(value, sequence);
    getShortcutManager().setSequence(value.mName, sequence);

    CSMPrefs::ShortcutSetting* setting
        = new CSMPrefs::ShortcutSetting(&mCurrentCategory->second, &mMutex, value.mName, label, *mIndex);
    mCurrentCategory->second.addSetting(setting);

    return *setting;
}

CSMPrefs::StringSetting& CSMPrefs::State::declareString(
    Settings::SettingValue<std::string>& value, const QString& label)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    CSMPrefs::StringSetting* setting
        = new CSMPrefs::StringSetting(&mCurrentCategory->second, &mMutex, value.mName, label, *mIndex);

    mCurrentCategory->second.addSetting(setting);

    return *setting;
}

CSMPrefs::ModifierSetting& CSMPrefs::State::declareModifier(
    Settings::SettingValue<std::string>& value, const QString& label)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    // Setup with actual data
    int modifier;

    getShortcutManager().convertFromString(value.get(), modifier);
    getShortcutManager().setModifier(value.mName, modifier);

    CSMPrefs::ModifierSetting* setting
        = new CSMPrefs::ModifierSetting(&mCurrentCategory->second, &mMutex, value.mName, label, *mIndex);
    mCurrentCategory->second.addSetting(setting);

    return *setting;
}

void CSMPrefs::State::declareSubcategory(const QString& label)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    mCurrentCategory->second.addSubcategory(
        new CSMPrefs::Subcategory(&mCurrentCategory->second, &mMutex, label, *mIndex));
}

CSMPrefs::State::State(const Files::ConfigurationManager& configurationManager)
    : mConfigFile("openmw-cs.cfg")
    , mDefaultConfigFile("defaults-cs.bin")
    , mConfigurationManager(configurationManager)
    , mCurrentCategory(mCategories.end())
    , mIndex(std::make_unique<Settings::Index>())
    , mValues(std::make_unique<Values>(*mIndex))
{
    if (sThis)
        throw std::logic_error("An instance of CSMPRefs::State already exists");

    sThis = this;

    declare();
}

CSMPrefs::State::~State()
{
    sThis = nullptr;
}

void CSMPrefs::State::save()
{
    Settings::Manager::saveUser(mConfigurationManager.getUserConfigPath() / mConfigFile);
}

CSMPrefs::State::Iterator CSMPrefs::State::begin()
{
    return mCategories.begin();
}

CSMPrefs::State::Iterator CSMPrefs::State::end()
{
    return mCategories.end();
}

CSMPrefs::ShortcutManager& CSMPrefs::State::getShortcutManager()
{
    return mShortcutManager;
}

CSMPrefs::Category& CSMPrefs::State::operator[](const std::string& key)
{
    Iterator iter = mCategories.find(key);

    if (iter == mCategories.end())
        throw std::logic_error("Invalid user settings category: " + key);

    return iter->second;
}

void CSMPrefs::State::update(const Setting& setting)
{
    emit settingChanged(&setting);
}

CSMPrefs::State& CSMPrefs::State::get()
{
    if (!sThis)
        throw std::logic_error("No instance of CSMPrefs::State");

    return *sThis;
}

void CSMPrefs::State::resetCategory(const std::string& category)
{
    Collection::iterator container = mCategories.find(category);
    if (container != mCategories.end())
    {
        for (Setting* setting : container->second)
        {
            setting->reset();
            update(*setting);
        }
    }
}

void CSMPrefs::State::resetAll()
{
    for (Collection::iterator iter = mCategories.begin(); iter != mCategories.end(); ++iter)
    {
        resetCategory(iter->first);
    }
}

CSMPrefs::State& CSMPrefs::get()
{
    return State::get();
}
