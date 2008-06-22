/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (audiere.d) is part of the OpenMW package.

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

// This file exposes the C++ functions that deal directly with the
// sound library. The functions are implemented in cpp_audiere.cpp

module sound.audiere;

// A sound resource is the handle that represents the resource,
// ie. the file.
typedef void* AudiereResource;

// A sound instance is an instance of this resource. We can have
// several instances of the same file playing at once. Each instance
// has its own volume, file position, pitch, etc.
typedef void* AudiereInstance;

extern(C)
{
  // Open the music device. Returns 0 on success, 1 on failure.
  int cpp_openDevice();

  // Open a new sound resource. Returns null on failure.
  AudiereResource cpp_openSound(char* filename);

  // Close a resource.
  void cpp_closeSound(AudiereResource sound);

  // Create an instance of a sound.
  AudiereInstance cpp_createInstance(AudiereResource sound);
  
  // Create an instance by streaming directly from file, and play it.
  AudiereInstance cpp_playStream(char* filename, float volume);

  // Destroy a previously created instance
  void cpp_destroyInstance(AudiereInstance instance);

  // Is this instance currently playing?
  int cpp_isPlaying(AudiereInstance sound);

  // Adjust parameters for this instance.
  void cpp_setParams(AudiereInstance sound, float volume, float pan);

  // Play a sound.
  void cpp_playSound(AudiereInstance sound);

  // Set repeat mode on
  void cpp_setRepeat(AudiereInstance sound);

  // Stop a sound
  void cpp_stopSound(AudiereInstance sound);
}
