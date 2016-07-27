
#include "state.hpp"

#include <stdexcept>
#include <algorithm>
#include <sstream>

#include <QMetaEnum>

#include "intsetting.hpp"
#include "doublesetting.hpp"
#include "boolsetting.hpp"
#include "coloursetting.hpp"
#include "shortcutsetting.hpp"
#include "modifiersetting.hpp"

CSMPrefs::State *CSMPrefs::State::sThis = 0;

void CSMPrefs::State::load()
{
    // default settings file
    boost::filesystem::path local = mConfigurationManager.getLocalPath() / mConfigFile;
    boost::filesystem::path global = mConfigurationManager.getGlobalPath() / mConfigFile;

    if (boost::filesystem::exists (local))
        mSettings.loadDefault (local.string());
    else if (boost::filesystem::exists (global))
        mSettings.loadDefault (global.string());
    else
        throw std::runtime_error ("No default settings file found! Make sure the file \"openmw-cs.cfg\" was properly installed.");

    // user settings file
    boost::filesystem::path user = mConfigurationManager.getUserConfigPath() / mConfigFile;

    if (boost::filesystem::exists (user))
        mSettings.loadUser (user.string());
}

void CSMPrefs::State::declare()
{
    declareCategory ("Windows");
    declareInt ("default-width", "Default window width", 800).
        setTooltip ("Newly opened top-level windows will open with this width.").
        setMin (80);
    declareInt ("default-height", "Default window height", 600).
        setTooltip ("Newly opened top-level windows will open with this height.").
        setMin (80);
    declareBool ("show-statusbar", "Show Status Bar", true).
        setTooltip ("If a newly open top level window is showing status bars or not. "
        " Note that this does not affect existing windows.");
    declareSeparator();
    declareBool ("reuse", "Reuse Subviews", true).
        setTooltip ("When a new subview is requested and a matching subview already "
        " exist, do not open a new subview and use the existing one instead.");
    declareInt ("max-subviews", "Maximum number of subviews per top-level window", 256).
        setTooltip ("If the maximum number is reached and a new subview is opened "
            "it will be placed into a new top-level window.").
        setRange (1, 256);
    declareBool ("hide-subview", "Hide single subview", false).
        setTooltip ("When a view contains only a single subview, hide the subview title "
        "bar and if this subview is closed also close the view (unless it is the last "
        "view for this document)");
    declareInt ("minimum-width", "Minimum subview width", 325).
        setTooltip ("Minimum width of subviews.").
        setRange (50, 10000);
    declareSeparator();
    EnumValue scrollbarOnly ("Scrollbar Only", "Simple addition of scrollbars, the view window "
        "does not grow automatically.");
    declareEnum ("mainwindow-scrollbar", "Horizontal scrollbar mode for main window.", scrollbarOnly).
        addValue (scrollbarOnly).
        addValue ("Grow Only", "The view window grows as subviews are added. No scrollbars.").
        addValue ("Grow then Scroll", "The view window grows. The scrollbar appears once it cannot grow any further.");
    declareBool ("grow-limit", "Grow Limit Screen", false).
        setTooltip ("When \"Grow then Scroll\" option is selected, the window size grows to"
        " the width of the virtual desktop. \nIf this option is selected the the window growth"
        "is limited to the current screen.");

    declareCategory ("Records");
    EnumValue iconAndText ("Icon and Text");
    EnumValues recordValues;
    recordValues.add (iconAndText).add ("Icon Only").add ("Text Only");
    declareEnum ("status-format", "Modification status display format", iconAndText).
        addValues (recordValues);
    declareEnum ("type-format", "ID type display format", iconAndText).
        addValues (recordValues);

    declareCategory ("ID Tables");
    EnumValue inPlaceEdit ("Edit in Place", "Edit the clicked cell");
    EnumValue editRecord ("Edit Record", "Open a dialogue subview for the clicked record");
    EnumValue view ("View", "Open a scene subview for the clicked record (not available everywhere)");
    EnumValue editRecordAndClose ("Edit Record and Close");
    EnumValues doubleClickValues;
    doubleClickValues.add (inPlaceEdit).add (editRecord).add (view).add ("Revert").
        add ("Delete").add (editRecordAndClose).
        add ("View and Close", "Open a scene subview for the clicked record and close the table subview");
    declareEnum ("double", "Double Click", inPlaceEdit).addValues (doubleClickValues);
    declareEnum ("double-s", "Shift Double Click", editRecord).addValues (doubleClickValues);
    declareEnum ("double-c", "Control Double Click", view).addValues (doubleClickValues);
    declareEnum ("double-sc", "Shift Control Double Click", editRecordAndClose).addValues (doubleClickValues);
    declareSeparator();
    EnumValue jumpAndSelect ("Jump and Select", "Scroll new record into view and make it the selection");
    declareEnum ("jump-to-added", "Action on adding or cloning a record", jumpAndSelect).
        addValue (jumpAndSelect).
        addValue ("Jump Only", "Scroll new record into view").
        addValue ("No Jump", "No special action");
    declareBool ("extended-config",
        "Manually specify affected record types for an extended delete/revert", false).
        setTooltip ("Delete and revert commands have an extended form that also affects "
        "associated records.\n\n"
        "If this option is enabled, types of affected records are selected "
        "manually before a command execution.\nOtherwise, all associated "
        "records are deleted/reverted immediately.");

    declareCategory ("ID Dialogues");
    declareBool ("toolbar", "Show toolbar", true);

    declareCategory ("Reports");
    EnumValue actionNone ("None");
    EnumValue actionEdit ("Edit", "Open a table or dialogue suitable for addressing the listed report");
    EnumValue actionRemove ("Remove", "Remove the report from the report table");
    EnumValue actionEditAndRemove ("Edit And Remove", "Open a table or dialogue suitable for addressing the listed report, then remove the report from the report table");
    EnumValues reportValues;
    reportValues.add (actionNone).add (actionEdit).add (actionRemove).add (actionEditAndRemove);
    declareEnum ("double", "Double Click", actionEdit).addValues (reportValues);
    declareEnum ("double-s", "Shift Double Click", actionRemove).addValues (reportValues);
    declareEnum ("double-c", "Control Double Click", actionEditAndRemove).addValues (reportValues);
    declareEnum ("double-sc", "Shift Control Double Click", actionNone).addValues (reportValues);

    declareCategory ("Search & Replace");
    declareInt ("char-before", "Characters before search string", 10).
        setTooltip ("Maximum number of character to display in search result before the searched text");
    declareInt ("char-after", "Characters after search string", 10).
        setTooltip ("Maximum number of character to display in search result after the searched text");
    declareBool ("auto-delete", "Delete row from result table after a successful replace", true);

    declareCategory ("Scripts");
    declareBool ("show-linenum", "Show Line Numbers", true).
        setTooltip ("Show line numbers to the left of the script editor window."
        "The current row and column numbers of the text cursor are shown at the bottom.");
    declareBool ("wrap-lines", "Wrap Lines", false).
        setTooltip ("Wrap lines longer than width of script editor.");
    declareBool ("mono-font", "Use monospace font", true);
    declareInt ("tab-width", "Tab Width", 4).
        setTooltip ("Number of characters for tab width").
        setRange (1, 10);
    EnumValue warningsNormal ("Normal", "Report warnings as warning");
    declareEnum ("warnings", "Warning Mode", warningsNormal).
        addValue ("Ignore", "Do not report warning").
        addValue (warningsNormal).
        addValue ("Strict", "Promote warning to an error");
    declareBool ("toolbar", "Show toolbar", true);
    declareInt ("compile-delay", "Delay between updating of source errors", 100).
        setTooltip ("Delay in milliseconds").
        setRange (0, 10000);
    declareInt ("error-height", "Initial height of the error panel", 100).
        setRange (100, 10000);
    declareSeparator();
    declareColour ("colour-int", "Highlight Colour: Integer Literals", QColor ("darkmagenta"));
    declareColour ("colour-float", "Highlight Colour: Float Literals", QColor ("magenta"));
    declareColour ("colour-name", "Highlight Colour: Names", QColor ("grey"));
    declareColour ("colour-keyword", "Highlight Colour: Keywords", QColor ("red"));
    declareColour ("colour-special", "Highlight Colour: Special Characters", QColor ("darkorange"));
    declareColour ("colour-comment", "Highlight Colour: Comments", QColor ("green"));
    declareColour ("colour-id", "Highlight Colour: IDs", QColor ("blue"));

    declareCategory ("General Input");
    declareBool ("cycle", "Cyclic next/previous", false).
        setTooltip ("When using next/previous functions at the last/first item of a "
        "list go to the first/last item");

    declareCategory ("3D Scene Input");
    declareDouble ("p-navi-free-sensitivity", "Free Camera Sensitivity", 1/650.).setPrecision(5).setRange(0.0, 1.0);
    declareBool ("p-navi-free-invert", "Invert Free Camera Mouse Input", false);
    declareDouble ("p-navi-orbit-sensitivity", "Orbit Camera Sensitivity", 1/650.).setPrecision(5).setRange(0.0, 1.0);
    declareBool ("p-navi-orbit-invert", "Invert Orbit Camera Mouse Input", false);
    declareDouble ("s-navi-sensitivity", "Secondary Camera Movement Sensitivity", 50.0).setRange(-1000.0, 1000.0);
    declareDouble ("navi-wheel-factor", "Camera Zoom Sensitivity", 8).setRange(-100.0, 100.0);
    declareDouble ("navi-free-lin-speed", "Free Camera Linear Speed", 1000.0).setRange(1.0, 10000.0);
    declareDouble ("navi-free-rot-speed", "Free Camera Rotational Speed", 3.14 / 2).setRange(0.001, 6.28);
    declareDouble ("navi-free-speed-mult", "Free Camera Speed Multiplier (from Modifier)", 8).setRange(0.001, 1000.0);
    declareDouble ("navi-orbit-rot-speed", "Orbital Camera Rotational Speed", 3.14 / 4).setRange(0.001, 6.28);
    declareDouble ("navi-orbit-speed-mult", "Orbital Camera Speed Multiplier (from Modifier)", 4).setRange(0.001, 1000.0);
    declareSeparator();
    declareBool ("context-select", "Context Sensitive Selection", false);
    declareDouble ("drag-factor", "Mouse sensitivity during drag operations", 1.0).
        setRange (0.001, 100.0);
    declareDouble ("drag-wheel-factor", "Mouse wheel sensitivity during drag operations", 1.0).
        setRange (0.001, 100.0);
    declareDouble ("drag-shift-factor",
            "Shift-acceleration factor during drag operations", 4.0).
        setTooltip ("Acceleration factor during drag operations while holding down shift").
        setRange (0.001, 100.0);

    declareCategory ("Tooltips");
    declareBool ("scene", "Show Tooltips in 3D scenes", true);
    declareBool ("scene-hide-basic", "Hide basic  3D scenes tooltips", false);
    declareInt ("scene-delay", "Tooltip delay in milliseconds", 500).
        setMin (1);

    EnumValue createAndInsert ("Create cell and insert");
    EnumValue showAndInsert ("Show cell and insert");
    EnumValue dontInsert ("Discard");
    EnumValue insertAnyway ("Insert anyway");
    EnumValues insertOutsideCell;
    insertOutsideCell.add (createAndInsert).add (dontInsert).add (insertAnyway);
    EnumValues insertOutsideVisibleCell;
    insertOutsideVisibleCell.add (showAndInsert).add (dontInsert).add (insertAnyway);

    declareCategory ("Scene Drops");
    declareInt ("distance", "Drop Distance", 50).
        setTooltip ("If an instance drop can not be placed against another object at the "
            "insert point, it will be placed by this distance from the insert point instead");
    declareEnum ("outside-drop", "Handling drops outside of cells", createAndInsert).
        addValues (insertOutsideCell);
    declareEnum ("outside-visible-drop", "Handling drops outside of visible cells", showAndInsert).
        addValues (insertOutsideVisibleCell);

    declareCategory ("Key Bindings");

    declareSubcategory ("Document");
    declareShortcut ("document-file-newgame", "Create new game", QKeySequence());
    declareShortcut ("document-file-newaddon", "Create new addon", QKeySequence());
    declareShortcut ("document-file-open", "Open", QKeySequence());
    declareShortcut ("document-file-save", "Save", QKeySequence(Qt::ControlModifier | Qt::Key_S));
    declareShortcut ("document-file-verify", "Verify", QKeySequence(Qt::ControlModifier | Qt::Key_V));
    declareShortcut ("document-file-merge", "Merge", QKeySequence());
    declareShortcut ("document-file-errorlog", "Load error log", QKeySequence());
    declareShortcut ("document-file-metadata", "Meta Data", QKeySequence());
    declareShortcut ("document-file-close", "Close", QKeySequence());
    declareShortcut ("document-file-exit", "Exit", QKeySequence());
    declareShortcut ("document-edit-undo", "Undo", QKeySequence(Qt::ControlModifier | Qt::Key_Z));
    declareShortcut ("document-edit-redo", "Redo", QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_Z));
    declareShortcut ("document-edit-preferences", "Show Preferences", QKeySequence());
    declareShortcut ("document-edit-search", "Show Search", QKeySequence());
    declareShortcut ("document-view-newview", "New View", QKeySequence());
    declareShortcut ("document-view-statusbar", "Show Status Bar", QKeySequence());
    declareShortcut ("document-view-filters", "Show Filters", QKeySequence());
    declareShortcut ("document-world-regions", "Show Regions", QKeySequence());
    declareShortcut ("document-world-cells", "Show Cells", QKeySequence());
    declareShortcut ("document-world-referencables", "Show Referencables", QKeySequence());
    declareShortcut ("document-world-references", "Show References", QKeySequence());
    declareShortcut ("document-world-pathgrid", "Show Pathgrids", QKeySequence());
    declareShortcut ("document-world-regionmap", "Show Region Map", QKeySequence());
    declareShortcut ("document-mechanics-globals", "Show Globals", QKeySequence());
    declareShortcut ("document-mechanics-gamesettings", "Show Game Settings", QKeySequence());
    declareShortcut ("document-mechanics-scripts", "Show Scripts", QKeySequence());
    declareShortcut ("document-mechanics-spells", "Show Spells", QKeySequence());
    declareShortcut ("document-mechanics-enchantments", "Show Enchantments", QKeySequence());
    declareShortcut ("document-mechanics-magiceffects", "Show Magic Effects", QKeySequence());
    declareShortcut ("document-mechanics-startscripts", "Show Start Scripts", QKeySequence());
    declareShortcut ("document-character-skills", "Show Skills", QKeySequence());
    declareShortcut ("document-character-classes", "Show Classes", QKeySequence());
    declareShortcut ("document-character-factions", "Show Factions", QKeySequence());
    declareShortcut ("document-character-races", "Show Races", QKeySequence());
    declareShortcut ("document-character-birthsigns", "Show Birthsigns", QKeySequence());
    declareShortcut ("document-character-topics", "Show Topics", QKeySequence());
    declareShortcut ("document-character-journals", "Show Journals", QKeySequence());
    declareShortcut ("document-character-topicinfos", "Show Topic Infos", QKeySequence());
    declareShortcut ("document-character-journalinfos", "Show Journal Infos", QKeySequence());
    declareShortcut ("document-character-bodyparts", "Show Body Parts", QKeySequence());
    declareShortcut ("document-assets-sounds", "Show Sound Assets", QKeySequence());
    declareShortcut ("document-assets-soundgens", "Show Sound Generators", QKeySequence());
    declareShortcut ("document-assets-meshes", "Show Mesh Assets", QKeySequence());
    declareShortcut ("document-assets-icons", "Show Icon Assets", QKeySequence());
    declareShortcut ("document-assets-music", "Show Music Assets", QKeySequence());
    declareShortcut ("document-assets-soundres", "Show Sound Files", QKeySequence());
    declareShortcut ("document-assets-textures", "TShow exture Assets", QKeySequence());
    declareShortcut ("document-assets-videos", "Show Video Assets", QKeySequence());
    declareShortcut ("document-debug-run", "Run Debug", QKeySequence());
    declareShortcut ("document-debug-shutdown", "Stop Debug", QKeySequence());
    declareShortcut ("document-debug-runlog", "Run Log", QKeySequence());

    declareSubcategory ("Table");
    declareShortcut ("table-edit", "Edit record", QKeySequence());
    declareShortcut ("table-add", "Add row/record", QKeySequence(Qt::ControlModifier | Qt::Key_N));
    declareShortcut ("table-clone", "Clone record", QKeySequence());
    declareShortcut ("table-revert", "Revert record", QKeySequence());
    declareShortcut ("table-remove", "Remove row/record", QKeySequence(Qt::Key_Delete));
    declareShortcut ("table-moveup", "Move record up", QKeySequence());
    declareShortcut ("table-movedown", "Move record down", QKeySequence());
    declareShortcut ("table-view", "View record", QKeySequence());
    declareShortcut ("table-preview", "Preview record", QKeySequence());
    declareShortcut ("table-extendeddelete", "Extended record deletion", QKeySequence());
    declareShortcut ("table-extendedrevert", "Extended record revertion", QKeySequence());

    declareSubcategory ("Report Table");
    declareShortcut ("reporttable-show", "Show report", QKeySequence());
    declareShortcut ("reporttable-remove", "Remove report", QKeySequence(Qt::Key_Delete));
    declareShortcut ("reporttable-replace", "Replace report", QKeySequence());
    declareShortcut ("reporttable-refresh", "Refresh report", QKeySequence());

    declareSubcategory ("1st/Free Camera");
    declareShortcut ("free-forward", "Forward", QKeySequence(Qt::Key_W), Qt::Key_Shift);
    declareShortcut ("free-backward", "Backward", QKeySequence(Qt::Key_S));
    declareShortcut ("free-left", "Left", QKeySequence(Qt::Key_A));
    declareShortcut ("free-right", "Right", QKeySequence(Qt::Key_D));
    declareModifier ("free-forward", "Speed modifier");
    declareShortcut ("free-roll-left", "Roll left", QKeySequence(Qt::Key_Q));
    declareShortcut ("free-roll-right", "Roll right", QKeySequence(Qt::Key_E));
    declareShortcut ("free-speed-mode", "Speed mode toggle", QKeySequence(Qt::Key_F));

    declareSubcategory ("Orbit Camera");
    declareShortcut ("orbit-up", "Up", QKeySequence(Qt::Key_W), Qt::Key_Shift);
    declareShortcut ("orbit-down", "Down", QKeySequence(Qt::Key_S));
    declareShortcut ("orbit-left", "Left", QKeySequence(Qt::Key_A));
    declareShortcut ("orbit-right", "Right", QKeySequence(Qt::Key_D));
    declareModifier ("orbit-up", "Speed modifier");
    declareShortcut ("orbit-roll-left", "Roll left", QKeySequence(Qt::Key_Q));
    declareShortcut ("orbit-roll-right", "Roll right", QKeySequence(Qt::Key_E));
    declareShortcut ("orbit-speed-mode", "Speed mode toggle", QKeySequence(Qt::Key_F));
    declareShortcut ("orbit-center-selection", "Center on selected", QKeySequence(Qt::Key_C));

    declareSubcategory ("Scene");
    declareShortcut ("scene-navi-primary", "Camera rotation from mouse movement", QKeySequence(Qt::LeftButton));
    declareShortcut ("scene-navi-secondary", "Camera translation from mouse movement",
        QKeySequence(Qt::ControlModifier | (int)Qt::LeftButton));
    declareShortcut ("scene-edit-primary", "Primary edit", QKeySequence(Qt::RightButton));
    declareShortcut ("scene-edit-secondary", "Secondary edit",
        QKeySequence(Qt::ControlModifier | (int)Qt::RightButton));
    declareShortcut ("scene-select-primary", "Primary select", QKeySequence(Qt::MiddleButton));
    declareShortcut ("scene-select-secondary", "Secondary select",
        QKeySequence(Qt::ControlModifier | (int)Qt::MiddleButton));
    declareShortcut ("scene-edit-abort", "Abort", QKeySequence(Qt::Key_Escape));
    declareShortcut ("scene-focus-toolbar", "Toggle toolbar focus", QKeySequence(Qt::Key_T));
    declareShortcut ("scene-render-stats", "Debug rendering stats", QKeySequence(Qt::Key_F3));
}

