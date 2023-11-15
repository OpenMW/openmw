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
    EnumValue scrollbarOnly("Scrollbar Only",
        "Simple addition of scrollbars, the view window "
        "does not grow automatically.");
    declareEnum("mainwindow-scrollbar", "Horizontal scrollbar mode for main window.", scrollbarOnly)
        .addValue(scrollbarOnly)
        .addValue("Grow Only", "The view window grows as subviews are added. No scrollbars.")
        .addValue("Grow then Scroll", "The view window grows. The scrollbar appears once it cannot grow any further.");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    declareBool(mValues->mWindows.mGrowLimit, "Grow Limit Screen")
        .setTooltip(
            "When \"Grow then Scroll\" option is selected, the window size grows to"
            " the width of the virtual desktop. \nIf this option is selected the the window growth"
            "is limited to the current screen.");
#endif

    declareCategory("Records");
    EnumValue iconAndText("Icon and Text");
    EnumValues recordValues;
    recordValues.add(iconAndText).add("Icon Only").add("Text Only");
    declareEnum("status-format", "Modification status display format", iconAndText).addValues(recordValues);
    declareEnum("type-format", "ID type display format", iconAndText).addValues(recordValues);

    declareCategory("ID Tables");
    EnumValue inPlaceEdit("Edit in Place", "Edit the clicked cell");
    EnumValue editRecord("Edit Record", "Open a dialogue subview for the clicked record");
    EnumValue view("View", "Open a scene subview for the clicked record (not available everywhere)");
    EnumValue editRecordAndClose("Edit Record and Close");
    EnumValues doubleClickValues;
    doubleClickValues.add(inPlaceEdit)
        .add(editRecord)
        .add(view)
        .add("Revert")
        .add("Delete")
        .add(editRecordAndClose)
        .add("View and Close", "Open a scene subview for the clicked record and close the table subview");
    declareEnum("double", "Double Click", inPlaceEdit).addValues(doubleClickValues);
    declareEnum("double-s", "Shift Double Click", editRecord).addValues(doubleClickValues);
    declareEnum("double-c", "Control Double Click", view).addValues(doubleClickValues);
    declareEnum("double-sc", "Shift Control Double Click", editRecordAndClose).addValues(doubleClickValues);
    EnumValue jumpAndSelect("Jump and Select", "Scroll new record into view and make it the selection");
    declareEnum("jump-to-added", "Action on adding or cloning a record", jumpAndSelect)
        .addValue(jumpAndSelect)
        .addValue("Jump Only", "Scroll new record into view")
        .addValue("No Jump", "No special action");
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

    declareCategory("ID Dialogues");
    declareBool(mValues->mIdDialogues.mToolbar, "Show toolbar");

    declareCategory("Reports");
    EnumValue actionNone("None");
    EnumValue actionEdit("Edit", "Open a table or dialogue suitable for addressing the listed report");
    EnumValue actionRemove("Remove", "Remove the report from the report table");
    EnumValue actionEditAndRemove("Edit And Remove",
        "Open a table or dialogue suitable for addressing the listed report, then remove the report from the report "
        "table");
    EnumValues reportValues;
    reportValues.add(actionNone).add(actionEdit).add(actionRemove).add(actionEditAndRemove);
    declareEnum("double", "Double Click", actionEdit).addValues(reportValues);
    declareEnum("double-s", "Shift Double Click", actionRemove).addValues(reportValues);
    declareEnum("double-c", "Control Double Click", actionEditAndRemove).addValues(reportValues);
    declareEnum("double-sc", "Shift Control Double Click", actionNone).addValues(reportValues);
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
    EnumValue warningsNormal("Normal", "Report warnings as warning");
    declareEnum("warnings", "Warning Mode", warningsNormal)
        .addValue("Ignore", "Do not report warning")
        .addValue(warningsNormal)
        .addValue("Strict", "Promote warning to an error");
    declareBool(mValues->mScripts.mToolbar, "Show toolbar");
    declareInt(mValues->mScripts.mCompileDelay, "Delay between updating of source errors")
        .setTooltip("Delay in milliseconds")
        .setRange(0, 10000);
    declareInt(mValues->mScripts.mErrorHeight, "Initial height of the error panel").setRange(100, 10000);
    declareBool(mValues->mScripts.mHighlightOccurrences, "Highlight other occurrences of selected names");
    declareColour("colour-highlight", "Colour of highlighted occurrences", QColor("lightcyan"));
    declareColour("colour-int", "Highlight Colour: Integer Literals", QColor("darkmagenta"));
    declareColour("colour-float", "Highlight Colour: Float Literals", QColor("magenta"));
    declareColour("colour-name", "Highlight Colour: Names", QColor("grey"));
    declareColour("colour-keyword", "Highlight Colour: Keywords", QColor("red"));
    declareColour("colour-special", "Highlight Colour: Special Characters", QColor("darkorange"));
    declareColour("colour-comment", "Highlight Colour: Comments", QColor("green"));
    declareColour("colour-id", "Highlight Colour: IDs", QColor("blue"));

    declareCategory("General Input");
    declareBool(mValues->mGeneralInput.mCycle, "Cyclic next/previous")
        .setTooltip(
            "When using next/previous functions at the last/first item of a "
            "list go to the first/last item");

    declareCategory("3D Scene Input");

    declareDouble("navi-wheel-factor", "Camera Zoom Sensitivity", 8).setRange(-100.0, 100.0);
    declareDouble("s-navi-sensitivity", "Secondary Camera Movement Sensitivity", 50.0).setRange(-1000.0, 1000.0);

    declareDouble("p-navi-free-sensitivity", "Free Camera Sensitivity", 1 / 650.).setPrecision(5).setRange(0.0, 1.0);
    declareBool(mValues->mSceneInput.mPNaviFreeInvert, "Invert Free Camera Mouse Input");
    declareDouble("navi-free-lin-speed", "Free Camera Linear Speed", 1000.0).setRange(1.0, 10000.0);
    declareDouble("navi-free-rot-speed", "Free Camera Rotational Speed", 3.14 / 2).setRange(0.001, 6.28);
    declareDouble("navi-free-speed-mult", "Free Camera Speed Multiplier (from Modifier)", 8).setRange(0.001, 1000.0);

    declareDouble("p-navi-orbit-sensitivity", "Orbit Camera Sensitivity", 1 / 650.).setPrecision(5).setRange(0.0, 1.0);
    declareBool(mValues->mSceneInput.mPNaviOrbitInvert, "Invert Orbit Camera Mouse Input");
    declareDouble("navi-orbit-rot-speed", "Orbital Camera Rotational Speed", 3.14 / 4).setRange(0.001, 6.28);
    declareDouble("navi-orbit-speed-mult", "Orbital Camera Speed Multiplier (from Modifier)", 4)
        .setRange(0.001, 1000.0);
    declareBool(mValues->mSceneInput.mNaviOrbitConstRoll, "Keep camera roll constant for orbital camera");

    declareBool(mValues->mSceneInput.mContextSelect, "Context Sensitive Selection");
    declareDouble("drag-factor", "Mouse sensitivity during drag operations", 1.0).setRange(0.001, 100.0);
    declareDouble("drag-wheel-factor", "Mouse wheel sensitivity during drag operations", 1.0).setRange(0.001, 100.0);
    declareDouble("drag-shift-factor", "Shift-acceleration factor during drag operations", 4.0)
        .setTooltip("Acceleration factor during drag operations while holding down shift")
        .setRange(0.001, 100.0);
    declareDouble("rotate-factor", "Free rotation factor", 0.007).setPrecision(4).setRange(0.0001, 0.1);

    declareCategory("Rendering");
    declareInt(mValues->mRendering.mFramerateLimit, "FPS limit")
        .setTooltip("Framerate limit in 3D preview windows. Zero value means \"unlimited\".")
        .setRange(0, 10000);
    declareInt(mValues->mRendering.mCameraFov, "Camera FOV").setRange(10, 170);
    declareBool(mValues->mRendering.mCameraOrtho, "Orthographic projection for camera");
    declareInt(mValues->mRendering.mCameraOrthoSize, "Orthographic projection size parameter")
        .setTooltip("Size of the orthographic frustum, greater value will allow the camera to see more of the world.")
        .setRange(10, 10000);
    declareDouble("object-marker-alpha", "Object Marker Transparency", 0.5).setPrecision(2).setRange(0, 1);
    declareBool(mValues->mRendering.mSceneUseGradient, "Use Gradient Background");
    declareColour("scene-day-background-colour", "Day Background Colour", QColor(110, 120, 128, 255));
    declareColour("scene-day-gradient-colour", "Day Gradient  Colour", QColor(47, 51, 51, 255))
        .setTooltip(
            "Sets the gradient color to use in conjunction with the day background color. Ignored if "
            "the gradient option is disabled.");
    declareColour("scene-bright-background-colour", "Scene Bright Background Colour", QColor(79, 87, 92, 255));
    declareColour("scene-bright-gradient-colour", "Scene Bright Gradient Colour", QColor(47, 51, 51, 255))
        .setTooltip(
            "Sets the gradient color to use in conjunction with the bright background color. Ignored if "
            "the gradient option is disabled.");
    declareColour("scene-night-background-colour", "Scene Night Background Colour", QColor(64, 77, 79, 255));
    declareColour("scene-night-gradient-colour", "Scene Night Gradient Colour", QColor(47, 51, 51, 255))
        .setTooltip(
            "Sets the gradient color to use in conjunction with the night background color. Ignored if "
            "the gradient option is disabled.");
    declareBool(mValues->mRendering.mSceneDayNightSwitchNodes, "Use Day/Night Switch Nodes");

    declareCategory("Tooltips");
    declareBool(mValues->mTooltips.mScene, "Show Tooltips in 3D scenes");
    declareBool(mValues->mTooltips.mSceneHideBasic, "Hide basic  3D scenes tooltips");
    declareInt(mValues->mTooltips.mSceneDelay, "Tooltip delay in milliseconds").setMin(1);

    EnumValue createAndInsert("Create cell and insert");
    EnumValue showAndInsert("Show cell and insert");
    EnumValue dontInsert("Discard");
    EnumValue insertAnyway("Insert anyway");
    EnumValues insertOutsideCell;
    insertOutsideCell.add(createAndInsert).add(dontInsert).add(insertAnyway);
    EnumValues insertOutsideVisibleCell;
    insertOutsideVisibleCell.add(showAndInsert).add(dontInsert).add(insertAnyway);

    EnumValue createAndLandEdit("Create cell and land, then edit");
    EnumValue showAndLandEdit("Show cell and edit");
    EnumValue dontLandEdit("Discard");
    EnumValues landeditOutsideCell;
    landeditOutsideCell.add(createAndLandEdit).add(dontLandEdit);
    EnumValues landeditOutsideVisibleCell;
    landeditOutsideVisibleCell.add(showAndLandEdit).add(dontLandEdit);

    EnumValue SelectOnly("Select only");
    EnumValue SelectAdd("Add to selection");
    EnumValue SelectRemove("Remove from selection");
    EnumValue selectInvert("Invert selection");
    EnumValues primarySelectAction;
    primarySelectAction.add(SelectOnly).add(SelectAdd).add(SelectRemove).add(selectInvert);
    EnumValues secondarySelectAction;
    secondarySelectAction.add(SelectOnly).add(SelectAdd).add(SelectRemove).add(selectInvert);

    declareCategory("3D Scene Editing");
    declareDouble("gridsnap-movement", "Grid snap size", 16);
    declareDouble("gridsnap-rotation", "Angle snap size", 15);
    declareDouble("gridsnap-scale", "Scale snap size", 0.25);
    declareInt(mValues->mSceneEditing.mDistance, "Drop Distance")
        .setTooltip(
            "If an instance drop can not be placed against another object at the "
            "insert point, it will be placed by this distance from the insert point instead");
    declareEnum("outside-drop", "Handling drops outside of cells", createAndInsert).addValues(insertOutsideCell);
    declareEnum("outside-visible-drop", "Handling drops outside of visible cells", showAndInsert)
        .addValues(insertOutsideVisibleCell);
    declareEnum("outside-landedit", "Handling terrain edit outside of cells", createAndLandEdit)
        .setTooltip("Behavior of terrain editing, if land editing brush reaches an area without cell record.")
        .addValues(landeditOutsideCell);
    declareEnum("outside-visible-landedit", "Handling terrain edit outside of visible cells", showAndLandEdit)
        .setTooltip("Behavior of terrain editing, if land editing brush reaches an area that is not currently visible.")
        .addValues(landeditOutsideVisibleCell);
    declareInt(mValues->mSceneEditing.mTexturebrushMaximumsize, "Maximum texture brush size").setMin(1);
    declareInt(mValues->mSceneEditing.mShapebrushMaximumsize, "Maximum height edit brush size")
        .setTooltip("Setting for the slider range of brush size in terrain height editing.")
        .setMin(1);
    declareBool(mValues->mSceneEditing.mLandeditPostSmoothpainting, "Smooth land after painting height")
        .setTooltip("Raise and lower tools will leave bumpy finish without this option");
    declareDouble("landedit-post-smoothstrength", "Smoothing strength (post-edit)", 0.25)
        .setTooltip(
            "If smoothing land after painting height is used, this is the percentage of smooth applied afterwards. "
            "Negative values may be used to roughen instead of smooth.")
        .setMin(-1)
        .setMax(1);
    declareBool(mValues->mSceneEditing.mOpenListView, "Open displays list view")
        .setTooltip(
            "When opening a reference from the scene view, it will open the"
            " instance list view instead of the individual instance record view.");
    declareEnum("primary-select-action", "Action for primary select", SelectOnly)
        .setTooltip(
            "Selection can be chosen between select only, add to selection, remove from selection and invert "
            "selection.")
        .addValues(primarySelectAction);
    declareEnum("secondary-select-action", "Action for secondary select", SelectAdd)
        .setTooltip(
            "Selection can be chosen between select only, add to selection, remove from selection and invert "
            "selection.")
        .addValues(secondarySelectAction);

    declareCategory("Key Bindings");

    declareSubcategory("Document");
    declareShortcut("document-file-newgame", "New Game", QKeySequence(Qt::ControlModifier | Qt::Key_N));
    declareShortcut("document-file-newaddon", "New Addon", QKeySequence());
    declareShortcut("document-file-open", "Open", QKeySequence(Qt::ControlModifier | Qt::Key_O));
    declareShortcut("document-file-save", "Save", QKeySequence(Qt::ControlModifier | Qt::Key_S));
    declareShortcut("document-help-help", "Help", QKeySequence(Qt::Key_F1));
    declareShortcut("document-help-tutorial", "Tutorial", QKeySequence());
    declareShortcut("document-file-verify", "Verify", QKeySequence());
    declareShortcut("document-file-merge", "Merge", QKeySequence());
    declareShortcut("document-file-errorlog", "Open Load Error Log", QKeySequence());
    declareShortcut("document-file-metadata", "Meta Data", QKeySequence());
    declareShortcut("document-file-close", "Close Document", QKeySequence(Qt::ControlModifier | Qt::Key_W));
    declareShortcut("document-file-exit", "Exit Application", QKeySequence(Qt::ControlModifier | Qt::Key_Q));
    declareShortcut("document-edit-undo", "Undo", QKeySequence(Qt::ControlModifier | Qt::Key_Z));
    declareShortcut("document-edit-redo", "Redo", QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_Z));
    declareShortcut("document-edit-preferences", "Open Preferences", QKeySequence());
    declareShortcut("document-edit-search", "Search", QKeySequence(Qt::ControlModifier | Qt::Key_F));
    declareShortcut("document-view-newview", "New View", QKeySequence());
    declareShortcut("document-view-statusbar", "Toggle Status Bar", QKeySequence());
    declareShortcut("document-view-filters", "Open Filter List", QKeySequence());
    declareShortcut("document-world-regions", "Open Region List", QKeySequence());
    declareShortcut("document-world-cells", "Open Cell List", QKeySequence());
    declareShortcut("document-world-referencables", "Open Object List", QKeySequence());
    declareShortcut("document-world-references", "Open Instance List", QKeySequence());
    declareShortcut("document-world-lands", "Open Lands List", QKeySequence());
    declareShortcut("document-world-landtextures", "Open Land Textures List", QKeySequence());
    declareShortcut("document-world-pathgrid", "Open Pathgrid List", QKeySequence());
    declareShortcut("document-world-regionmap", "Open Region Map", QKeySequence());
    declareShortcut("document-mechanics-globals", "Open Global List", QKeySequence());
    declareShortcut("document-mechanics-gamesettings", "Open Game Settings", QKeySequence());
    declareShortcut("document-mechanics-scripts", "Open Script List", QKeySequence());
    declareShortcut("document-mechanics-spells", "Open Spell List", QKeySequence());
    declareShortcut("document-mechanics-enchantments", "Open Enchantment List", QKeySequence());
    declareShortcut("document-mechanics-magiceffects", "Open Magic Effect List", QKeySequence());
    declareShortcut("document-mechanics-startscripts", "Open Start Script List", QKeySequence());
    declareShortcut("document-character-skills", "Open Skill List", QKeySequence());
    declareShortcut("document-character-classes", "Open Class List", QKeySequence());
    declareShortcut("document-character-factions", "Open Faction List", QKeySequence());
    declareShortcut("document-character-races", "Open Race List", QKeySequence());
    declareShortcut("document-character-birthsigns", "Open Birthsign List", QKeySequence());
    declareShortcut("document-character-topics", "Open Topic List", QKeySequence());
    declareShortcut("document-character-journals", "Open Journal List", QKeySequence());
    declareShortcut("document-character-topicinfos", "Open Topic Info List", QKeySequence());
    declareShortcut("document-character-journalinfos", "Open Journal Info List", QKeySequence());
    declareShortcut("document-character-bodyparts", "Open Body Part List", QKeySequence());
    declareShortcut("document-assets-reload", "Reload Assets", QKeySequence(Qt::Key_F5));
    declareShortcut("document-assets-sounds", "Open Sound Asset List", QKeySequence());
    declareShortcut("document-assets-soundgens", "Open Sound Generator List", QKeySequence());
    declareShortcut("document-assets-meshes", "Open Mesh Asset List", QKeySequence());
    declareShortcut("document-assets-icons", "Open Icon Asset List", QKeySequence());
    declareShortcut("document-assets-music", "Open Music Asset List", QKeySequence());
    declareShortcut("document-assets-soundres", "Open Sound File List", QKeySequence());
    declareShortcut("document-assets-textures", "Open Texture Asset List", QKeySequence());
    declareShortcut("document-assets-videos", "Open Video Asset List", QKeySequence());
    declareShortcut("document-debug-run", "Run Debug", QKeySequence());
    declareShortcut("document-debug-shutdown", "Stop Debug", QKeySequence());
    declareShortcut("document-debug-profiles", "Debug Profiles", QKeySequence());
    declareShortcut("document-debug-runlog", "Open Run Log", QKeySequence());

    declareSubcategory("Table");
    declareShortcut("table-edit", "Edit Record", QKeySequence());
    declareShortcut("table-add", "Add Row/Record", QKeySequence(Qt::ShiftModifier | Qt::Key_A));
    declareShortcut("table-clone", "Clone Record", QKeySequence(Qt::ShiftModifier | Qt::Key_D));
    declareShortcut("touch-record", "Touch Record", QKeySequence());
    declareShortcut("table-revert", "Revert Record", QKeySequence());
    declareShortcut("table-remove", "Remove Row/Record", QKeySequence(Qt::Key_Delete));
    declareShortcut("table-moveup", "Move Record Up", QKeySequence());
    declareShortcut("table-movedown", "Move Record Down", QKeySequence());
    declareShortcut("table-view", "View Record", QKeySequence(Qt::ShiftModifier | Qt::Key_C));
    declareShortcut("table-preview", "Preview Record", QKeySequence(Qt::ShiftModifier | Qt::Key_V));
    declareShortcut("table-extendeddelete", "Extended Record Deletion", QKeySequence());
    declareShortcut("table-extendedrevert", "Extended Record Revertion", QKeySequence());

    declareSubcategory("Report Table");
    declareShortcut("reporttable-show", "Show Report", QKeySequence());
    declareShortcut("reporttable-remove", "Remove Report", QKeySequence(Qt::Key_Delete));
    declareShortcut("reporttable-replace", "Replace Report", QKeySequence());
    declareShortcut("reporttable-refresh", "Refresh Report", QKeySequence());

    declareSubcategory("Scene");
    declareShortcut("scene-navi-primary", "Camera Rotation From Mouse Movement", QKeySequence(Qt::LeftButton));
    declareShortcut("scene-navi-secondary", "Camera Translation From Mouse Movement",
        QKeySequence(Qt::ControlModifier | (int)Qt::LeftButton));
    declareShortcut("scene-open-primary", "Primary Open", QKeySequence(Qt::ShiftModifier | (int)Qt::LeftButton));
    declareShortcut("scene-edit-primary", "Primary Edit", QKeySequence(Qt::RightButton));
    declareShortcut("scene-edit-secondary", "Secondary Edit", QKeySequence(Qt::ControlModifier | (int)Qt::RightButton));
    declareShortcut("scene-select-primary", "Primary Select", QKeySequence(Qt::MiddleButton));
    declareShortcut(
        "scene-select-secondary", "Secondary Select", QKeySequence(Qt::ControlModifier | (int)Qt::MiddleButton));
    declareShortcut(
        "scene-select-tertiary", "Tertiary Select", QKeySequence(Qt::ShiftModifier | (int)Qt::MiddleButton));
    declareModifier(mValues->mKeyBindings.mSceneSpeedModifier, "Speed Modifier");
    declareShortcut("scene-delete", "Delete Instance", QKeySequence(Qt::Key_Delete));
    declareShortcut("scene-instance-drop-terrain", "Drop to terrain level", QKeySequence(Qt::Key_G));
    declareShortcut("scene-instance-drop-collision", "Drop to collision", QKeySequence(Qt::Key_H));
    declareShortcut("scene-instance-drop-terrain-separately", "Drop to terrain level separately", QKeySequence());
    declareShortcut("scene-instance-drop-collision-separately", "Drop to collision separately", QKeySequence());
    declareShortcut("scene-load-cam-cell", "Load Camera Cell", QKeySequence(Qt::KeypadModifier | Qt::Key_5));
    declareShortcut("scene-load-cam-eastcell", "Load East Cell", QKeySequence(Qt::KeypadModifier | Qt::Key_6));
    declareShortcut("scene-load-cam-northcell", "Load North Cell", QKeySequence(Qt::KeypadModifier | Qt::Key_8));
    declareShortcut("scene-load-cam-westcell", "Load West Cell", QKeySequence(Qt::KeypadModifier | Qt::Key_4));
    declareShortcut("scene-load-cam-southcell", "Load South Cell", QKeySequence(Qt::KeypadModifier | Qt::Key_2));
    declareShortcut("scene-edit-abort", "Abort", QKeySequence(Qt::Key_Escape));
    declareShortcut("scene-focus-toolbar", "Toggle Toolbar Focus", QKeySequence(Qt::Key_T));
    declareShortcut("scene-render-stats", "Debug Rendering Stats", QKeySequence(Qt::Key_F3));
    declareShortcut("scene-duplicate", "Duplicate Instance", QKeySequence(Qt::ShiftModifier | Qt::Key_C));

    declareSubcategory("1st/Free Camera");
    declareShortcut("free-forward", "Forward", QKeySequence(Qt::Key_W));
    declareShortcut("free-backward", "Backward", QKeySequence(Qt::Key_S));
    declareShortcut("free-left", "Left", QKeySequence(Qt::Key_A));
    declareShortcut("free-right", "Right", QKeySequence(Qt::Key_D));
    declareShortcut("free-roll-left", "Roll Left", QKeySequence(Qt::Key_Q));
    declareShortcut("free-roll-right", "Roll Right", QKeySequence(Qt::Key_E));
    declareShortcut("free-speed-mode", "Toggle Speed Mode", QKeySequence(Qt::Key_F));

    declareSubcategory("Orbit Camera");
    declareShortcut("orbit-up", "Up", QKeySequence(Qt::Key_W));
    declareShortcut("orbit-down", "Down", QKeySequence(Qt::Key_S));
    declareShortcut("orbit-left", "Left", QKeySequence(Qt::Key_A));
    declareShortcut("orbit-right", "Right", QKeySequence(Qt::Key_D));
    declareShortcut("orbit-roll-left", "Roll Left", QKeySequence(Qt::Key_Q));
    declareShortcut("orbit-roll-right", "Roll Right", QKeySequence(Qt::Key_E));
    declareShortcut("orbit-speed-mode", "Toggle Speed Mode", QKeySequence(Qt::Key_F));
    declareShortcut("orbit-center-selection", "Center On Selected", QKeySequence(Qt::Key_C));

    declareSubcategory("Script Editor");
    declareShortcut("script-editor-comment", "Comment Selection", QKeySequence());
    declareShortcut("script-editor-uncomment", "Uncomment Selection", QKeySequence());

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

