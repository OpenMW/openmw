/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (events.d) is part of the OpenMW package.

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

module input.events;

import std.stdio;
import std.string;

import sound.audio;

import core.config;

import scene.soundlist;
import scene.player;

import ogre.bindings;

import input.keys;
import input.ois;

// Debug output
//debug=printMouse;      // Mouse button events
//debug=printMouseMove;  // Mouse movement events
//debug=printKeys;       // Keypress events

// TODO: Jukebox controls and other state-related data will later be
// handled entirely in script code, as will some of the key bindings.

// Are we currently playing battle music?
bool battle = false;

// Pause?
bool pause = false;

// Temporarily store volume while muted
float muteVolume = -1;

void toggleMute()
{
  if(muteVolume < 0)
    {
      muteVolume = config.mainVolume;
      config.setMainVolume(0);
      writefln("Muted");
    }
  else
    {
      config.setMainVolume(muteVolume);
      muteVolume = -1;
      writefln("Mute off");
    }
}

// Switch between normal and battle music
void toggleBattle()
{
  if(battle)
    {
      writefln("Changing to normal music");
      jukebox.resumeMusic();
      battleMusic.pauseMusic();
      battle=false;
    }
  else
    {
      writefln("Changing to battle music");
      jukebox.pauseMusic();
      battleMusic.resumeMusic();
      battle=true;
    }
}

const float volDiff = 0.05;

void musVolume(bool increase)
{
  float diff = -volDiff;
  if(increase) diff = -diff;
  config.setMusicVolume(diff + config.musicVolume);
  writefln(increase?"Increasing":"Decreasing", " music volume to ", config.musicVolume);
}

void sfxVolume(bool increase)
{
  float diff = -volDiff;
  if(increase) diff = -diff;
  config.setSfxVolume(diff + config.sfxVolume);
  writefln(increase?"Increasing":"Decreasing", " sound effect volume to ", config.sfxVolume);
}

void mainVolume(bool increase)
{
  float diff = -volDiff;
  if(increase) diff = -diff;
  config.setMainVolume(diff + config.mainVolume);
  writefln(increase?"Increasing":"Decreasing", " main volume to ", config.mainVolume);
}

void takeScreenShot()
{
  char[] file = format("screenshot_%06d.png", config.screenShotNum++);
  cpp_screenshot(toStringz(file));
  writefln("Wrote '%s'", file);
}

// Mouse sensitivity
float effMX, effMY;

void updateMouseSensitivity()
{
  effMX = config.mouseSensX;
  effMY = config.mouseSensY;
  if(config.flipMouseY) effMY = -effMY;
}

void togglePause()
{
  pause = !pause;
  if(pause) writefln("Pause");
  else writefln("Pause off");
}

bool doExit = false;

void exitProgram()
{
  doExit = true;
}

extern(C) void d_handleMouseMove(MouseState *state)
{
  debug(printMouseMove)
    writefln("handleMouseMove: Abs(%s, %s, %s) Rel(%s, %s, %s)",
             state.X.abs, state.Y.abs, state.Z.abs,
             state.X.rel, state.Y.rel, state.Z.rel);

  cpp_rotateCamera( state.X.rel * effMX,
                    state.Y.rel * effMY );
}

extern(C) void d_handleMouseButton(MouseState *state, int button)
{
  debug(printMouse)
    writefln("handleMouseButton %s: Abs(%s, %s, %s)", button, 
             state.X.abs, state.Y.abs, state.Z.abs);

  // For the moment, just treat mouse clicks as normal key presses.
  d_handleKey(cast(KC) (KC.Mouse0 + button));
}

