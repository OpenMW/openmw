Paths
#####

Default paths
=============

The following describes the default locations for the various OpenMW file paths for different types of files on different operating systems.
To see how to customise this, look at `Custom configuration directories`_.

.. note::
    Actual location depends on your computer's setup. Username, harddrive, and language may vary.

Configuration files and log files
---------------------------------

+--------------+-----------------------------------------------------------------------------------------------+
| OS           | Location                                                                                      |
+==============+===============================================================================================+
| Linux        | ``$XDG_CONFIG_HOME/openmw`` or ``$HOME/.config/openmw``                                       |
+--------------+-----------------------------------------------------------------------------------------------+
| Mac          | ``$HOME/Library/Preferences/openmw``                                                          |
+--------------+---------------+-------------------------------------------------------------------------------+
| Windows      | File Explorer | ``Documents\My Games\OpenMW``                                                 |
|              +---------------+-------------------------------------------------------------------------------+
|              | PowerShell    | ``Join-Path ([environment]::GetFolderPath("mydocuments")) "My Games\OpenMW"`` |
|              +---------------+-------------------------------------------------------------------------------+
|              | Example       | ``C:\Users\Username\Documents\My Games\OpenMW``                               |
+--------------+---------------+-------------------------------------------------------------------------------+

.. note::
    Flatpak sets ``$XDG_CONFIG_HOME`` to ``$HOME/.var/app/$FLATPAK_ID/config``, so these files will be at ``$HOME/.var/app/org.openmw.OpenMW/config/openmw`` if you use the Flatpak.

Savegames
---------

+--------------+-----------------------------------------------------------------------------------------------------+
| OS           | Location                                                                                            |
+==============+=====================================================================================================+
| Linux        | ``$XDG_DATA_HOME/openmw/saves`` or ``$HOME/.local/share/openmw/saves``                              |
+--------------+-----------------------------------------------------------------------------------------------------+
| Mac          | ``$HOME/Library/Application\ Support/openmw/saves``                                                 |
+--------------+---------------+-------------------------------------------------------------------------------------+
| Windows      | File Explorer | ``Documents\My Games\OpenMW\saves``                                                 |
|              +---------------+-------------------------------------------------------------------------------------+
|              | PowerShell    | ``Join-Path ([environment]::GetFolderPath("mydocuments")) "My Games\OpenMW\saves"`` |
|              +---------------+-------------------------------------------------------------------------------------+
|              | Example       | ``C:\Users\Username\Documents\My Games\OpenMW\saves``                               |
+--------------+---------------+-------------------------------------------------------------------------------------+

.. note::
    Flatpak sets ``$XDG_DATA_HOME`` to ``$HOME/.var/app/$FLATPAK_ID/data``, so saves will be at ``$HOME/.var/app/org.openmw.OpenMW/data/openmw/saves`` if you use the Flatpak.

Screenshots
-----------

+--------------+-----------------------------------------------------------------------------------------------------------+
| OS           | Location                                                                                                  |
+==============+===========================================================================================================+
| Linux        | ``$XDG_DATA_HOME/openmw/screenshots`` or ``$HOME/.local/share/openmw/screenshots``                        |
+--------------+-----------------------------------------------------------------------------------------------------------+
| Mac          | ``$HOME/Library/Application\ Support/openmw/screenshots``                                                 |
+--------------+---------------+-------------------------------------------------------------------------------------------+
| Windows      | File Explorer | ``Documents\My Games\OpenMW\screenshots``                                                 |
|              +---------------+-------------------------------------------------------------------------------------------+
|              | PowerShell    | ``Join-Path ([environment]::GetFolderPath("mydocuments")) "My Games\OpenMW\screenshots"`` |
|              +---------------+-------------------------------------------------------------------------------------------+
|              | Example       | ``C:\Users\Username\Documents\My Games\OpenMW\screenshots``                               |
+--------------+---------------+-------------------------------------------------------------------------------------------+