void CSMPrefs::State::declareCategory (const std::string& key)
{
    std::map<std::string, Category>::iterator iter = mCategories.find (key);

    if (iter!=mCategories.end())
    {
        mCurrentCategory = iter;
    }
    else
    {
        mCurrentCategory =
            mCategories.insert (std::make_pair (key, Category (this, key))).first;
    }
}

CSMPrefs::IntSetting& CSMPrefs::State::declareInt (const std::string& key,
    const std::string& label, int default_)
{
    if (mCurrentCategory==mCategories.end())
        throw std::logic_error ("no category for setting");

    std::ostringstream stream;
    stream << default_;
    setDefault (key, stream.str());

    default_ = mSettings.getInt (key, mCurrentCategory->second.getKey());

    CSMPrefs::IntSetting *setting =
        new CSMPrefs::IntSetting (&mCurrentCategory->second, &mSettings, &mMutex, key, label,
        default_);

    mCurrentCategory->second.addSetting (setting);

    return *setting;
}

CSMPrefs::DoubleSetting& CSMPrefs::State::declareDouble (const std::string& key,
    const std::string& label, double default_)
{
    if (mCurrentCategory==mCategories.end())
        throw std::logic_error ("no category for setting");

    std::ostringstream stream;
    stream << default_;
    setDefault (key, stream.str());

    default_ = mSettings.getFloat (key, mCurrentCategory->second.getKey());

    CSMPrefs::DoubleSetting *setting =
        new CSMPrefs::DoubleSetting (&mCurrentCategory->second, &mSettings, &mMutex,
        key, label, default_);

    mCurrentCategory->second.addSetting (setting);

    return *setting;
}

