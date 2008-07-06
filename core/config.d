/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (config.d) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

module core.config;

import std.string;
import std.file;
import std.path;
import std.stdio;

import monster.util.string;

import core.inifile;

import sound.audio;

import input.keys;
import input.ois;

//import sdl.Keysym;

//import input.events : updateMouseSensitivity;

ConfigManager config;

/*
 * Structure that handles all user adjustable configuration options,
 * including things like file paths, plugins, graphics resolution,
 * game settings, window positions, etc. It is also responsible for
 * reading and writing configuration files, for importing settings
 * from Morrowind.ini and for configuring OGRE. It doesn't currently
 * DO all of this, but it is supposed to in the future.
 */
struct ConfigManager
{
  IniWriter iniWriter;

  // Sound setting
  float musicVolume;
  float sfxVolume;
  float mainVolume;
  bool useMusic;

  // Mouse sensitivity
  float mouseSensX;
  float mouseSensY;
  bool flipMouseY;

  // Ogre configuration
  bool showOgreConfig; // The configuration setting
  // The actual result, overridable by a command line switch, and also
  // set to true if firstRun is true.
  bool finalOgreConfig;

  // Other settings
  bool firstRun;

  // Number of current screen shot. Saved upon exit, so that shots
  // from separate sessions don't overwrite each other.
  int screenShotNum;

  // Directories
  char[] esmDir;
  char[] bsaDir;
  char[] sndDir;
  char[] musDir; // Explore music
  char[] musDir2; // Battle music

  // Configuration file
  char[] confFile = "openmw.ini";

  // Cell to load at startup
  char[] defaultCell;

  // Check that a given volume is within sane limits (0.0-1.0)
  private static float saneVol(float vol)
  {
    if(!(vol >= 0)) vol = 0;
    else if(!(vol <= 1)) vol = 1;
    return vol;
  }

  // These set the volume to a new value and updates all sounds to
  // take notice.
  void setMusicVolume(float vol)
  {
    musicVolume = saneVol(vol);

    jukebox.updateVolume();
    battleMusic.updateVolume();
  }

  void setSfxVolume(float vol)
  {
    sfxVolume = saneVol(vol);

    // TODO: Call some sound manager here to adjust all active sounds
  }

  void setMainVolume(float vol)
  {
    mainVolume = saneVol(vol);

    // Update the sound managers
    setMusicVolume(musicVolume);
    setSfxVolume(sfxVolume);
  }

  // These calculate the "effective" volumes.
  float calcMusicVolume()
  {
    return musicVolume * mainVolume;
  }

  float calcSfxVolume()
  {
    return sfxVolume * mainVolume;
  }

  // Initialize the config manager. Send a 'true' parameter to reset
  // all keybindings to the default. A lot of this stuff will be moved
  // to script code at some point. In general, all input mechanics and
  // distribution of key events should happen in native code, while
  // all setup and control should be handled in script code.
  void initialize(bool reset = false)
  {
    // Initialize the key binding manager
    keyBindings.initKeys();

    /* Disable this at the moment. It's a good idea to put
       configuration in a central location, but it's useless as long
       as Ogre expects to find it's files in the current working
       directory. The best permanent solution would be to let the
       locations of ogre.cfg and plugins.cfg be determined by
       openmw.ini - I will fix that later.

    version(Posix)
      {
        if(!exists(confFile))
          confFile = expandTilde("~/.openmw/openmw.ini");
      }
    */

    readIni(reset);

    // I think DMD is on the brink of collapsing here. This has been
    // moved elsewhere, because DMD couldn't handle one more import in
    // this file.
    //updateMouseSensitivity();
  }