CSMPrefs::DoubleSetting& CSMPrefs::State::declareDouble(const std::string& key, const QString& label, double default_)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    CSMPrefs::DoubleSetting* setting
        = new CSMPrefs::DoubleSetting(&mCurrentCategory->second, &mMutex, key, label, *mIndex);

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

CSMPrefs::EnumSetting& CSMPrefs::State::declareEnum(const std::string& key, const QString& label, EnumValue default_)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    CSMPrefs::EnumSetting* setting = new CSMPrefs::EnumSetting(&mCurrentCategory->second, &mMutex, key, label, *mIndex);

    mCurrentCategory->second.addSetting(setting);

    return *setting;
}

CSMPrefs::ColourSetting& CSMPrefs::State::declareColour(const std::string& key, const QString& label, QColor default_)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    CSMPrefs::ColourSetting* setting
        = new CSMPrefs::ColourSetting(&mCurrentCategory->second, &mMutex, key, label, *mIndex);

    mCurrentCategory->second.addSetting(setting);

    return *setting;
}

CSMPrefs::ShortcutSetting& CSMPrefs::State::declareShortcut(
    const std::string& key, const QString& label, const QKeySequence& default_)
{
    if (mCurrentCategory == mCategories.end())
        throw std::logic_error("no category for setting");

    // Setup with actual data
    QKeySequence sequence;

    getShortcutManager().convertFromString(mIndex->get<std::string>(mCurrentCategory->second.getKey(), key), sequence);
    getShortcutManager().setSequence(key, sequence);

    CSMPrefs::ShortcutSetting* setting
        = new CSMPrefs::ShortcutSetting(&mCurrentCategory->second, &mMutex, key, label, *mIndex);
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