.. note::
    Flatpak sets ``$XDG_DATA_HOME`` to ``$HOME/.var/app/$FLATPAK_ID/data``, so screenshots will be at ``$HOME/.var/app/org.openmw.OpenMW/data/openmw/screenshots`` if you use the Flatpak.

Custom configuration directories
================================

OpenMW has a powerful system to control where configuration files and user data are stored.
This is useful for things like a `Portable install`_ or `Profiles`_\ , but can also be used if you just want your saved games to end up somewhere else.
The OpenMW engine has supported this since 0.48, but if you want to use OpenMW's launcher, that only works correctly from 0.49 onwards.

Concepts
--------
Configuration sources
^^^^^^^^^^^^^^^^^^^^^

Configuration for OpenMW is composed from several sources.
From lowest to highest priority, these are:

* The local or global ``openmw.cfg``.
* Any ``openmw.cfg`` files in any other directories specified with the ``config`` option.
* Options specified on the command line.

.. tip::
    Any option that can be specified as ``option=value`` in ``openmw.cfg`` can also be specified as ``--option=value`` or ``--option value`` on the command line for the engine (but not other OpenMW tools).

The command line is always the highest-priority source, even if it specifies extra configuration directories with ``--config``.
This lets you quickly test with extra options even if you're not using your normal configuration directories.

If one ``openmw.cfg`` specifies multiple configuration directories, they're all higher-priority than the one that specified them, and lower priority than any they go on to specify themselves.
I.e. if ``dir1/openmw.cfg`` contains ``config=dir2`` then ``config=dir3``, and ``dir2/openmw.cfg`` contains ``config=dir4`` the priority order will be ``dir1``, ``dir2``, ``dir3``, ``dir4``.
This might be a surprise if you expect it to work like C's ``#include`` directive.

Most settings in ``openmw.cfg`` only allow a single value, so will take the one from the highest-priority config source that sets one.
Others take several values which are combined together into a list.
Values from low-priority sources are included earlier than ones from high-priority sources.
If you don't want this, and would prefer that only values from the current source (and ones of higher priority) are used, you can specify this for each option with the ``replace`` option.
E.g. passing ``--replace content --content Morrowind.esm`` on the command line will ignore any ``content=…`` lines in your ``openmw.cfg``\ (s) and run the game with only ``Morrowind.esm``.

Every configuration directory is allowed, but not required, to contain its own ``settings.cfg``.
Settings in ``settings.cfg`` only ever have one value, so it'll be taken from the highest priority ``settings.cfg`` that sets one.

Special ``openmw.cfg`` files
""""""""""""""""""""""""""""

A local ``openmw.cfg`` is one which is in the same directory as the OpenMW binary, e.g. ``openmw.exe`` on Windows.
It is always loaded if it exists.

A global ``openmw.cfg`` is one in a special system-specific location for system-wide application configuration, e.g. ``/etc/openmw`` on some Linux systems.
Not all systems have a directory like this – a global ``openmw.cfg`` only makes sense if there's a single system-wide OpenMW installation, e.g. from the operating system's package manager.
It is only loaded if there's no local ``openmw.cfg``.

If there's no local or global ``openmw.cfg``, OpenMW won't launch.

The user ``openmw.cfg`` is the highest priority ``openmw.cfg`` file that's active.
This is the one that the launcher will edit, so it must be somewhere the user has write access.
It *can* be the local ``openmw.cfg``, e.g. in a `Portable install`_, but we strongly recommend against installing OpenMW to a system protected directory (e.g. ``/usr/bin`` on Unix, ``C:\Program Files`` on Windows) if you decide to do this.
We also strongly recommend against using a global ``openmw.cfg`` as the user ``openmw.cfg``.

``openmw.cfg`` syntax
^^^^^^^^^^^^^^^^^^^^^

An ``openmw.cfg`` file is a sequence of lines.
Each line is either blank, contains an option, or contains a comment.

Blank lines are ignored.

Lines where the first non-whitespace character is an octothorpe (``#``), also known as the hash symbol or pound sign, are comments.
The line is ignored no matter what else it contains.
You can use comments to make notes for yourself or temporarily make OpenMW ignore specific lines.
Be aware that the launcher can only make a best effort to preserve comments when you use it to edit your user ``openmw.cfg``.
It has no way of knowing if you've written a comment to describe the lines above it versus below it (if a comment even goes with a particular line), so it has to guess.
This can't change until computers are able to read minds.

Lines with options have an option name, then an equals sign (``=``), then an option value.
Option names and values have leading and trailing whitespace trimmed, but whitespace within an option value is preserved - it's only removed if it's at the ends.
This means that these are all equivalent:

.. code-block:: openmwcfg

    data=some/dir
        data=some/dir
    data = some/dir

As mentioned above, some options allow more than one value, but some only allow one.
If only one is allowed, and you provide two in the same file, the later one is used.

Extra rules for paths
"""""""""""""""""""""