// Handle a keyboard event through key bindings. Direct movement
// (eg. arrow keys) is not handled here, see d_frameStarted() below.
extern(C) void d_handleKey(KC keycode, dchar text = 0)
{
  // Do some preprocessing on the data to account for OIS
  // shortcommings.

  // Some keys (especially international keys) have no key code but
  // return a character instead.
  if(keycode == 0)
    {
      // If no character is given, just drop this event since OIS did
      // not manage to give us any useful data at all.
      if(text == 0) return;

      keycode = KC.CharOnly;
    }

  // Debug output
  debug(printKeys)
    {
      char[] str;
      if(keycode >= 0 && keycode < keysymToString.length)
        str = keysymToString[keycode];
      else str = "OUT_OF_RANGE";
      writefln("Key %s, text '%s', name '%s'", keycode, text, str);
    }

  // Look up the key binding. We have to send both the keycode and the
  // text.
  Keys k = keyBindings.findMatch(keycode, text);

  if(k)
    switch(k)
      {
      case Keys.ToggleBattleMusic: toggleBattle(); break;

      case Keys.MainVolUp: mainVolume(true); break;
      case Keys.MainVolDown: mainVolume(false); break;
      case Keys.MusVolUp: musVolume(true); break;
      case Keys.MusVolDown: musVolume(false); break;
      case Keys.SfxVolUp: sfxVolume(true); break;
      case Keys.SfxVolDown: sfxVolume(false); break;
      case Keys.Mute: toggleMute(); break;

      case Keys.Debug: cpp_debug(0); break;
      case Keys.ScreenShot: takeScreenShot(); break;
      case Keys.Pause: togglePause(); break;
      case Keys.Exit: exitProgram(); break;
      default:
        assert(k >= 0 && k < keyToString.length);
        writefln("WARNING: Event %s has no effect", keyToString[k]);
      }
  return false;
}

// Refresh rate for sfx placements, in seconds.
const float sndRefresh = 0.17;

// Refresh rate for music fadeing, seconds.
const float musRefresh = 0.2;

float sndCumTime = 0;
float musCumTime = 0;

void initializeInput()
{
  // Move the camera in place
  with(playerData.position)
    {
      cpp_moveCamera(position[0], position[1], position[2],
		     rotation[0], rotation[1], rotation[2]);
    }

  // TODO/FIXME: This should have been in config, but DMD's module
  // system is on the brink of collapsing, and it won't compile if I
  // put another import in core.config. I should probably check the
  // bug list and report it.
  updateMouseSensitivity();
}

float tmpTime = 0;
int cnt;

extern(C) int cpp_isPressed(int keysym);

// Check if a key is currently down
bool isPressed(Keys key)
{
  KeyBind *b = &keyBindings.bindings[key];
  foreach(i; b.syms)
    if(i != 0 && cpp_isPressed(i)) return true;
  return false;
}

extern(C) int d_frameStarted(float time)
{
  /*
  tmpTime += time;
  cnt++;
  if(tmpTime >= 1.0)
    {
      writefln("\nfps: ", cnt/tmpTime);
      cnt = 0;
      tmpTime = 0;
    }
  //*/

  if(doExit) return 0;

  musCumTime += time;
  if(musCumTime > musRefresh)
    {
      jukebox.addTime(musRefresh);
      battleMusic.addTime(musRefresh);
      musCumTime -= musRefresh;
    }

  // The rest is ignored in pause mode
  if(pause) return 1;

  const float moveSpeed = 900;

  // Check if the movement keys are pressed
  float moveX = 0, moveY = 0, moveZ = 0;

  if(isPressed(Keys.MoveLeft)) moveX -= moveSpeed;
  if(isPressed(Keys.MoveRight)) moveX += moveSpeed;
  if(isPressed(Keys.MoveForward)) moveZ -= moveSpeed;
  if(isPressed(Keys.MoveBackward)) moveZ += moveSpeed;
  if(isPressed(Keys.MoveUp)) moveY += moveSpeed;
  if(isPressed(Keys.MoveDown)) moveY -= moveSpeed;

  // Move camera. We only support "ghost-mode" at the moment, so we
  // move without physics or collision detection.
  cpp_moveCameraRel(moveX*time, moveY*time, moveZ*time);

  sndCumTime += time;
  if(sndCumTime > sndRefresh)
    {
      float x, y, z;
      cpp_getCameraPos(&x, &y, &z);

      soundScene.update(x,y,z);
      sndCumTime -= sndRefresh;
    }

  return 1;
}

