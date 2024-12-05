---
-- `openmw.ambient` controls background 2D sounds specific to a given player.
-- Can be used only by menu scripts and local scripts that are attached to a player.
-- @module ambient
-- @usage local ambient = require('openmw.ambient')



---
-- Play a 2D sound
-- @function [parent=#ambient] playSound
-- @param #string soundId ID of Sound record to play
-- @param #table options An optional table with additional optional arguments. Can contain:
--
--   * `timeOffset` - a floating point number >= 0, to skip some time (in seconds) from the beginning of the sound (default: 0);
--   * `volume` - a floating point number >= 0, to set the sound's volume (default: 1);
--   * `pitch` - a floating point number >= 0, to set the sound's pitch (default: 1);
--   * `scale` - a boolean, to set if the sound's pitch should be scaled by simulation time scaling (default: true);
--   * `loop` - a boolean, to set if the sound should be repeated when it ends (default: false);
-- @usage local params = {
--    timeOffset=0.1
--    volume=0.3,
--    scale=false,
--    pitch=1.0,
--    loop=true
-- };
-- ambient.playSound("shock bolt", params)

---
-- Play a 2D sound file
-- @function [parent=#ambient] playSoundFile
-- @param #string fileName Path to a sound file in VFS
-- @param #table options An optional table with additional optional arguments. Can contain:
--
--   * `timeOffset` - a floating point number >= 0, to skip some time (in seconds) from the beginning of the sound file (default: 0);
--   * `volume` - a floating point number >= 0, to set the sound's volume (default: 1);
--   * `pitch` - a floating point number >= 0, to set the sound's pitch (default: 1);
--   * `scale` - a boolean, to set if the sound's pitch should be scaled by simulation time scaling (default: true);
--   * `loop` - a boolean, to set if the sound should be repeated when it ends (default: false);
-- @usage local params = {
--    timeOffset=0.1
--    volume=0.3,
--    scale=false,
--    pitch=1.0,
--    loop=true
-- };
-- ambient.playSoundFile("Sound\\test.mp3", params)

---
-- Stop a sound
-- @function [parent=#ambient] stopSound
-- @param #string soundId ID of Sound record to stop
-- @usage ambient.stopSound("shock bolt");

---
-- Stop a sound file
-- @function [parent=#ambient] stopSoundFile
-- @param #string fileName Path to a sound file in VFS
-- @usage ambient.stopSoundFile("Sound\\test.mp3");

---
-- Check if a sound is playing
-- @function [parent=#ambient] isSoundPlaying
-- @param #string soundId ID of Sound record to check
-- @return #boolean
-- @usage local isPlaying = ambient.isSoundPlaying("shock bolt");

---
-- Check if a sound file is playing
-- @function [parent=#ambient] isSoundFilePlaying
-- @param #string fileName Path to a sound file in VFS
-- @return #boolean
-- @usage local isPlaying = ambient.isSoundFilePlaying("Sound\\test.mp3");

---
-- Play a sound file as a music track
-- @function [parent=#ambient] streamMusic
-- @param #string fileName Path to a file in VFS
-- @param #table options An optional table with additional optional arguments. Can contain:
--
--   * `fadeOut` - a floating point number >= 0, time (in seconds) to fade out the current track before playing this one (default 1.0);
-- @usage local params = {
--    fadeOut=2.0
-- };
-- ambient.streamMusic("Music\\Test\\Test.mp3", params)

---
-- Stop the currently playing music
-- @function [parent=#ambient] stopMusic
-- @usage ambient.stopMusic();

---
-- Check if music is playing
-- @function [parent=#ambient] isMusicPlaying
-- @return #boolean
-- @usage local isPlaying = ambient.isMusicPlaying();

---
-- Play an ambient voiceover.
-- @function [parent=#ambient] say
-- @param #string fileName Path to a sound file in VFS
-- @param #string text Subtitle text (optional)
-- @usage -- play voiceover and print messagebox
-- ambient.say("Sound\\Vo\\Misc\\voice.mp3", "Subtitle text")
-- @usage -- play voiceover, without messagebox
-- ambient.say("Sound\\Vo\\Misc\\voice.mp3")

---
-- Stop an ambient voiceover
-- @function [parent=#ambient] stopSay
-- @param #string fileName Path to a sound file in VFS
-- @usage ambient.stopSay();

---
-- Check if an ambient voiceover is playing
-- @function [parent=#Sound] isSayActive
-- @return #boolean
-- @usage local isActive = isSayActive();

return nil