CSMPrefs::BoolSetting& CSMPrefs::State::declareBool (const std::string& key,
    const std::string& label, bool default_)
{
    if (mCurrentCategory==mCategories.end())
        throw std::logic_error ("no category for setting");

    setDefault (key, default_ ? "true" : "false");

    default_ = mSettings.getBool (key, mCurrentCategory->second.getKey());

    CSMPrefs::BoolSetting *setting =
        new CSMPrefs::BoolSetting (&mCurrentCategory->second, &mSettings, &mMutex, key, label,
        default_);

    mCurrentCategory->second.addSetting (setting);

    return *setting;
}

CSMPrefs::EnumSetting& CSMPrefs::State::declareEnum (const std::string& key,
    const std::string& label, EnumValue default_)
{
    if (mCurrentCategory==mCategories.end())
        throw std::logic_error ("no category for setting");

    setDefault (key, default_.mValue);

    default_.mValue = mSettings.getString (key, mCurrentCategory->second.getKey());

    CSMPrefs::EnumSetting *setting =
        new CSMPrefs::EnumSetting (&mCurrentCategory->second, &mSettings, &mMutex, key, label,
        default_);

    mCurrentCategory->second.addSetting (setting);

    return *setting;
}

CSMPrefs::ColourSetting& CSMPrefs::State::declareColour (const std::string& key,
    const std::string& label, QColor default_)
{
    if (mCurrentCategory==mCategories.end())
        throw std::logic_error ("no category for setting");

    setDefault (key, default_.name().toUtf8().data());

    default_.setNamedColor (QString::fromUtf8 (mSettings.getString (key, mCurrentCategory->second.getKey()).c_str()));

    CSMPrefs::ColourSetting *setting =
        new CSMPrefs::ColourSetting (&mCurrentCategory->second, &mSettings, &mMutex, key, label,
        default_);

    mCurrentCategory->second.addSetting (setting);

    return *setting;
}