  // Read config from morro.ini, if it exists. The reset parameter is
  // set to true if we should use default key bindings instead of the
  // ones from the config file.
  void readIni(bool reset)
  {
    // Read configuration file, if it exists.
    IniReader ini;

    // TODO: Right now we have to specify each option twice, once for
    // reading and once for writing. Fix it? Nah. Don't do anything,
    // this entire configuration scheme is likely to change anyway.

    ini.readFile(confFile);

    screenShotNum = ini.getInt("General", "Screenshots", 0);
    mainVolume = saneVol(ini.getFloat("Sound", "Main Volume", 0.7));
    musicVolume = saneVol(ini.getFloat("Sound", "Music Volume", 0.5));
    sfxVolume = saneVol(ini.getFloat("Sound", "SFX Volume", 0.5));
    useMusic = ini.getBool("Sound", "Enable Music", true);

    mouseSensX = ini.getFloat("Controls", "Mouse Sensitivity X", 0.2);
    mouseSensY = ini.getFloat("Controls", "Mouse Sensitivity Y", 0.2);
    flipMouseY = ini.getBool("Controls", "Flip Mouse Y Axis", false);

    defaultCell = ini.getString("General", "Default Cell", "Sud");

    firstRun = ini.getBool("General", "First Run", true);
    showOgreConfig = ini.getBool("General", "Show Ogre Config", false);

    // This flag determines whether we will actually show the Ogre
    // config dialogue. The EITHER of the following are true, the
    // config box will be shown:
    // - The program is being run for the first time
    // - The "Show Ogre Config" option in openmw.ini is set.
    // - The -oc option is specified on the command line
    // - The file ogre.cfg is missing

    finalOgreConfig = showOgreConfig || firstRun ||
                      !exists("ogre.cfg");

    // Set default key bindings if the user specified the -rk setting,
    // or if no config file was found.
    if(reset || !ini.wasRead) with(keyBindings)
      {
        // Remove all existing key bindings
        //clear();

	// Bind some default keys
	bind(Keys.MoveLeft, KC.A, KC.LEFT);
	bind(Keys.MoveRight, KC.D, KC.RIGHT);
	bind(Keys.MoveForward, KC.W, KC.UP);
	bind(Keys.MoveBackward, KC.S, KC.DOWN);
	bind(Keys.MoveUp, KC.LSHIFT);
	bind(Keys.MoveDown, KC.LCONTROL);

	bind(Keys.MainVolUp, KC.ADD);
	bind(Keys.MainVolDown, KC.SUBTRACT);
	bind(Keys.MusVolDown, KC.N1);
	bind(Keys.MusVolUp, KC.N2);
	bind(Keys.SfxVolDown, KC.N3);
	bind(Keys.SfxVolUp, KC.N4);
        bind(Keys.Mute, KC.M);

	bind(Keys.ToggleBattleMusic, KC.SPACE);
        bind(Keys.Debug, KC.G);

	bind(Keys.Pause, KC.PAUSE, KC.P);
	bind(Keys.ScreenShot, KC.SYSRQ);
	bind(Keys.Exit, KC.Q, KC.ESCAPE);
      }
    else
      {
        // Read key bindings
        for(int i; i<Keys.Length; i++)
          {
            char[] s = keyToString[i];
            if(s.length)
              keyBindings.bindComma(cast(Keys)i, ini.getString("Bindings", s, ""));
          }
      }

    // Read specific directories
    bsaDir = ini.getString("General", "BSA Directory", "data/");
    esmDir = ini.getString("General", "ESM Directory", "data/");
    sndDir = ini.getString("General", "SFX Directory", "data/Sound/");
    musDir = ini.getString("General", "Explore Music Directory", "data/Music/Explore/");
    musDir2 = ini.getString("General", "Battle Music Directory", "data/Music/Battle/");
  }

  // Create the config file
  void writeConfig()
  {
    writefln("writeConfig(%s)", confFile);
    with(iniWriter)
      {
	openFile(confFile);

	comment("Don't write your own comments in this file, they");
	comment("will disappear when the file is rewritten.");
	section("General");
	writeString("ESM Directory", esmDir);
	writeString("BSA Directory", bsaDir);
	writeString("SFX Directory", sndDir);
	writeString("Explore Music Directory", musDir);
	writeString("Battle Music Directory", musDir2);
	writeInt("Screenshots", screenShotNum);
	writeString("Default Cell", defaultCell);

        // Save the setting as it appeared in the input. The setting
        // you specify in the ini is persistent, specifying the -oc
        // parameter does not change it.
        writeBool("Show Ogre Config", showOgreConfig);

        // The next run is never the first run.
        writeBool("First Run", false);

	section("Controls");
	writeFloat("Mouse Sensitivity X", mouseSensX);
	writeFloat("Mouse Sensitivity Y", mouseSensY);
	writeBool("Flip Mouse Y Axis", flipMouseY);

	section("Bindings");
	comment("Key bindings. The strings must match exactly.");
	foreach(int i, KeyBind b; keyBindings.bindings)
	  {
	    char[] s = keyToString[i];
	    if(s.length)
	      writeString(s, b.getString());
	  }

	section("Sound");
	writeFloat("Main Volume", mainVolume);
	writeFloat("Music Volume", musicVolume);
	writeFloat("SFX Volume", sfxVolume);
	writeBool("Enable Music", useMusic);

	close();
      }
  }

  // In the future this will import settings from Morrowind.ini, as
  // far as this is sensible.
  void importIni()
  {
    /*
    IniReader ini;
    ini.readFile("../Morrowind.ini");

    // Example of sensible options to convert:

    tryArchiveFirst = ini.getInt("General", "TryArchiveFirst");
    useAudio = ( ini.getInt("General", "Disable Audio") == 0 );
    footStepVolume = ini.getFloat("General", "PC Footstep Volume");
    subtitles = ini.getInt("General", "Subtitles") == 1;

    The plugin list (all esm and esp files) would be handled a bit
    differently. In our system they might be a per-user (per
    "character") setting, or even per-savegame. It should be safe and
    intuitive to try out a new mod without risking your savegame data
    or original settings. So these would be handled in a separate
    plugin manager.

    In any case, the import should be interactive and user-driven, so
    there is no use in making it before we have a gui of some sort up
    and running.
    */
  }
}