OpenMW accepts Unix-style paths (separated by forward slashes (``/``)) on Unix, and both Unix-style and Windows-style (separated by backward slashes (``\``)) on Windows, including mixed paths.
Backward slashes have no special meaning and are not an escape character, so paths can be copied and pasted straight from your file browser on any platform.

Paths can also use a quoted syntax.
This is mainly useful if you want to make an ``openmw.cfg`` file that also works with older versions, where quoting was mandatory.
It also lets you specify paths with whitespace at the beginning or end, which would otherwise be stripped.

To quote a path, put a double quote mark (``"``) at the beginning and end.
Everything after the closing quote mark will be ignored.
If a path has quote marks within it, they can be escaped by putting an ampersand (``&``) first, and ampersands can be escaped by putting another ampersand first.
E.g. ``data=a/path/with a " symbol & an ampersand`` can also be written as ``data="a/path/with a &" symbol && an ampersand"``.
If you don't need your configuration to work with older versions, then it's usually easier not to bother quoting paths.

Paths can be absolute, relative, or start with a token.

Absolute paths start with a slash (``/`` or ``\``), or, on Windows, a drive identifier (e.g. ``C:\``).

Relative paths are **relative to the** ``openmw.cfg`` **file they're in**, or the current working directory if they're passed via the command line.

Tokens are used to access platform-dependent paths where OpenMW can store specific kinds of data.
The available tokens are ``?local?``, ``?userconfig?``, ``?userdata?`` and ``?global?``.
Tokens are used in the `Default paths`_.

:``?local?``: The directory where the OpenMW binary is installed, except on MacOS, where it's the ``Resources`` directory inside the bundle.

:``?userconfig?``: Platform-dependent:

    +--------------+-----------------------------------------------------------+
    | OS           | Location                                                  |
    +==============+===========================================================+
    | Linux        | ``$XDG_CONFIG_HOME/openmw/`` or ``$HOME/.config/openmw/`` |
    +--------------+-----------------------------------------------------------+
    | Mac          | ``$HOME/Library/Preferences/openmw/``                     |
    +--------------+-----------------------------------------------------------+
    | Windows      | ``Documents\My Games\OpenMW\``                            |
    +--------------+-----------------------------------------------------------+

.. note::
    Flatpak sets ``$XDG_CONFIG_HOME`` to ``$HOME/.var/app/$FLATPAK_ID/config``, so ``?userconfig?`` will mean ``$HOME/.var/app/org.openmw.OpenMW/config/openmw/`` if you use the Flatpak.

:``?userdata?``: Platform-dependent:

    +--------------+--------------------------------------------------------------+
    | OS           | Location                                                     |
    +==============+==============================================================+
    | Linux        | ``$XDG_DATA_HOME/openmw/`` or ``$HOME/.local/share/openmw/`` |
    +--------------+--------------------------------------------------------------+
    | Mac          | ``$HOME/Library/Application Support/openmw/``                |
    +--------------+--------------------------------------------------------------+
    | Windows      | ``Documents\My Games\OpenMW\``                               |
    +--------------+--------------------------------------------------------------+

.. note::
    Flatpak sets ``$XDG_DATA_HOME`` to ``$HOME/.var/app/$FLATPAK_ID/data``, so ``?userdata?`` will mean ``$HOME/.var/app/org.openmw.OpenMW/data/openmw/`` if you use the Flatpak.

:``?global?``: Platform-dependent:

    +--------------+-------------------------------------------------------------------+
    | OS           | Location                                                          |
    +==============+===================================================================+
    | Linux        | Chosen by the downstream packager, typically ``/usr/share/games`` |
    +--------------+-------------------------------------------------------------------+
    | Mac          | ``/Library/Application Support/``                                 |
    +--------------+-------------------------------------------------------------------+
    | Windows      | Not applicable                                                    |
    +--------------+-------------------------------------------------------------------+

Examples
--------

Portable install
^^^^^^^^^^^^^^^^

If you want to put OpenMW onto removable storage so you can play on multiple machines, or you want an entirely self-contained setup, you'll want to set up a portable install.

Single ``openmw.cfg`` file
""""""""""""""""""""""""""

Some users find it easiest if there's a single ``openmw.cfg`` file with all their configuration, even if it means it's mixed in with the engine's default configuration.

To set up this kind of install, first install a fresh copy of OpenMW to a directory where you have write access.
Navigate to the OpenMW installation directory, and open the ``openmw.cfg`` file it contains.

By default, this contains a warning at the top telling you that this is the local ``openmw.cfg`` and not to modify it.
However, for this kind of install, it's okay to do so, so you can remove this warning.

Change the start of the file from:

.. code-block:: openmwcfg
    :caption: openmw.cfg

    # This is the local openmw.cfg file. Do not modify!
    # Modifications should be done on the user openmw.cfg file instead
    # (see: https://openmw.readthedocs.io/en/master/reference/modding/paths.html)

    data-local="?userdata?data"
    user-data="?userdata?"
    config="?userconfig?"
    resources=./resources
    data=./resources/vfs-mw

    # lighting
    fallback=LightAttenuation_UseConstant,0
    fallback=LightAttenuation_ConstantValue,0.0
    fallback=LightAttenuation_UseLinear,1

to:

.. code-block:: openmwcfg
    :caption: openmw.cfg

    data-local=userdata/data
    user-data=userdata
    resources=./resources
    data=./resources/vfs-mw

    # lighting
    fallback=LightAttenuation_UseConstant,0
    fallback=LightAttenuation_ConstantValue,0.0
    fallback=LightAttenuation_UseLinear,1

You can now run OpenMW's launcher to do first-time setup.
This will import the basic data to play *Morrowind* into the ``openmw.cfg`` you just modified, and create a ``settings.cfg`` next to it.
You can make any further changes you want to these files, or make changes in the launcher, which will modify them for you.

You'll need to make sure that any ``data=…`` lines in your ``openmw.cfg`` use relative paths so that they're not dependent on the drive letter/mount point when moved to another computer.
If you add data directories via the launcher, you'll need to change them manually afterwards.

Separate user ``openmw.cfg`` file
"""""""""""""""""""""""""""""""""

For most users, this is the type of portable OpenMW install we would recommend as it's the most similar to a regular install.
You'll have a separate local ``openmw.cfg`` with the engine's basic configuration and a user ``openmw.cfg`` with your personal configuration.

To set up this kind of install, first install a fresh copy of OpenMW to a directory where you have write access.
Navigate to the OpenMW installation directory, and open the ``openmw.cfg`` file it contains.

By default, this contains a warning at the top telling you that this is the local ``openmw.cfg`` and not to modify it.
However, you'll need to make a small change to create this kind of install.

Change the start of the file from:

.. code-block:: openmwcfg
    :caption: openmw.cfg

    # This is the local openmw.cfg file. Do not modify!
    # Modifications should be done on the user openmw.cfg file instead
    # (see: https://openmw.readthedocs.io/en/master/reference/modding/paths.html)

    data-local="?userdata?data"
    user-data="?userdata?"
    config="?userconfig?"
    resources=./resources
    data=./resources/vfs-mw

    # lighting
    fallback=LightAttenuation_UseConstant,0
    fallback=LightAttenuation_ConstantValue,0.0
    fallback=LightAttenuation_UseLinear,1

to:

.. code-block:: openmwcfg
    :caption: openmw.cfg

    # This is the local openmw.cfg file. Do not modify!
    # Modifications should be done on the user openmw.cfg file instead
    # (see: https://openmw.readthedocs.io/en/master/reference/modding/paths.html)

    data-local="userdata/data"
    user-data="userdata"
    config="config"
    resources=./resources
    data=./resources/vfs-mw

    # lighting
    fallback=LightAttenuation_UseConstant,0
    fallback=LightAttenuation_ConstantValue,0.0
    fallback=LightAttenuation_UseLinear,1

You can now run OpenMW's launcher to do first-time setup.
This will import the basic data to play Morrowind into a new ``openmw.cfg`` in the ``config`` directory, and create a ``settings.cfg`` next to it.
You can make any further changes you want to these files, or make changes in the launcher, which will modify them for you.

You'll need to make sure that any ``data=…`` lines in your ``openmw.cfg`` use relative paths so that they're not dependent on the drive letter/mount point when moved to another computer.
Remember that paths are relative to the ``openmw.cfg`` file they're in, not the OpenMW installation root.
If you add data directories via the launcher, you'll need to change them manually afterwards.

Profiles
^^^^^^^^

OpenMW can potentially be used to play several different games, and you may want to try several different mod lists for each.
You can use the custom configuration directory system to create a profile for each different setup with its own configuration directory.

For the example, we'll create a subdirectory in the default configuration directory for each game, and then create a subdirectory in the relevant game's directory for each mod list.

From scratch
""""""""""""

Start by installing OpenMW in the usual way.
Don't bother with first-time setup (i.e. telling it the location of an existing *Morrowind* installation).

In the default configuration directory (see `Configuration files and log files`_), create a file called ``openmw.cfg`` containing just

.. code-block:: openmwcfg
    :caption: openmw.cfg

    # select the game profile
    config=Morrowind

Now it's time to run the launcher to do first-time setup.
This will put the basic setup required to play *Morrowind* into a new ``Morrowind`` directory of the default configuration directory, e.g. ``Documents\My Games\OpenMW\Morrowind\openmw.cfg`` on Windows.

Next, come up with a name for the subprofile you'll create for your mod list.
If you're following a modding guide, they've probably already given it a name, e.g. *Total Overhaul*, so that's the example we'll use.
Add a line to the ``Morrowind/openmw.cfg`` with the profile name like this:

.. code-block:: openmwcfg
    :caption: Morrowind/openmw.cfg

    # select the mod list profile
    config=Total Overhaul

Run the launcher again.

You'll now have three separate levels of ``openmw.cfg`` and ``settings.cfg``.

The ones in the base default configuration directory are used for all profiles, so they're best for machine-wide settings, like your monitor's resolution.

The ones in the ``Morrowind`` directory are used for all profiles for *Morrowind*, so they're best for game-specific settings, like the values imported from ``Morrowind.ini``.

The ones in the ``Morrowind/Total Overhaul`` directory are only used for the *Total Overhaul* profile, so you can set up that mod list and any settings it requires here, and they won't affect any other profiles you set up later.
Making changes within the launcher will affect these files and leave all the others alone.

If you want the *Total Overhaul* profile to keep its saved games etc. in a dedicated location instead of mixing them in with ones from another profile, you can add a ``user-data=…`` line to your ``Morrowind/Total Overhaul/openmw.cfg``, like this:

.. code-block:: openmwcfg
    :caption: Morrowind/Total Overhaul/openmw.cfg

    # put saved games in a saves directory next to this file
    user-data=.

When you want to set up another game or mod list, you can set up a new one just like the first – create another directory for it and change the ``config=…`` line in the ``openmw.cfg`` next to the directory to use that directory's name.
To switch back, just change the line back.

Migrating an existing setup
"""""""""""""""""""""""""""

Lots of people will have an existing OpenMW setup, and decide they want to try a new mod list or game.
That existing configuration can be moved out of the way and turned into a profile.

Start by creating a subdirectory in the default configuration directory (see `Configuration files and log files`_) to be the profile.
Give it a meaningful name so you know what it is – this example will call it *Original*.
You'll now have an empty directory e.g. at ``Documents\My Games\OpenMW\Original`` on Windows.

Next, move all the files that were already in the default configuration directory to the profile directory you just made.
Afterwards, the default configuration directory should only contain the profile directory you made.

Create a new ``openmw.cfg`` file in the default configuration directory containing:

.. code-block:: openmwcfg
    :caption: openmw.cfg

    # select the profile
    config=Original

In the ``openmw.cfg`` in the profile directory, add these lines:

.. code-block:: openmwcfg
    :caption: openmw.cfg

    data-local=data
    user-data=.

Now, if you run OpenMW or any of its tools, they'll work just like before, even though you've moved the files.

You can now make other directories for other profiles as described in the `From scratch`_ example, and switch between them and your original setup by changing the ``config=…`` line.

Launcher scripts and shortcuts
""""""""""""""""""""""""""""""

Once profiles have been set up, it might be a hassle to switch between them by editing ``config=…`` lines in ``openmw.cfg`` files.
Passing arguments on the command line lets you avoid this.

.. note::
    This feature only works with the OpenMW engine, not tools like the launcher.

The basic idea is that you need to pass ``--replace config`` to ignore the configuration directories that the engine would have loaded because they were specified in ``openmw.cfg`` files, and pass each one you want to use instead with ``--config <directory path here>``.

E.g. if you've got a profile called *Morrowind* in your default configuration directory, and it's got a *Total Overhaul* subprofile, you could load it by running:

.. code-block:: console

    $ openmw --replace config --config ?userconfig?/Morrowind --config "?userconfig?/Morrowind/Total Overhaul"

You can put this command into a script or shortcut and use it to easily launch OpenMW with that profile.

The command exactly as it appears above will work in most common shells (e.g. Bash, Windows Command Prompt and PowerShell) if OpenMW is on the system path.
Otherwise, the path to OpenMW must be specified instead of just the ``openmw`` command.

On Windows, you can create a desktop shortcut to run this command with these steps:

* Navigate to the OpenMW install directory.
* Right-click ``openmw.exe`` and choose *Send to* > *Desktop (create shortcut)*.
* Navigate to the Desktop, or minimise all windows.
* Find the newly-created shortcut and give it a sensible name, e.g. *OpenMW - Total Overhaul*.
* Right-click the shortcut and choose *Properties*.
* In the *Shortcut* tab of the *Properties* pane, find the *Target* field.
* At the end of that field, add the arguments for the profile you want, e.g. ``--replace config --config ?userconfig?/Morrowind --config "?userconfig?/Morrowind/Total Overhaul"``.
* Press *Apply* or *OK* to save the changes, and test the shortcut by double-clicking it.

On most Linux distros, you can create a ``.desktop`` file like this:

.. code-block:: desktop

	[Desktop Entry]
	Type=Application
	Name=OpenMW - Total Overhaul
	GenericName=Role Playing Game
	Comment=OpenMW with the Total Overhaul profile
	Keywords=Morrowind;Reimplementation Mods;esm;bsa;
	TryExec=openmw
	Exec=openmw --replace config --config ?userconfig?/Morrowind --config "?userconfig?/Morrowind/Total Overhaul" 
	Icon=openmw
	Categories=Game;RolePlaying;