CSMPrefs::ShortcutSetting& CSMPrefs::State::declareShortcut (const std::string& key, const std::string& label,
    const QKeySequence& default_, int modifier_)
{
    if (mCurrentCategory==mCategories.end())
        throw std::logic_error ("no category for setting");

    std::string seqStr = getShortcutManager().convertToString(default_, modifier_);
    setDefault (key, seqStr);

    // Setup with actual data
    QKeySequence sequence;
    int mod;

    getShortcutManager().convertFromString(mSettings.getString(key, mCurrentCategory->second.getKey()), sequence, mod);
    getShortcutManager().setSequence(key, sequence, mod);

    CSMPrefs::ShortcutSetting *setting = new CSMPrefs::ShortcutSetting (&mCurrentCategory->second, &mSettings, &mMutex,
        key, label);
    mCurrentCategory->second.addSetting (setting);

    return *setting;
}

CSMPrefs::ModifierSetting& CSMPrefs::State::declareModifier(const std::string& key, const std::string& label)
{
    if (mCurrentCategory==mCategories.end())
        throw std::logic_error ("no category for setting");

    CSMPrefs::ModifierSetting *setting = new CSMPrefs::ModifierSetting (&mCurrentCategory->second, &mSettings, &mMutex,
        key, label);
    mCurrentCategory->second.addSetting (setting);

    return *setting;
}

void CSMPrefs::State::declareSeparator()
{
    if (mCurrentCategory==mCategories.end())
        throw std::logic_error ("no category for setting");

    CSMPrefs::Setting *setting =
        new CSMPrefs::Setting (&mCurrentCategory->second, &mSettings, &mMutex, "", "");

    mCurrentCategory->second.addSetting (setting);
}

void CSMPrefs::State::declareSubcategory(const std::string& label)
{
    if (mCurrentCategory==mCategories.end())
        throw std::logic_error ("no category for setting");

    CSMPrefs::Setting *setting =
        new CSMPrefs::Setting (&mCurrentCategory->second, &mSettings, &mMutex, "", label);

    mCurrentCategory->second.addSetting (setting);
}

void CSMPrefs::State::setDefault (const std::string& key, const std::string& default_)
{
    Settings::CategorySetting fullKey (mCurrentCategory->second.getKey(), key);

    Settings::CategorySettingValueMap::iterator iter =
        mSettings.mDefaultSettings.find (fullKey);

    if (iter==mSettings.mDefaultSettings.end())
        mSettings.mDefaultSettings.insert (std::make_pair (fullKey, default_));
}

CSMPrefs::State::State (const Files::ConfigurationManager& configurationManager)
: mConfigFile ("openmw-cs.cfg"), mConfigurationManager (configurationManager),
  mCurrentCategory (mCategories.end())
{
    if (sThis)
        throw std::logic_error ("An instance of CSMPRefs::State already exists");

    sThis = this;

    load();
    declare();
}

CSMPrefs::State::~State()
{
    sThis = 0;
}

void CSMPrefs::State::save()
{
    boost::filesystem::path user = mConfigurationManager.getUserConfigPath() / mConfigFile;
    mSettings.saveUser (user.string());
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

CSMPrefs::Category& CSMPrefs::State::operator[] (const std::string& key)
{
    Iterator iter = mCategories.find (key);

    if (iter==mCategories.end())
        throw std::logic_error ("Invalid user settings category: " + key);

    return iter->second;
}

void CSMPrefs::State::update (const Setting& setting)
{
    emit (settingChanged (&setting));
}

CSMPrefs::State& CSMPrefs::State::get()
{
    if (!sThis)
        throw std::logic_error ("No instance of CSMPrefs::State");

    return *sThis;
}


CSMPrefs::State& CSMPrefs::get()
{
    return State::get();
}
